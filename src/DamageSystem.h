#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// DamageSystem — tracks thrown objects and applies physics-based damage when
// they collide with actors.
//
// Approach:
//   - When an object is thrown, it is registered here with its release velocity.
//   - Each frame we check if the object's speed has dropped significantly
//     (collision detected via velocity change).
//   - On collision, we find nearby actors and apply damage:
//       damage = clamp(mass * speed² * scalar, minDmg, maxDmg)
//   - Heavy objects (mass > threshold) also apply stagger.
//   - Friendly fire toggle: skip actors in the player's team if disabled.
// ─────────────────────────────────────────────────────────────────────────────
class DamageSystem {
public:
    static DamageSystem* GetSingleton();

    // Register a thrown object. releaseVelocity in Skyrim units/sec.
    void RegisterThrown(RE::TESObjectREFR* ref, const RE::NiPoint3& releaseVelocity);

    // Per-frame update: detect collisions and apply damage.
    void Update(float dt);

    // Remove a reference from tracking (e.g., object was picked up).
    void Unregister(RE::TESObjectREFR* ref);

private:
    struct ThrownObject {
        RE::ObjectRefHandle handle;
        RE::NiPoint3        lastVelocity;
        float               lastSpeed{ 0.0f };
        float               timeAlive{ 0.0f };
        static constexpr float kMaxTrackTime = 10.0f; // Stop tracking after N seconds
    };

    std::vector<ThrownObject> m_tracked;

    void ApplyImpactDamage(RE::TESObjectREFR* ref,
                           RE::bhkRigidBody*  body,
                           float              impactSpeed,
                           float              mass) const;

    // Find the actor closest to pos within radius; returns nullptr if none.
    RE::Actor* FindNearestActor(const RE::NiPoint3& pos, float radius) const;

    bool ShouldDamageActor(RE::Actor* actor) const;
};
