#pragma once
#include "PCH.h"
#include "ObjectManipulation.h"

class TelekinesisManager {
public:
    static TelekinesisManager* GetSingleton();

    void Initialize();

    void AddObject(bool vrLeftHand = false);
    void RemoveObject(RE::TESObjectREFR* ref);
    void DropAll();
    void ThrowAll(float chargeLevel);
    void SetCharging(bool charging);

    void Update(float dt);

    bool IsHolding() const { return !m_held.empty(); }
    int  HeldCount() const { return static_cast<int>(m_held.size()); }

    int GetPerkMaxObjects() const;

private:
    std::vector<HeldObjectData> m_held;

    RE::SpellItem* m_spell          = nullptr;
    bool           m_wasSpellActive = false;
    float          m_castHoldTime   = 0.0f;

    bool IsSpellActive() const;

    void DrainMagicka(float dt);
    void CheckProjectileIntercepts();

    struct UpdateHook;
};
