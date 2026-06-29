#include "SwitchInteraction.h"

SwitchInteraction* SwitchInteraction::GetSingleton() {
    static SwitchInteraction instance;
    return &instance;
}

bool SwitchInteraction::TryActivate(RE::TESObjectREFR* ref) const {
    if (!IsSwitch(ref)) return false;
    SendActivate(ref);
    logger::info("TelekinesisActivate: {}", ref->GetName());
    return true;
}

bool SwitchInteraction::IsSwitch(RE::TESObjectREFR* ref) {
    if (!ref) return false;

    // Check if the base form is an Activator (but not a door — doors require
    // proximity or keys and shouldn't be triggered by telekinesis).
    auto* base = ref->GetBaseObject();
    if (!base) return false;

    if (base->Is(RE::FormType::Activator)) {
        // Exclude doors (DOOR form type)
        return !base->Is(RE::FormType::Door);
    }

    return false;
}

void SwitchInteraction::SendActivate(RE::TESObjectREFR* ref) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !ref) return;

    // Use the game's Activate virtual function so scripts and animations fire.
    ref->ActivateRef(player, 0, nullptr, 0, false);
}
