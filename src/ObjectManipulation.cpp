#include "ObjectManipulation.h"
#include "WeaponOrientation.h"
#include "Settings.h"
#include "Util.h"
#include <cmath>
#include <numbers>

ObjectManipulation* ObjectManipulation::GetSingleton() {
    static ObjectManipulation instance;
    return &instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// HeldObjectData helpers
// ─────────────────────────────────────────────────────────────────────────────
RE::TESObjectREFR* HeldObjectData::Ref() const {
    RE::TESObjectREFR* ref = nullptr;
    RE::LookupReferenceByHandle(handle, ref);
    return ref;
}

bool HeldObjectData::IsValid() const {
    auto* ref = Ref();
    return ref && !ref->IsDisabled() && !ref->IsDeleted();
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginPull — disable gravity, apply initial velocity toward hold position
// ─────────────────────────────────────────────────────────────────────────────
void ObjectManipulation::BeginPull(RE::TESObjectREFR* ref) const {
    auto* body = HavokUtil::GetRigidBody(ref);
    if (!body) return;

    HavokUtil::SetGravityFactor(body, 0.0f);

    // Velocity will be set on the first UpdateHold frame to pull direction.
    HavokUtil::SetLinearVelocity(body, {});
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateHold — spring-damper physics (Bioshock-style inertia)
// ─────────────────────────────────────────────────────────────────────────────
void ObjectManipulation::UpdateHold(HeldObjectData& obj, float dt) const {
    if (!obj.IsValid()) return;

    auto* ref  = obj.Ref();
    auto* body = HavokUtil::GetRigidBody(ref);
    if (!body) return;

    auto* settings = Settings::GetSingleton();

    RE::NiPoint3 target  = CalcHoldPosition(obj.slotIndex,
                                             /* totalCount filled by caller */ 1,
                                             obj.vrLeftHand);
    RE::NiPoint3 current = HavokUtil::GetPosition(body);
    RE::NiPoint3 vel     = HavokUtil::GetLinearVelocity(body);

    // Spring force: F = k * (target - current)
    RE::NiPoint3 spring = {
        settings->springConstant * (target.x - current.x),
        settings->springConstant * (target.y - current.y),
        settings->springConstant * (target.z - current.z)
    };

    // Damper force: F -= d * vel
    RE::NiPoint3 damper = {
        -settings->dampingCoefficient * vel.x,
        -settings->dampingCoefficient * vel.y,
        -settings->dampingCoefficient * vel.z
    };

    RE::NiPoint3 newVel = {
        vel.x + (spring.x + damper.x) * dt,
        vel.y + (spring.y + damper.y) * dt,
        vel.z + (spring.z + damper.z) * dt
    };

    newVel = MathUtil::Clamp(newVel, settings->maxHoldSpeed);
    HavokUtil::SetLinearVelocity(body, newVel);

    // If this is a Pulling state and we're close enough, snap to Holding
    if (obj.state == HeldObjectState::Pulling) {
        RE::NiPoint3 diff  = { target.x - current.x, target.y - current.y, target.z - current.z };
        float        distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSq < 50.0f * 50.0f) {  // within 50 units → considered arrived
            obj.state = HeldObjectState::Holding;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Throw — apply impulse in aim direction, scaled by chargeLevel
// ─────────────────────────────────────────────────────────────────────────────
void ObjectManipulation::Throw(HeldObjectData& obj) const {
    if (!obj.IsValid()) return;

    auto* ref  = obj.Ref();
    auto* body = HavokUtil::GetRigidBody(ref);
    if (!body) return;

    auto* settings = Settings::GetSingleton();

    RE::NiPoint3 aimDir = settings->vrEnabled
                              ? MathUtil::GetVRHandForward(obj.vrLeftHand)
                              : MathUtil::GetCameraForward();

    float chargeMultiplier = 1.0f + obj.chargeLevel * (settings->throwChargeMax - 1.0f);
    float force            = settings->throwBaseForce * chargeMultiplier;

    RE::NiPoint3 impulse = {
        aimDir.x * force,
        aimDir.y * force,
        aimDir.z * force
    };

    HavokUtil::SetGravityFactor(body, 1.0f);  // restore gravity
    HavokUtil::SetLinearVelocity(body, impulse);

    obj.releaseSpeed = MathUtil::Length(impulse);
    obj.state        = HeldObjectState::Released;

    logger::debug("Throw: force={:.0f}, charge={:.2f}", force, obj.chargeLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// Drop — release without impulse; restore gravity
// ─────────────────────────────────────────────────────────────────────────────
void ObjectManipulation::Drop(HeldObjectData& obj) const {
    if (!obj.IsValid()) return;
    RestorePhysics(obj.Ref());
    obj.state = HeldObjectState::Released;
}

void ObjectManipulation::RestorePhysics(RE::TESObjectREFR* ref) const {
    auto* body = HavokUtil::GetRigidBody(ref);
    if (!body) return;
    HavokUtil::SetGravityFactor(body, 1.0f);
    HavokUtil::SetLinearVelocity(body, {});
}

// ─────────────────────────────────────────────────────────────────────────────
// CalcHoldPosition — polygon formation in front of player
//
//  1 object  → center (distance=holdDistance ahead of camera)
//  2 objects → left / right offset
//  3         → triangle (center + left/right)
//  4         → square corners
//  5         → pentagon
// ─────────────────────────────────────────────────────────────────────────────
RE::NiPoint3 ObjectManipulation::CalcHoldPosition(int slotIndex,
                                                   int totalCount,
                                                   bool vrLeftHand) const {
    auto* settings = Settings::GetSingleton();

    RE::NiPoint3 origin  = settings->vrEnabled
                               ? MathUtil::GetVRHandPosition(vrLeftHand)
                               : MathUtil::GetCameraPosition();
    RE::NiPoint3 forward = settings->vrEnabled
                               ? MathUtil::GetVRHandForward(vrLeftHand)
                               : MathUtil::GetCameraForward();

    // Base anchor = origin + holdDistance * forward
    RE::NiPoint3 anchor = {
        origin.x + forward.x * settings->holdDistance,
        origin.y + forward.y * settings->holdDistance,
        origin.z + forward.z * settings->holdDistance
    };

    if (totalCount <= 1) return anchor;

    // Build right and up vectors from forward
    RE::NiPoint3 worldUp = { 0.0f, 0.0f, 1.0f };
    RE::NiPoint3 right = MathUtil::Normalize({
         forward.y * worldUp.z - forward.z * worldUp.y,
        -forward.x * worldUp.z + forward.z * worldUp.x,
         forward.x * worldUp.y - forward.y * worldUp.x
    });
    RE::NiPoint3 up = MathUtil::Normalize({
        forward.y * right.z - forward.z * right.y,
       -forward.x * right.z + forward.z * right.x,
        forward.x * right.y - forward.y * right.x
    });

    // Distribute on a circle in the right-up plane
    float angle = (2.0f * std::numbers::pi_v<float> / static_cast<float>(totalCount))
                  * static_cast<float>(slotIndex);
    float r = settings->orbitRadius;

    return {
        anchor.x + right.x * std::cos(angle) * r + up.x * std::sin(angle) * r,
        anchor.y + right.y * std::cos(angle) * r + up.y * std::sin(angle) * r,
        anchor.z + right.z * std::cos(angle) * r + up.z * std::sin(angle) * r
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// AlignWeaponToAim — rotate weapon so its tip faces the aim direction
// ─────────────────────────────────────────────────────────────────────────────
void ObjectManipulation::AlignWeaponToAim(HeldObjectData& obj) const {
    if (!obj.IsValid()) return;
    auto* ref = obj.Ref();
    if (!WeaponOrientation::IsOrientable(ref)) return;

    RE::NiPoint3 aimDir = Settings::GetSingleton()->vrEnabled
                              ? MathUtil::GetVRHandForward(obj.vrLeftHand)
                              : MathUtil::GetCameraForward();

    WeaponOrientation::GetSingleton()->OrientToward(ref, aimDir);
}

// ─────────────────────────────────────────────────────────────────────────────
// RepackSlots — reassign consecutive slot indices after removal
// ─────────────────────────────────────────────────────────────────────────────
void ObjectManipulation::RepackSlots(std::vector<HeldObjectData>& objs) const {
    for (int i = 0; i < static_cast<int>(objs.size()); ++i) {
        objs[i].slotIndex = i;
    }
}
