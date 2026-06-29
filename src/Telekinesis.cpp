#include "Telekinesis.h"
#include <REL/Relocation.h>
#include <numbers>

// ── Hook on PlayerCharacter::Update ────────────────────────────────────────
struct Telekinesis::Hook
{
    static void Thunk(RE::PlayerCharacter* pc, float dt)
    {
        Telekinesis::GetSingleton()->Update(dt);
        func(pc, dt);
    }
    static inline REL::Relocation<decltype(Thunk)*> func;
    static void Install()
    {
        REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
        func = vtbl.write_vfunc(0xAD, Thunk);
        logger::info("PlayerCharacter::Update hook installed");
    }
};

// ── Singleton ───────────────────────────────────────────────────────────────
Telekinesis* Telekinesis::GetSingleton()
{
    static Telekinesis inst;
    return &inst;
}

void Telekinesis::Initialize(RE::SpellItem* spell)
{
    m_spell = spell;
    Hook::Install();
    logger::info("Telekinesis initialized");
}

// ── Helpers ─────────────────────────────────────────────────────────────────
RE::hkVector4 Telekinesis::ToHavok(const RE::NiPoint3& v)
{
    RE::hkVector4 r;
    r.quad.m128_f32[0] = v.x * kHavokScale;
    r.quad.m128_f32[1] = v.y * kHavokScale;
    r.quad.m128_f32[2] = v.z * kHavokScale;
    r.quad.m128_f32[3] = 0.0f;
    return r;
}

RE::NiPoint3 Telekinesis::CameraPos()
{
    auto* cam = RE::PlayerCamera::GetSingleton();
    if (cam && cam->cameraRoot)
        return cam->cameraRoot->world.translate;
    auto* p = RE::PlayerCharacter::GetSingleton();
    return p ? p->GetPosition() : RE::NiPoint3{};
}

RE::NiPoint3 Telekinesis::CameraForward()
{
    auto* cam = RE::PlayerCamera::GetSingleton();
    if (cam && cam->cameraRoot) {
        const auto& r = cam->cameraRoot->world.rotate;
        // Row 1 of NiMatrix3 = local Y = world forward in Skyrim
        RE::NiPoint3 f{ r.entry[0][1], r.entry[1][1], r.entry[2][1] };
        float len = std::sqrt(f.x*f.x + f.y*f.y + f.z*f.z);
        if (len > 1e-5f) return { f.x/len, f.y/len, f.z/len };
    }
    auto* p = RE::PlayerCharacter::GetSingleton();
    if (!p) return { 0, 1, 0 };
    float yaw = p->data.angle.z, pitch = p->data.angle.x;
    return { std::sin(yaw)*std::cos(pitch),
             std::cos(yaw)*std::cos(pitch),
            -std::sin(pitch) };
}

RE::bhkRigidBody* Telekinesis::GetBody(RE::TESObjectREFR* ref)
{
    if (!ref) return nullptr;
    auto* root = ref->Get3D();
    if (!root || !root->collisionObject) return nullptr;
    auto* col = static_cast<RE::bhkNiCollisionObject*>(root->collisionObject.get());
    if (!col || !col->body) return nullptr;
    return static_cast<RE::bhkRigidBody*>(col->body.get());
}

// ── Spell detection ─────────────────────────────────────────────────────────
bool Telekinesis::IsSpellActive() const
{
    if (!m_spell) return false;
    auto* p = RE::PlayerCharacter::GetSingleton();
    if (!p) return false;

    for (auto src : { RE::MagicSystem::CastingSource::kRightHand,
                      RE::MagicSystem::CastingSource::kLeftHand }) {
        auto* c = p->GetMagicCaster(src);
        if (!c || c->currentSpell != m_spell) continue;
        auto st = c->state.get();
        if (st == RE::MagicCaster::State::kCasting ||
            st == RE::MagicCaster::State::kCharging)
            return true;
    }
    return false;
}

// ── Targeting ───────────────────────────────────────────────────────────────
RE::TESObjectREFR* Telekinesis::FindTarget() const
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return nullptr;
    auto* cell = player->GetParentCell();
    if (!cell) return nullptr;

    RE::NiPoint3 origin  = CameraPos();
    RE::NiPoint3 forward = CameraForward();
    float cosMax = std::cos(kConeHalfDeg * std::numbers::pi_v<float> / 180.0f);

    RE::TESObjectREFR* best    = nullptr;
    float              bestDot = cosMax;

    // Only pick up loose items (same set as vanilla telekinesis)
    static constexpr std::array kAllowedTypes{
        RE::FormType::Misc,        RE::FormType::Weapon,      RE::FormType::Armor,
        RE::FormType::Ammo,        RE::FormType::Ingredient,  RE::FormType::AlchemyItem,
        RE::FormType::Book,        RE::FormType::Scroll,      RE::FormType::SoulGem,
        RE::FormType::KeyMaster,
    };

    cell->ForEachReference([&](RE::TESObjectREFR& ref) {
        if (&ref == player)           return RE::BSContainer::ForEachResult::kContinue;
        if (ref.IsDisabled())         return RE::BSContainer::ForEachResult::kContinue;
        if (ref.IsDeleted())          return RE::BSContainer::ForEachResult::kContinue;
        if (ref.As<RE::Actor>())      return RE::BSContainer::ForEachResult::kContinue;
        if (!GetBody(&ref))           return RE::BSContainer::ForEachResult::kContinue;

        // Skip anything that isn't a pickable loose item
        auto* base = ref.GetBaseObject();
        if (!base) return RE::BSContainer::ForEachResult::kContinue;
        auto ft = base->GetFormType();
        if (std::find(kAllowedTypes.begin(), kAllowedTypes.end(), ft) == kAllowedTypes.end())
            return RE::BSContainer::ForEachResult::kContinue;

        RE::NiPoint3 pos  = ref.GetPosition();
        RE::NiPoint3 diff = { pos.x - origin.x,
                              pos.y - origin.y,
                              pos.z - origin.z };
        float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
        if (dist > kMaxDist || dist < 10.0f)
            return RE::BSContainer::ForEachResult::kContinue;

        float dot = (diff.x*forward.x + diff.y*forward.y + diff.z*forward.z) / dist;
        if (dot > bestDot) {
            bestDot = dot;
            best    = &ref;
        }
        return RE::BSContainer::ForEachResult::kContinue;
    });

    if (best) logger::debug("Target: '{}' dot={:.3f}", best->GetName(), bestDot);
    else      logger::debug("No target in cone");
    return best;
}

// ── Grab ────────────────────────────────────────────────────────────────────
void Telekinesis::TryGrab()
{
    auto* target = FindTarget();
    if (!target) return;

    // Zero velocity so it doesn't keep flying when grabbed mid-air
    if (auto* body = GetBody(target))
        body->SetLinearVelocity(ToHavok({}));

    m_held     = target->GetHandle();
    m_holdTime = 0.0f;
    logger::info("Grabbed '{}'", target->GetName());
}

// ── Hold ────────────────────────────────────────────────────────────────────
void Telekinesis::Hold(float dt)
{
    auto* ref = m_held.get().get();
    if (!ref || ref->IsDisabled() || ref->IsDeleted()) {
        m_held = {};
        return;
    }
    m_holdTime += dt;

    // Float the object 150 units in front of camera, following its rotation
    RE::NiPoint3 cam    = CameraPos();
    RE::NiPoint3 fwd    = CameraForward();
    RE::NiPoint3 target = { cam.x + fwd.x * kHoldDist,
                             cam.y + fwd.y * kHoldDist,
                             cam.z + fwd.z * kHoldDist };

    // Move the rendered 3D node directly so it's visible in the right place
    if (auto* root = ref->Get3D()) {
        root->world.translate = target;
        root->local.translate = target;
    }

    // Move the Havok body (wakes it + syncs physics)
    auto* body = GetBody(ref);
    if (body) {
        auto hkPos = ToHavok(target);
        body->SetPosition(hkPos);
        body->SetLinearVelocity(ToHavok({}));
    }
}

// ── Throw ────────────────────────────────────────────────────────────────────
void Telekinesis::Throw()
{
    auto* ref = m_held.get().get();
    if (!ref) { m_held = {}; return; }

    auto* body = GetBody(ref);
    if (body) {
        float charge = std::clamp(m_holdTime / 2.0f, 0.0f, 1.0f);
        float force  = kThrowForce * (1.0f + charge * 2.0f);
        RE::NiPoint3 fwd = CameraForward();
        RE::NiPoint3 vel = { fwd.x * force, fwd.y * force, fwd.z * force };

        // Wake up the sleeping body then apply velocity
        RE::hkVector4 curHk;
        body->GetPosition(curHk);
        body->SetPosition(curHk);
        body->SetLinearVelocity(ToHavok(vel));

        logger::info("Thrown '{}' force={:.0f} charge={:.2f}",
                     ref->GetName(), force, charge);
    }

    m_held     = {};
    m_holdTime = 0.0f;
}

// ── Release (drop) ───────────────────────────────────────────────────────────
void Telekinesis::Release()
{
    auto* ref = m_held.get().get();
    if (ref) {
        if (auto* body = GetBody(ref))
            body->SetLinearVelocity(ToHavok({}));
        logger::info("Dropped '{}'", ref->GetName());
    }
    m_held     = {};
    m_holdTime = 0.0f;
}

// ── Per-frame update ─────────────────────────────────────────────────────────
void Telekinesis::Update(float dt)
{
    bool active = IsSpellActive();

    if (active && !m_wasActive) {
        // Spell just activated → grab nearest object in crosshair
        TryGrab();
    }

    if (!active && m_wasActive && m_held) {
        // Spell just deactivated → throw
        Throw();
    }

    m_wasActive = active;

    // Keep holding the object every frame while spell is active
    if (active && m_held) {
        Hold(dt);
    }
}
