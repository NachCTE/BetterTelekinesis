#pragma once
#include "PCH.h"
#include "ObjectManipulation.h"

// ─────────────────────────────────────────────────────────────────────────────
// TelekinesisManager — central coordinator for the Ultimate Telekinesis spell.
//
// Lifecycle:
//   Initialize()     → called on DataLoaded; registers PlayerCharacter update hook.
//   AddObject(ref)   → grab a ref and add it to the held list.
//   RemoveObject(ref)→ drop a specific ref.
//   DropAll()        → drop all refs without throwing.
//   ThrowAll()       → throw all held refs; chargeLevel from InputHandler.
//   Update(dt)       → called every frame; handles physics, magicka drain,
//                      orbit shield, and perk gate checks.
//
// Perk gates (Alteration skill):
//   Skill 0–24:  max 1 object, no damage.
//   Skill 25–49: max 2 objects, damage enabled.
//   Skill 50–74: max 3 objects, orbit/shield enabled.
//   Skill 75–99: max 4 objects, charged throw enabled.
//   Skill 100:   max 5 objects.
// ─────────────────────────────────────────────────────────────────────────────
class TelekinesisManager {
public:
    static TelekinesisManager* GetSingleton();

    void Initialize();

    // Add the nearest valid object to the hold list.
    // vrLeftHand: in VR, grab with left hand instead of right.
    void AddObject(bool vrLeftHand = false);

    // Remove a specific object (drop without throw).
    void RemoveObject(RE::TESObjectREFR* ref);

    void DropAll();
    void ThrowAll(float chargeLevel);

    // Per-frame update; called from the PlayerCharacter::Update hook.
    void Update(float dt);

    bool IsHolding() const { return !m_held.empty(); }
    int  HeldCount() const { return static_cast<int>(m_held.size()); }

    // Returns the effective max objects allowed by current Alteration skill.
    int  GetPerkMaxObjects() const;

    // Called by InputHandler when charged throw state changes.
    void SetCharging(bool charging);

private:
    std::vector<HeldObjectData> m_held;

    void DrainMagicka(float dt);
    void UpdateOrbitShield();
    void CheckProjectileIntercepts();

    // Hook on PlayerCharacter::Update (installed once in Initialize).
    struct UpdateHook;
};
