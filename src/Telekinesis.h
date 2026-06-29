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

    bool IsSpellActive() const;
    void TryGrab();
    void Hold(float dt);
    void Throw();
    void Release();

    RE::TESObjectREFR* FindTarget() const;

    static RE::NiPoint3      CameraPos();
    static RE::NiPoint3      CameraForward();
    static RE::bhkRigidBody* GetBody(RE::TESObjectREFR* ref);
    static RE::hkVector4     ToHavok(const RE::NiPoint3& v);

    static constexpr float kHoldDist     = 150.0f;
    static constexpr float kMaxDist      = 2000.0f;
    static constexpr float kConeHalfDeg  = 25.0f;
    static constexpr float kThrowForce   = 3500.0f;
    static constexpr float kHavokScale   = 1.0f / 70.0f;
    static constexpr float kPullStrength = 8.0f;    // spring constant (velocity = dist * k)
    static constexpr float kMaxPullSpeed = 1500.0f; // cap in Skyrim units/s

    struct Hook;
};
