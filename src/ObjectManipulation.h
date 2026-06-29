#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// State of a single held object in the telekinesis manager.
// ─────────────────────────────────────────────────────────────────────────────
enum class HeldObjectState : uint8_t {
    Pulling,   // Being attracted toward the hold position at high speed
    Holding,   // Held with spring-damper physics; inercia style
    Charging,  // Holding + throw button is held; charge accumulating
    Released,  // Just thrown; tracked briefly for damage calculation
};

struct HeldObjectData {
    RE::ObjectRefHandle handle;       // Weak handle to the ref
    HeldObjectState     state{ HeldObjectState::Pulling };
    int                 slotIndex{ 0 };   // Index in the hold formation [0..4]
    float               chargeLevel{ 0.0f }; // [0,1] for charged throw
    float               releaseSpeed{ 0.0f }; // Recorded speed at throw for damage
    bool                vrLeftHand{ false };   // VR: which hand grabbed this

    RE::TESObjectREFR* Ref() const;
    bool               IsValid() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ObjectManipulation — low-level physics operations on held objects.
// Called every frame by TelekinesisManager::Update().
// ─────────────────────────────────────────────────────────────────────────────
class ObjectManipulation {
public:
    static ObjectManipulation* GetSingleton();

    // Begin pulling a ref toward the hold position.
    // Disables gravity; sets initial high-speed velocity.
    void BeginPull(RE::TESObjectREFR* ref) const;

    // Per-frame: apply spring-damper force to keep object at target position.
    // dt: frame delta time in seconds.
    void UpdateHold(HeldObjectData& obj, float dt) const;

    // Throw the object in the camera forward direction.
    // chargeLevel [0,1] scales the final impulse.
    void Throw(HeldObjectData& obj) const;

    // Drop the object without applying any impulse; restore gravity.
    void Drop(HeldObjectData& obj) const;

    // Restore physics state (gravity, velocity) of a ref.
    void RestorePhysics(RE::TESObjectREFR* ref) const;

    // Calculate world-space hold position for slotIndex within totalCount objects.
    // Objects are arranged in a polygon pattern in front of the player.
    RE::NiPoint3 CalcHoldPosition(int slotIndex, int totalCount, bool vrLeftHand = false) const;

    // Rotate weapon-type objects so their tip/edge faces the aim direction.
    void AlignWeaponToAim(HeldObjectData& obj) const;

    // Reassign slot indices after an object is removed.
    void RepackSlots(std::vector<HeldObjectData>& objs) const;

private:
    // Returns the hold anchor point (position + distance offset along aim dir).
    RE::NiPoint3 AnchorPoint(bool vrLeftHand = false) const;
};
