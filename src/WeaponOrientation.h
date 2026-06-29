#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// WeaponOrientation — detects weapon types and applies rotation corrections
// so that the blade/tip always points toward the aim direction while held.
//
// Each weapon keyword maps to a local-space "forward" vector (the direction
// of the tip/edge in the weapon's model space). We compute the quaternion
// rotation that aligns that vector with the world aim direction.
// ─────────────────────────────────────────────────────────────────────────────
class WeaponOrientation {
public:
    static WeaponOrientation* GetSingleton();

    // Returns true if the ref is a weapon or arrow that should be oriented.
    static bool IsOrientable(RE::TESObjectREFR* ref);

    // Apply tip-forward rotation to the ref's rigid body.
    // aimDir: normalized world direction to aim the tip toward.
    void OrientToward(RE::TESObjectREFR* ref, const RE::NiPoint3& aimDir) const;

private:
    // Returns the local-space "tip forward" vector for this form type/keyword.
    RE::NiPoint3 GetTipVector(RE::TESObjectBOOK* /* unused */) const { return {}; }
    RE::NiPoint3 GetTipVector(RE::TESObjectWEAP* weap) const;
    RE::NiPoint3 GetTipVector(RE::TESAmmo* ammo) const;

    // Build rotation matrix that maps 'from' to 'to'.
    RE::NiMatrix3 RotationFromTo(const RE::NiPoint3& from, const RE::NiPoint3& to) const;
};
