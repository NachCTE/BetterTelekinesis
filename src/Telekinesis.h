#pragma once
#include "PCH.h"

class Telekinesis
{
public:
    static Telekinesis* GetSingleton();

    void Initialize(RE::SpellItem* spell);
    void Update(float dt);

private:
    RE::SpellItem*      m_spell     = nullptr;
    RE::ObjectRefHandle m_held      = {};
    bool                m_wasActive = false;
    float               m_holdTime  = 0.0f;

    // Projectile tracking for damage
    RE::ObjectRefHandle m_projectile     = {};
    float               m_projectileTime = 0.0f;
    float               m_throwSpeed     = 0.0f;

    bool IsSpellActive() const;
    void TryGrab();
    void Hold(float dt);
    void Throw();
    void Release();
    void TrackProjectile(float dt);

    RE::TESObjectREFR* FindTarget() const;

    static RE::NiPoint3      CameraPos();
    static RE::NiPoint3      CameraForward();
    static RE::bhkRigidBody* GetBody(RE::TESObjectREFR* ref);
    static RE::hkVector4     ToHavok(const RE::NiPoint3& v);
    static float             GetMass(RE::TESObjectREFR* ref);
    static float             GetSpeed(RE::TESObjectREFR* ref);

    static constexpr float kHoldDist        = 150.0f;
    static constexpr float kMaxDist         = 2000.0f;
    static constexpr float kConeHalfDeg     = 25.0f;
    static constexpr float kThrowForce      = 3500.0f;
    static constexpr float kHavokScale      = 1.0f / 70.0f;
    static constexpr float kPullStrength    = 8.0f;
    static constexpr float kMaxPullSpeed    = 1500.0f;
    static constexpr float kDamageFactor    = 0.0012f; // tunable: dmg = mass * speed * factor
    static constexpr float kImpactRadius    = 80.0f;   // units to consider a hit
    static constexpr float kMinImpactSpeed  = 300.0f;  // minimum speed to deal damage
    static constexpr float kTrackDuration   = 4.0f;    // seconds to track after throw

    struct Hook;
};
