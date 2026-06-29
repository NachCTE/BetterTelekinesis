#include "DamageSystem.h"
#include "Settings.h"
#include "Util.h"
#include <cmath>

DamageSystem* DamageSystem::GetSingleton() {
    static DamageSystem instance;
    return &instance;
}

void DamageSystem::RegisterThrown(RE::TESObjectREFR* ref,
                                   const RE::NiPoint3& releaseVelocity) {
    if (!ref) return;

    ThrownObject obj;
    obj.handle       = ref->GetHandle();
    obj.lastVelocity = releaseVelocity;
    obj.lastSpeed    = MathUtil::Length(releaseVelocity);
    obj.timeAlive    = 0.0f;

    m_tracked.push_back(obj);
    logger::debug("DamageSystem: tracking thrown ref {:08X}, v={:.0f}",
                  ref->GetFormID(), obj.lastSpeed);
}

void DamageSystem::Unregister(RE::TESObjectREFR* ref) {
    if (!ref) return;
    std::erase_if(m_tracked, [&](const ThrownObject& t) {
        return t.handle.get().get() == ref;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-frame update: detect velocity drop (collision) and apply damage
// ─────────────────────────────────────────────────────────────────────────────
void DamageSystem::Update(float dt) {
    if (m_tracked.empty()) return;

    std::erase_if(m_tracked, [&](ThrownObject& t) -> bool {
        t.timeAlive += dt;
        if (t.timeAlive > ThrownObject::kMaxTrackTime) return true;

        RE::TESObjectREFR* ref = t.handle.get().get();
        if (!ref || ref->IsDisabled() || ref->IsDeleted()) return true;

        auto* body = HavokUtil::GetRigidBody(ref);
        if (!body) return true;

        RE::NiPoint3 curVel   = HavokUtil::GetLinearVelocity(body);
        float        curSpeed = MathUtil::Length(curVel);

        // Collision detected: speed drops more than 60% in one frame.
        // Skip the very first frame (lastSpeed not yet stabilized).
        if (t.timeAlive > 0.05f && t.lastSpeed > 100.0f &&
            curSpeed < t.lastSpeed * 0.4f) {
            ApplyImpactDamage(ref, body, t.lastSpeed, HavokUtil::GetMass(body));
            return true;  // stop tracking after first impact
        }

        t.lastVelocity = curVel;
        t.lastSpeed    = curSpeed;

        // Also stop if object has slowed to a crawl (landed on floor, etc.)
        if (curSpeed < 50.0f && t.timeAlive > 0.3f) return true;

        return false;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// ApplyImpactDamage
// ─────────────────────────────────────────────────────────────────────────────
void DamageSystem::ApplyImpactDamage(RE::TESObjectREFR* ref,
                                      RE::bhkRigidBody*  /* body */,
                                      float              impactSpeed,
                                      float              mass) const {
    auto* settings = Settings::GetSingleton();
    auto* root     = ref->Get3D();
    if (!root) return;

    RE::NiPoint3 pos = root->world.translate;

    // Find the nearest actor within a generous impact radius
    auto* target = FindNearestActor(pos, 120.0f);
    if (!target) return;

    if (!ShouldDamageActor(target)) return;

    // damage = clamp(mass * speed² * scalar, min, max)
    float rawDamage = mass * (impactSpeed * impactSpeed) * settings->damageScalar;
    float damage    = std::clamp(rawDamage, settings->minDamage, settings->maxDamage);

    logger::debug("Impact: mass={:.1f} speed={:.0f} -> damage={:.1f}", mass, impactSpeed, damage);

    // Apply health damage via ActorValueOwner interface
    target->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -damage);

    // Stagger on heavy impact
    bool shouldStagger = (damage >= settings->staggerThreshold)
                      || (mass >= settings->heavyObjectMass);
    if (shouldStagger) {
        target->SetGraphVariableFloat("staggerMagnitude", 1.0f);
        target->NotifyAnimationGraph("staggerStart");
    }
}

RE::Actor* DamageSystem::FindNearestActor(const RE::NiPoint3& pos, float radius) const {
    RE::Actor* nearest = nullptr;
    float      nearestDistSq = radius * radius;

    // Iterate process lists (all loaded actors)
    auto* pl = RE::ProcessLists::GetSingleton();
    if (!pl) return nullptr;

    auto CheckList = [&](RE::BSTArray<RE::ActorHandle>& list) {
        for (auto& handle : list) {
            RE::Actor* actor = handle.get().get();
            if (!actor) continue;
            if (actor->IsPlayerRef()) continue;

            auto* root = actor->Get3D();
            if (!root) continue;

            RE::NiPoint3 aPos  = root->world.translate;
            float dx = aPos.x - pos.x, dy = aPos.y - pos.y, dz = aPos.z - pos.z;
            float dSq = dx * dx + dy * dy + dz * dz;
            if (dSq < nearestDistSq) {
                nearestDistSq = dSq;
                nearest       = actor;
            }
        }
    };

    CheckList(pl->highActorHandles);
    CheckList(pl->middleHighActorHandles);

    return nearest;
}

bool DamageSystem::ShouldDamageActor(RE::Actor* actor) const {
    if (!actor) return false;
    if (actor->IsDead()) return false;

    if (!Settings::GetSingleton()->friendlyFire) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        // Skip allies (same team or friendly faction)
        if (actor->IsPlayerTeammate()) return false;
        if (player && actor->IsHostileToActor(player) == false &&
            !actor->IsInFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("CurrentFollowerFaction"))) {
            // Neutral NPC — still allow damage since it was an accident
        }
    }

    return true;
}
