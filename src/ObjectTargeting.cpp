#include "ObjectTargeting.h"
#include "TelekinesisManager.h"
#include "Settings.h"
#include "Util.h"
#include <cmath>

ObjectTargeting* ObjectTargeting::GetSingleton() {
    static ObjectTargeting instance;
    return &instance;
}

RE::TESObjectREFR* ObjectTargeting::FindBestTarget(bool vrLeftHand) const {
    auto* settings = Settings::GetSingleton();
    auto* player   = RE::PlayerCharacter::GetSingleton();
    if (!player) return nullptr;

    RE::NiPoint3 origin  = settings->vrEnabled
                               ? MathUtil::GetVRHandPosition(vrLeftHand)
                               : MathUtil::GetCameraPosition();
    RE::NiPoint3 aimDir  = settings->vrEnabled
                               ? MathUtil::GetVRHandForward(vrLeftHand)
                               : MathUtil::GetCameraForward();

    float coneRad    = settings->vrEnabled ? settings->vrAimConeHalfDeg : settings->coneHalfAngleDeg;
    float minCos     = std::cos(coneRad * (3.14159265f / 180.0f));
    float maxDistSq  = settings->maxGrabDistance * settings->maxGrabDistance;

    RE::TESObjectREFR* best      = nullptr;
    float              bestScore = -2.0f;

    // Iterate all object references in the current cell and surrounding cells.
    auto* cell = player->GetParentCell();
    if (!cell) return nullptr;

    auto ProcessRef = [&](RE::TESObjectREFR& ref) {
        if (!IsValidTarget(&ref)) return;
        float score = ScoreTarget(&ref, origin, aimDir, maxDistSq, minCos);
        if (score > bestScore) {
            bestScore = score;
            best      = &ref;
        }
    };

    // Iterate references in the current cell
    for (auto& entry : cell->references) {
        if (entry) ProcessRef(*entry);
    }

    return best;
}

bool ObjectTargeting::IsValidTarget(RE::TESObjectREFR* ref) const {
    if (!ref) return false;
    if (ref->IsDisabled() || ref->IsDeleted()) return false;
    if (ref->Is(RE::FormType::ActorCharacter)) return false; // skip actors
    if (ref == RE::PlayerCharacter::GetSingleton()) return false;

    // Must have a 3D node with collision (grabbable by physics)
    auto* root = ref->Get3D();
    if (!root) return false;
    if (!root->collisionObject) return false;

    // Skip if already held
    auto* mgr = TelekinesisManager::GetSingleton();
    // We'll do a handle comparison indirectly — if we can find it in the held list.
    // For now, rely on TelekinesisManager to reject duplicates in AddObject.

    return true;
}

bool ObjectTargeting::IsActivator(RE::TESObjectREFR* ref) const {
    return SwitchInteraction::IsSwitch(ref);
}

float ObjectTargeting::ScoreTarget(RE::TESObjectREFR* ref,
                                   const RE::NiPoint3& origin,
                                   const RE::NiPoint3& aimDir,
                                   float               maxDistSq,
                                   float               minCosAngle) const {
    auto* root = ref->Get3D();
    if (!root) return -2.0f;

    RE::NiPoint3 refPos  = root->world.translate;
    RE::NiPoint3 toRef   = { refPos.x - origin.x, refPos.y - origin.y, refPos.z - origin.z };
    float        distSq  = toRef.x * toRef.x + toRef.y * toRef.y + toRef.z * toRef.z;

    if (distSq > maxDistSq) return -2.0f;

    RE::NiPoint3 toRefN  = MathUtil::Normalize(toRef);
    float        cosAng  = MathUtil::Dot(toRefN, aimDir);

    if (cosAng < minCosAngle) return -2.0f;

    // Score favors objects close to the center of the cone (higher cosine).
    return cosAng;
}
