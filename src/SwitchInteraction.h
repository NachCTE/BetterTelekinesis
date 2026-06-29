#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// SwitchInteraction — detects and activates levers, buttons, and other
// Activator references via telekinesis.
//
// Two cases:
//   1. The targeted object IS an activator → activate it directly.
//   2. The targeted object is not an activator → check if a collidable
//      activator is nearby the targeted position and activate that.
// ─────────────────────────────────────────────────────────────────────────────
class SwitchInteraction {
public:
    static SwitchInteraction* GetSingleton();

    // Attempt to telekinetically activate the ref.
    // Returns true if the ref was identified as an activator and activated.
    bool TryActivate(RE::TESObjectREFR* ref) const;

    // Returns true if ref is an activatable switch/lever/button (not a door).
    static bool IsSwitch(RE::TESObjectREFR* ref);

private:
    // Send an Activate event from the player to ref.
    static void SendActivate(RE::TESObjectREFR* ref);
};
