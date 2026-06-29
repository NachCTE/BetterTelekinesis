#include "InputHandler.h"
#include "TelekinesisManager.h"
#include "SwitchInteraction.h"
#include "ObjectTargeting.h"
#include "Settings.h"

InputHandler* InputHandler::GetSingleton() {
    static InputHandler instance;
    return &instance;
}

void InputHandler::Install() {
    auto* devMgr = RE::BSInputDeviceManager::GetSingleton();
    if (devMgr) {
        devMgr->AddEventSink(this);
        logger::info("InputHandler sink registered");
    }
}

RE::BSEventNotifyControl InputHandler::ProcessEvent(
    RE::InputEvent* const*                   a_events,
    RE::BSTEventSource<RE::InputEvent*>*     /* a_source */) {

    if (!a_events) return RE::BSEventNotifyControl::kContinue;

    auto* settings = Settings::GetSingleton();
    auto* mgr      = TelekinesisManager::GetSingleton();

    for (auto* event = *a_events; event; event = event->next) {
        auto* buttonEvent = event->AsButtonEvent();
        if (!buttonEvent) continue;

        uint32_t keyCode  = buttonEvent->GetIDCode();
        bool     pressed  = buttonEvent->IsDown();
        bool     released = buttonEvent->IsUp();
        bool     held     = buttonEvent->IsHeld();

        // ── Add object (G by default) ───────────────────────────────────────
        if (keyCode == settings->keyAddObject && pressed) {
            // Check if target is a switch first; if so, activate it
            auto* targeting = ObjectTargeting::GetSingleton();
            auto* target    = targeting->FindBestTarget(false);
            if (target && SwitchInteraction::GetSingleton()->TryActivate(target)) {
                // Switch activated — do not grab
            } else {
                mgr->AddObject(false);
            }
        }

        // ── Drop all (R by default) ─────────────────────────────────────────
        if (keyCode == settings->keyDropAll && pressed) {
            mgr->DropAll();
        }

        // ── Throw key — hold to charge, release to throw ────────────────────
        if (keyCode == settings->keyThrow && keyCode != 0xFF) {
            if (pressed) {
                OnThrowBegin();
            } else if (released && m_charging) {
                OnThrowRelease();
            }
        }

        // ── VR: left hand grab (same key, different hand) ───────────────────
        // In VR mode, holding Alt while pressing Add grabs with left hand.
        // This is a simplified approach; a full VR mod would use controller events.
        if (settings->vrEnabled && keyCode == settings->keyAddObject && pressed) {
            // Check modifier key state (Left Alt = 0x38 in DirectInput)
            auto* controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                // TODO: proper VR dual-hand logic via SKSE VR APIs
            }
        }
    }

    // Update charge timer if charging
    if (m_charging && mgr->IsHolding()) {
        // Time is tracked in TelekinesisManager via HeldObjectData::chargeLevel
    }

    return RE::BSEventNotifyControl::kContinue;
}

void InputHandler::OnAddObject() {
    TelekinesisManager::GetSingleton()->AddObject(false);
}

void InputHandler::OnDropAll() {
    TelekinesisManager::GetSingleton()->DropAll();
}

void InputHandler::OnThrowBegin() {
    if (!TelekinesisManager::GetSingleton()->IsHolding()) return;
    m_charging    = true;
    m_chargeTimer = 0.0f;
    TelekinesisManager::GetSingleton()->SetCharging(true);
    logger::debug("Throw charging started");
}

void InputHandler::OnThrowRelease() {
    m_charging = false;
    TelekinesisManager::GetSingleton()->SetCharging(false);

    // chargeLevel is accumulated per-frame in TelekinesisManager
    float chargeLevel = std::clamp(m_chargeTimer / Settings::GetSingleton()->throwChargeTime,
                                   0.0f, 1.0f);
    TelekinesisManager::GetSingleton()->ThrowAll(chargeLevel);
    m_chargeTimer = 0.0f;
    logger::debug("Throw released: charge={:.2f}", chargeLevel);
}

void InputHandler::OnActivateSwitch() {
    auto* target = ObjectTargeting::GetSingleton()->FindBestTarget(false);
    SwitchInteraction::GetSingleton()->TryActivate(target);
}
