#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// InputHandler — processes keyboard/gamepad input for telekinesis hotkeys.
//
// Actions:
//   kAddObject   — grab nearest object to crosshair and add to hold list.
//                  If already at maxObjects, replaces the oldest held object.
//   kDropAll     — drop all held objects without throwing.
//   kThrowHeld   — throw all (or selected) held objects.
//                  Hold the key to charge; release to throw at charged force.
//   kActivateSwitch — attempt to telekinetically activate switch at crosshair.
//
// All scan codes are configurable via Settings / MCM.
// ─────────────────────────────────────────────────────────────────────────────
class InputHandler : public RE::BSTEventSink<RE::InputEvent*> {
public:
    static InputHandler* GetSingleton();

    // Register this sink with the input device manager.
    void Install();

    // RE::BSTEventSink<RE::InputEvent*>
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events,
                                          RE::BSTEventSource<RE::InputEvent*>* a_source) override;

private:
    // Charging state for charged throw
    bool  m_charging{ false };
    float m_chargeTimer{ 0.0f };

    void OnAddObject();
    void OnDropAll();
    void OnThrowBegin();
    void OnThrowRelease();
    void OnActivateSwitch();
};
