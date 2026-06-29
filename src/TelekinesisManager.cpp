#include "TelekinesisManager.h"
#include "ObjectTargeting.h"
#include "ObjectManipulation.h"
#include "DamageSystem.h"
#include "SwitchInteraction.h"
#include "Settings.h"
#include "Util.h"
#include <REL/Relocation.h>

// ─────────────────────────────────────────────────────────────────────────────
// PlayerCharacter::Update hook — fires every game frame
// ─────────────────────────────────────────────────────────────────────────────
struct TelekinesisManager::UpdateHook {
    static void Thunk(RE::PlayerCharacter* player, float dt) {
        TelekinesisManager::GetSingleton()->Update(dt);
        DamageSystem::GetSingleton()->Update(dt);
        func(player, dt);
    }

    static inline REL::Relocation<decltype(Thunk)*> func;

    static void Install() {
        // PlayerCharacter::Update — vtable index 0xAD (173)
        // This offset is valid for AE 1.6.x and VR.
        // Use Address Library ID for correct relocation.
        REL::Relocation<std::uintptr_t> vTable{ RE::VTABLE_PlayerCharacter[0] };
        func   = vTable.write_vfunc(0xAD, Thunk);
        logger::info("TelekinesisManager::UpdateHook installed (vtbl+0xAD)");
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
TelekinesisManager* TelekinesisManager::GetSingleton() {
    static TelekinesisManager instance;
    return &instance;
}

void TelekinesisManager::Initialize() {
    UpdateHook::Install();

    // Look up the spell by EditorID so we can detect when it's being cast.
    m_spell = RE::TESForm::LookupByEditorID<RE::SpellItem>("BetterTelekinesisSpell");
    if (m_spell) {
        logger::info("TelekinesisManager: found spell {:08X}", m_spell->GetFormID());
    } else {
        logger::warn("TelekinesisManager: BetterTelekinesisSpell not found — check ESP is loaded!");
    }

    logger::info("TelekinesisManager initialized");
}

// ─────────────────────────────────────────────────────────────────────────────
// AddObject
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::AddObject(bool vrLeftHand) {
    auto* settings = Settings::GetSingleton();
    int   maxObj   = std::min(settings->maxObjects, GetPerkMaxObjects());

    if (static_cast<int>(m_held.size()) >= maxObj) {
        logger::debug("AddObject: already at max ({}/{})", m_held.size(), maxObj);
        // Remove the oldest held object to make room
        if (!m_held.empty()) {
            ObjectManipulation::GetSingleton()->Drop(m_held.front());
            m_held.erase(m_held.begin());
            ObjectManipulation::GetSingleton()->RepackSlots(m_held);
        }
    }

    auto* target = ObjectTargeting::GetSingleton()->FindBestTarget(vrLeftHand);
    if (!target) {
        logger::debug("AddObject: no valid target found");
        return;
    }

    // Check for duplicate
    for (auto& obj : m_held) {
        if (obj.handle.get().get() == target) return;  // already held
    }

    HeldObjectData data;
    data.handle     = target->GetHandle();
    data.state      = HeldObjectState::Pulling;
    data.slotIndex  = static_cast<int>(m_held.size());
    data.vrLeftHand = vrLeftHand;

    ObjectManipulation::GetSingleton()->BeginPull(target);
    m_held.push_back(data);

    logger::info("AddObject: grabbed '{}' (slot {})", target->GetName(), data.slotIndex);
}

// ─────────────────────────────────────────────────────────────────────────────
// RemoveObject
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::RemoveObject(RE::TESObjectREFR* ref) {
    auto it = std::find_if(m_held.begin(), m_held.end(), [ref](const HeldObjectData& d) {
        return d.handle.get().get() == ref;
    });

    if (it == m_held.end()) return;

    ObjectManipulation::GetSingleton()->Drop(*it);
    m_held.erase(it);
    ObjectManipulation::GetSingleton()->RepackSlots(m_held);
}

// ─────────────────────────────────────────────────────────────────────────────
// DropAll — drop everything without throwing
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::DropAll() {
    auto* manip = ObjectManipulation::GetSingleton();
    for (auto& obj : m_held) {
        manip->Drop(obj);
    }
    m_held.clear();
    logger::info("DropAll: all objects released");
}

// ─────────────────────────────────────────────────────────────────────────────
// ThrowAll — throw all held objects with given charge level
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::ThrowAll(float chargeLevel) {
    auto* manip   = ObjectManipulation::GetSingleton();
    auto* dmg     = DamageSystem::GetSingleton();
    auto* settings = Settings::GetSingleton();

    int alterationSkill = static_cast<int>(
        RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(RE::ActorValue::kAlteration));

    // Charged throw requires Alteration >= perkLevelCharge
    if (chargeLevel > 0.0f && alterationSkill < settings->perkLevelCharge) {
        chargeLevel = 0.0f;
    }

    for (auto& obj : m_held) {
        obj.chargeLevel = chargeLevel;
        if (!obj.IsValid()) continue;

        // Record velocity before throw for damage tracking
        auto* ref  = obj.Ref();
        auto* body = HavokUtil::GetRigidBody(ref);

        manip->Throw(obj);

        if (body && alterationSkill >= settings->perkLevelDamage) {
            RE::NiPoint3 vel = HavokUtil::GetLinearVelocity(body);
            dmg->RegisterThrown(ref, vel);
        }
    }
    m_held.clear();
    logger::info("ThrowAll: {} objects thrown (charge={:.2f})", m_held.size(), chargeLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// IsSpellActive — true while the player is actively casting our spell
// ─────────────────────────────────────────────────────────────────────────────
bool TelekinesisManager::IsSpellActive() const {
    if (!m_spell) return false;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return false;

    auto* list = player->GetMagicTarget()->GetActiveEffectList();
    if (!list) return false;

    for (auto* eff : *list) {
        if (!eff) continue;
        if (eff->spell == m_spell &&
            eff->flags.none(RE::ActiveEffect::Flag::kInactive) &&
            eff->flags.none(RE::ActiveEffect::Flag::kDispelled)) {
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Update — per-frame coordinator
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::Update(float dt) {
    auto* settings = Settings::GetSingleton();

    // ── Spell state machine ──────────────────────────────────────────────────
    bool spellActive = IsSpellActive();

    if (spellActive && !m_wasSpellActive) {
        // Button just pressed — grab nearest object
        AddObject(false);
        m_castHoldTime = 0.0f;
    }

    if (!spellActive && m_wasSpellActive && !m_held.empty()) {
        // Button just released — throw with accumulated charge
        float charge = std::clamp(m_castHoldTime / settings->throwChargeTime, 0.0f, 1.0f);
        logger::debug("Spell ended — throwing (hold={:.2f}s charge={:.2f})", m_castHoldTime, charge);
        ThrowAll(charge);
        m_wasSpellActive = spellActive;
        return;
    }

    m_wasSpellActive = spellActive;

    if (m_held.empty()) return;

    if (spellActive) {
        m_castHoldTime += dt;
    }

    // Remove any invalid/dead refs
    std::erase_if(m_held, [](const HeldObjectData& d) { return !d.IsValid(); });
    if (m_held.empty()) return;

    // Drain magicka; drop all if empty
    DrainMagicka(dt);
    if (m_held.empty()) return;

    auto* manip = ObjectManipulation::GetSingleton();
    int   total = static_cast<int>(m_held.size());

    for (auto& obj : m_held) {
        auto* ref = obj.Ref();
        if (!ref) continue;

        manip->CalcHoldPosition(obj.slotIndex, total, obj.vrLeftHand);

        switch (obj.state) {
        case HeldObjectState::Pulling:
        case HeldObjectState::Holding:
        case HeldObjectState::Charging:
            manip->UpdateHold(obj, dt);
            manip->AlignWeaponToAim(obj);
            break;
        case HeldObjectState::Released:
            break;
        }
    }

    std::erase_if(m_held, [](const HeldObjectData& d) {
        return d.state == HeldObjectState::Released;
    });
    if (!m_held.empty()) {
        manip->RepackSlots(m_held);
    }

    // Orbit shield
    int altSkill = static_cast<int>(
        RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(RE::ActorValue::kAlteration));

    if (settings->orbitShieldEnabled && total > 1 && altSkill >= settings->perkLevelOrbit) {
        CheckProjectileIntercepts();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SetCharging — kept for compatibility but no longer used (driven by spell state)
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::SetCharging(bool /*charging*/) {}

// ─────────────────────────────────────────────────────────────────────────────
// GetPerkMaxObjects — max objects gated by Alteration skill
// ─────────────────────────────────────────────────────────────────────────────
int TelekinesisManager::GetPerkMaxObjects() const {
    auto* settings = Settings::GetSingleton();
    auto* player   = RE::PlayerCharacter::GetSingleton();
    if (!player) return 1;

    int skill = static_cast<int>(player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kAlteration));

    if (skill >= settings->perkLevelMax5)   return 5;
    if (skill >= settings->perkLevelCharge) return 4;
    if (skill >= settings->perkLevelOrbit)  return 3;
    if (skill >= settings->perkLevelDamage) return 2;
    return 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// DrainMagicka — cost = base * perObj^(count-1) per second
// Drop everything if magicka runs out.
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::DrainMagicka(float dt) {
    auto* settings = Settings::GetSingleton();
    auto* player   = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    int   count   = static_cast<int>(m_held.size());
    float cost    = settings->magickaCostBase
                    * std::pow(settings->magickaCostPerObj, static_cast<float>(count - 1))
                    * dt;

    float current = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka);
    if (current <= 0.0f) {
        logger::info("Magicka exhausted — dropping all objects");
        DropAll();
        return;
    }

    player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage,
                               RE::ActorValue::kMagicka, -cost);
}

// ─────────────────────────────────────────────────────────────────────────────
// CheckProjectileIntercepts — orbit objects block incoming projectiles
// ─────────────────────────────────────────────────────────────────────────────
void TelekinesisManager::CheckProjectileIntercepts() {
    auto* settings = Settings::GetSingleton();
    auto* player   = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    RE::NiPoint3 playerPos = player->GetPosition();
    float        rSq       = settings->interceptRadius * settings->interceptRadius;

    // Check all active projectiles by iterating the player's current cell
    auto* cell = player->GetParentCell();
    if (!cell) return;

    cell->ForEachReference([&](RE::TESObjectREFR& refr) {
        auto* proj = refr.AsProjectile();
        if (!proj) return RE::BSContainer::ForEachResult::kContinue;

        // Skip our own projectiles
        auto& rtData = proj->GetProjectileRuntimeData();
        if (rtData.shooter.get().get() == player)
            return RE::BSContainer::ForEachResult::kContinue;

        RE::NiPoint3 projPos = proj->GetPosition();
        float dx = projPos.x - playerPos.x;
        float dy = projPos.y - playerPos.y;
        float dz = projPos.z - playerPos.z;

        if (dx * dx + dy * dy + dz * dz <= rSq) {
            float roll = static_cast<float>(rand()) / RAND_MAX;
            if (roll < settings->interceptChance) {
                proj->SetPosition({ projPos.x, projPos.y, projPos.z + 9999.0f });
                proj->Disable();
                logger::debug("Orbit shield intercepted projectile {:08X}", proj->GetFormID());
            }
        }
        return RE::BSContainer::ForEachResult::kContinue;
    });
}
