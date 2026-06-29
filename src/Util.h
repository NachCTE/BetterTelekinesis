#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// Havok utility helpers shared across systems.
// Havok uses a compressed unit scale relative to Skyrim game units.
// kHavokScale ≈ 1/70: pos_havok = pos_skyrim * kHavokScale
// ─────────────────────────────────────────────────────────────────────────────
namespace HavokUtil {

    constexpr float kHavokScale    = 1.0f / 70.0f;
    constexpr float kHavokScaleInv = 70.0f;

    RE::hkVector4  ToHavok(const RE::NiPoint3& v);
    RE::NiPoint3   FromHavok(const RE::hkVector4& v);

    RE::bhkRigidBody* GetRigidBody(RE::TESObjectREFR* ref);

    RE::NiPoint3   GetPosition(RE::bhkRigidBody* body);
    RE::NiPoint3   GetLinearVelocity(RE::bhkRigidBody* body);
    void           SetLinearVelocity(RE::bhkRigidBody* body, const RE::NiPoint3& vel);
    void           ApplyLinearImpulse(RE::bhkRigidBody* body, const RE::NiPoint3& impulse);
    float          GetMass(RE::bhkRigidBody* body);
    // Gravity: rather than toggling gravityFactor (hkHalf), we apply a
    // compensating upward impulse each frame via the spring-damper when holding.

}  // namespace HavokUtil

// ─────────────────────────────────────────────────────────────────────────────
// Math helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace MathUtil {

    RE::NiPoint3 Normalize(const RE::NiPoint3& v);
    float        Length(const RE::NiPoint3& v);
    float        Dot(const RE::NiPoint3& a, const RE::NiPoint3& b);
    RE::NiPoint3 Clamp(const RE::NiPoint3& v, float maxLen);

    // Camera / player aim direction (works in both AE and VR-handed mode)
    RE::NiPoint3 GetCameraForward();
    RE::NiPoint3 GetCameraPosition();

    // VR hand forward direction (isLeft: true = left hand)
    RE::NiPoint3 GetVRHandForward(bool isLeft);
    RE::NiPoint3 GetVRHandPosition(bool isLeft);

}  // namespace MathUtil
