#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// ObjectTargeting — finds the best object to grab.
//
// Strategy:
//   1. Cast a ray from the camera along its forward direction.
//   2. Collect all TESObjectREFRs within maxGrabDistance.
//   3. Score each by angle to crosshair direction; pick the closest within cone.
//   For VR, the "crosshair" direction is replaced by the hand forward vector.
// ─────────────────────────────────────────────────────────────────────────────
class ObjectTargeting {
public:
    static ObjectTargeting* GetSingleton();

    // Returns the best grab candidate, or nullptr if none found.
    // vrLeftHand: used in VR to select which controller to aim with.
    RE::TESObjectREFR* FindBestTarget(bool vrLeftHand = false) const;

    // Returns true if the ref is a valid grab candidate
    // (has collision, is not the player, not an actor, not already held).
    bool IsValidTarget(RE::TESObjectREFR* ref) const;

    // Returns true if the ref is a switch/activator (lever, button, etc.)
    bool IsActivator(RE::TESObjectREFR* ref) const;

private:
    // Score = cosine of angle between ref direction and aim direction.
    // Returns -1.0 if outside cone or beyond maxDistance.
    float ScoreTarget(RE::TESObjectREFR* ref,
                      const RE::NiPoint3& origin,
                      const RE::NiPoint3& aimDir,
                      float              maxDistSq,
                      float              minCosAngle) const;
};
