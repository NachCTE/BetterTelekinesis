#include "Util.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// HavokUtil
// ─────────────────────────────────────────────────────────────────────────────
namespace HavokUtil {

RE::hkVector4 ToHavok(const RE::NiPoint3& v) {
    RE::hkVector4 hk;
    hk.quad.m128_f32[0] = v.x * kHavokScale;
    hk.quad.m128_f32[1] = v.y * kHavokScale;
    hk.quad.m128_f32[2] = v.z * kHavokScale;
    hk.quad.m128_f32[3] = 0.0f;
    return hk;
}

RE::NiPoint3 FromHavok(const RE::hkVector4& v) {
    return { v.quad.m128_f32[0] * kHavokScaleInv,
             v.quad.m128_f32[1] * kHavokScaleInv,
             v.quad.m128_f32[2] * kHavokScaleInv };
}

RE::bhkRigidBody* GetRigidBody(RE::TESObjectREFR* ref) {
    if (!ref) return nullptr;
    auto* root = ref->Get3D();
    if (!root) return nullptr;
    auto* colObj = root->collisionObject.get();
    if (!colObj) return nullptr;
    return static_cast<RE::bhkRigidBody*>(colObj->GetRigidBody());
}

RE::NiPoint3 GetPosition(RE::bhkRigidBody* body) {
    if (!body) return {};
    auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
    if (!hkBody) return {};
    return FromHavok(hkBody->motion.motionState.transform.translation);
}

RE::NiPoint3 GetLinearVelocity(RE::bhkRigidBody* body) {
    if (!body) return {};
    auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
    if (!hkBody) return {};
    return FromHavok(hkBody->motion.linearVelocity);
}

void SetLinearVelocity(RE::bhkRigidBody* body, const RE::NiPoint3& vel) {
    if (!body) return;
    auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
    if (!hkBody) return;
    hkBody->motion.linearVelocity = ToHavok(vel);
}

void ApplyLinearImpulse(RE::bhkRigidBody* body, const RE::NiPoint3& impulse) {
    if (!body) return;
    auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
    if (!hkBody) return;
    auto current = hkBody->motion.linearVelocity;
    auto add      = ToHavok(impulse);
    current.quad.m128_f32[0] += add.quad.m128_f32[0];
    current.quad.m128_f32[1] += add.quad.m128_f32[1];
    current.quad.m128_f32[2] += add.quad.m128_f32[2];
    hkBody->motion.linearVelocity = current;
}

float GetMass(RE::bhkRigidBody* body) {
    if (!body) return 0.0f;
    auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
    if (!hkBody) return 0.0f;
    float invMass = hkBody->motion.inertiaAndMassInv.quad.m128_f32[3];
    return (invMass > 1e-6f) ? (1.0f / invMass) : 0.0f;
}

void SetGravityFactor(RE::bhkRigidBody* body, float factor) {
    if (!body) return;
    auto* hkBody = static_cast<RE::hkpRigidBody*>(body->referencedObject.get());
    if (!hkBody) return;
    hkBody->motion.gravityFactor = factor;
}

}  // namespace HavokUtil

// ─────────────────────────────────────────────────────────────────────────────
// MathUtil
// ─────────────────────────────────────────────────────────────────────────────
namespace MathUtil {

float Length(const RE::NiPoint3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

RE::NiPoint3 Normalize(const RE::NiPoint3& v) {
    float len = Length(v);
    if (len < 1e-6f) return { 0.0f, 0.0f, 1.0f };
    return { v.x / len, v.y / len, v.z / len };
}

float Dot(const RE::NiPoint3& a, const RE::NiPoint3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

RE::NiPoint3 Clamp(const RE::NiPoint3& v, float maxLen) {
    float len = Length(v);
    if (len <= maxLen || len < 1e-6f) return v;
    float s = maxLen / len;
    return { v.x * s, v.y * s, v.z * s };
}

RE::NiPoint3 GetCameraPosition() {
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!camera || !camera->cameraRoot) return {};
    const auto& pos = camera->cameraRoot->world.translate;
    return pos;
}

RE::NiPoint3 GetCameraForward() {
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!camera || !camera->cameraRoot) {
        // Fallback: use player rotation
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return { 0.0f, 1.0f, 0.0f };
        float yaw   = player->data.angle.z;
        float pitch = player->data.angle.x;
        return {
            std::sin(yaw) * std::cos(pitch),
            std::cos(yaw) * std::cos(pitch),
            -std::sin(pitch)
        };
    }

    // NiMatrix3 row 1 = local Y axis in world space = forward vector
    const auto& rot = camera->cameraRoot->world.rotate;
    return Normalize({ rot.entry[1][0], rot.entry[1][1], rot.entry[1][2] });
}

RE::NiPoint3 GetVRHandForward(bool isLeft) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return GetCameraForward();

    const char* nodeName = isLeft ? "NPC L Hand [LHnd]" : "NPC R Hand [RHnd]";
    auto* node = player->GetNodeByName(nodeName);
    if (!node) return GetCameraForward();

    // Hand node's forward = local Y in world space
    const auto& rot = node->world.rotate;
    return Normalize({ rot.entry[1][0], rot.entry[1][1], rot.entry[1][2] });
}

RE::NiPoint3 GetVRHandPosition(bool isLeft) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return GetCameraPosition();

    const char* nodeName = isLeft ? "NPC L Hand [LHnd]" : "NPC R Hand [RHnd]";
    auto* node = player->GetNodeByName(nodeName);
    if (!node) return GetCameraPosition();

    return node->world.translate;
}

}  // namespace MathUtil
