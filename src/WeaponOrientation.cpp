#include "WeaponOrientation.h"
#include "Util.h"
#include <cmath>

WeaponOrientation* WeaponOrientation::GetSingleton() {
    static WeaponOrientation instance;
    return &instance;
}

bool WeaponOrientation::IsOrientable(RE::TESObjectREFR* ref) {
    if (!ref) return false;
    auto* base = ref->GetBaseObject();
    if (!base) return false;
    return base->Is(RE::FormType::Weapon) || base->Is(RE::FormType::Ammo);
}

// ─────────────────────────────────────────────────────────────────────────────
// GetTipVector — local-space direction of tip/edge for each weapon type.
// These are approximate canonical values; the exact model offsets may vary
// per weapon. Fine-tuning per specific weapon is possible via MCM later.
//
// Skyrim local weapon space (typical): blade tip is along +Y or +X axis.
// ─────────────────────────────────────────────────────────────────────────────
RE::NiPoint3 WeaponOrientation::GetTipVector(RE::TESObjectWEAP* weap) const {
    if (!weap) return { 0.0f, 1.0f, 0.0f };

    using WT = RE::WEAPON_TYPE;
    switch (weap->GetWeaponType()) {
    case WT::kSword:
    case WT::kGreatSword:
    case WT::kDagger:
        return { 0.0f, 1.0f, 0.0f };  // tip along local +Y

    case WT::kWarAxe:
    case WT::kBattleAxe:
        return { 1.0f, 0.0f, 0.0f };  // blade edge along local +X

    case WT::kMace:
    case WT::kWarhammer:
        return { 0.0f, 1.0f, 0.0f };  // head along +Y

    case WT::kBow:
    case WT::kCrossbow:
        return { 0.0f, 1.0f, 0.0f };  // arrow notch end

    case WT::kStaff:
        return { 0.0f, 1.0f, 0.0f };

    default:
        return { 0.0f, 1.0f, 0.0f };
    }
}

RE::NiPoint3 WeaponOrientation::GetTipVector(RE::TESAmmo* /* ammo */) const {
    return { 0.0f, 1.0f, 0.0f };  // arrow tip along +Y
}

// ─────────────────────────────────────────────────────────────────────────────
// RotationFromTo — build a rotation matrix that maps 'from' to 'to'.
// Uses Rodrigues' rotation formula.
// ─────────────────────────────────────────────────────────────────────────────
RE::NiMatrix3 WeaponOrientation::RotationFromTo(const RE::NiPoint3& from,
                                                 const RE::NiPoint3& to) const {
    RE::NiPoint3 f = MathUtil::Normalize(from);
    RE::NiPoint3 t = MathUtil::Normalize(to);

    float cosA = MathUtil::Dot(f, t);
    cosA = std::clamp(cosA, -1.0f, 1.0f);

    RE::NiMatrix3 mat;

    if (cosA > 0.9999f) {
        // Already aligned — identity
        mat.MakeIdentity();
        return mat;
    }

    if (cosA < -0.9999f) {
        // 180° rotation: find an orthogonal axis
        RE::NiPoint3 perp = (std::abs(f.x) < 0.9f)
                                ? RE::NiPoint3{ 1.0f, 0.0f, 0.0f }
                                : RE::NiPoint3{ 0.0f, 1.0f, 0.0f };
        // Cross product
        RE::NiPoint3 axis = MathUtil::Normalize({
            f.y * perp.z - f.z * perp.y,
           -f.x * perp.z + f.z * perp.x,
            f.x * perp.y - f.y * perp.x
        });
        float s = 0.0f, c = -1.0f;
        // Rodrigues for 180°
        mat.entry[0][0] = 2.0f * axis.x * axis.x - 1.0f;
        mat.entry[0][1] = 2.0f * axis.x * axis.y;
        mat.entry[0][2] = 2.0f * axis.x * axis.z;
        mat.entry[1][0] = 2.0f * axis.y * axis.x;
        mat.entry[1][1] = 2.0f * axis.y * axis.y - 1.0f;
        mat.entry[1][2] = 2.0f * axis.y * axis.z;
        mat.entry[2][0] = 2.0f * axis.z * axis.x;
        mat.entry[2][1] = 2.0f * axis.z * axis.y;
        mat.entry[2][2] = 2.0f * axis.z * axis.z - 1.0f;
        return mat;
    }

    // Rodrigues' rotation: axis = normalize(from × to)
    RE::NiPoint3 axis = MathUtil::Normalize({
        f.y * t.z - f.z * t.y,
       -f.x * t.z + f.z * t.x,
        f.x * t.y - f.y * t.x
    });
    float sinA = std::sqrt(1.0f - cosA * cosA);
    float ic   = 1.0f - cosA;

    mat.entry[0][0] = cosA + axis.x * axis.x * ic;
    mat.entry[0][1] = axis.x * axis.y * ic - axis.z * sinA;
    mat.entry[0][2] = axis.x * axis.z * ic + axis.y * sinA;
    mat.entry[1][0] = axis.y * axis.x * ic + axis.z * sinA;
    mat.entry[1][1] = cosA + axis.y * axis.y * ic;
    mat.entry[1][2] = axis.y * axis.z * ic - axis.x * sinA;
    mat.entry[2][0] = axis.z * axis.x * ic - axis.y * sinA;
    mat.entry[2][1] = axis.z * axis.y * ic + axis.x * sinA;
    mat.entry[2][2] = cosA + axis.z * axis.z * ic;

    return mat;
}

// ─────────────────────────────────────────────────────────────────────────────
// OrientToward — apply rotation to the ref's NiNode so tip points at aimDir
// ─────────────────────────────────────────────────────────────────────────────
void WeaponOrientation::OrientToward(RE::TESObjectREFR* ref,
                                      const RE::NiPoint3& aimDir) const {
    if (!ref) return;

    auto* root = ref->Get3D();
    if (!root) return;

    auto* base = ref->GetBaseObject();
    if (!base) return;

    RE::NiPoint3 tipVec;
    if (auto* weap = base->As<RE::TESObjectWEAP>()) {
        tipVec = GetTipVector(weap);
    } else if (auto* ammo = base->As<RE::TESAmmo>()) {
        tipVec = GetTipVector(ammo);
    } else {
        return;
    }

    RE::NiMatrix3 rot = RotationFromTo(tipVec, aimDir);

    // Apply to root node's local rotation
    root->local.rotate = rot;
    RE::NiUpdateData updateData;
    updateData.flags.set(RE::NiUpdateData::Flag::kDirty);
    root->Update(updateData);

    // Also update Havok rigid body rotation
    auto* body = HavokUtil::GetRigidBody(ref);
    if (body) {
        auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
        if (hkBody) {
            // Convert NiMatrix3 to hkRotation
            RE::hkMatrix3 hkRot;
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    hkRot.col[c].quad.m128_f32[r] = rot.entry[r][c];
                }
            }
            hkBody->motion.motionState.transform.rotation = hkRot;
        }
    }
}
