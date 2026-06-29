#pragma once
#include "PCH.h"

// ─────────────────────────────────────────────────────────────────────────────
// Settings — loaded from BetterTelekinesis.ini on startup and by the MCM.
// All values are in Skyrim units (1 unit ≈ 1 inch) unless noted.
// ─────────────────────────────────────────────────────────────────────────────
class Settings {
public:
    static Settings* GetSingleton();

    void Load();
    void Save() const;

    // ── Targeting ─────────────────────────────────────────────────────────────
    float maxGrabDistance    = 2000.0f;  // Max distance to grab an object
    float coneHalfAngleDeg   = 15.0f;   // Crosshair cone half-angle tolerance
    float pullSpeed          = 3500.0f; // Pull velocity toward player (units/s)

    // ── Hold / Orbit ─────────────────────────────────────────────────────────
    float holdDistance       = 150.0f;  // Distance ahead of player to hold objects
    float orbitRadius        = 90.0f;   // Orbit radius for multiple objects
    float springConstant     = 120.0f;  // Spring stiffness for spring-damper
    float dampingCoefficient = 14.0f;   // Damping for spring-damper
    float maxHoldSpeed       = 800.0f;  // Clamp velocity during hold updates
    int   maxObjects         = 5;       // Max simultaneous held objects (1-5)
    bool  holdToggleMode     = false;   // false = hold button, true = toggle

    // ── Throw / Charged Throw ─────────────────────────────────────────────────
    float throwBaseForce     = 5000.0f; // Base throw impulse (Skyrim units/s)
    float throwChargeMax     = 3.0f;    // Multiplier at full charge
    float throwChargeTime    = 2.0f;    // Seconds to reach full charge

    // ── Damage ────────────────────────────────────────────────────────────────
    float damageScalar       = 0.05f;   // damage = clamp(mass * vel² * scalar, min, max)
    float minDamage          = 1.0f;
    float maxDamage          = 200.0f;
    float staggerThreshold   = 40.0f;   // Damage above this staggers enemy
    float heavyObjectMass    = 15.0f;   // Always-stagger mass threshold (Skyrim lbs)
    bool  friendlyFire       = false;   // Apply damage to followers/allies

    // ── Orbit Shield ─────────────────────────────────────────────────────────
    bool  orbitShieldEnabled  = true;
    float interceptRadius     = 130.0f; // Interception sphere radius
    float interceptChance     = 0.75f;  // Probability to block projectile [0-1]

    // ── Magicka Cost ─────────────────────────────────────────────────────────
    float magickaCostBase     = 4.0f;   // Magicka/sec with 1 object
    float magickaCostPerObj   = 1.6f;   // cost = base * perObj^(count-1)

    // ── Perks integration ────────────────────────────────────────────────────
    // These define which Alteration skill level unlocks each feature.
    // Set to 0 to always enable, 100 to disable.
    int   perkLevelDamage     = 25;     // Apprentice: damage unlocked
    int   perkLevelOrbit      = 50;     // Adept: orbit/shield unlocked
    int   perkLevelCharge     = 75;     // Expert: charged throw unlocked
    int   perkLevelMax5       = 100;    // Master: max 5 objects

    // ── Hotkeys (DirectInput scan codes) ─────────────────────────────────────
    uint32_t keyAddObject     = 0x22;   // G — add nearest object to hold list
    uint32_t keyDropAll       = 0x13;   // R — drop all without throwing
    uint32_t keyThrow         = 0xFF;   // 0xFF = use cast button (configured in MCM)

    // ── VR ────────────────────────────────────────────────────────────────────
    bool  vrEnabled           = false;  // Auto-detected at runtime
    float vrAimConeHalfDeg    = 20.0f;  // Wider cone for VR controller aim

private:
    static constexpr auto kIniPath = "Data/SKSE/Plugins/BetterTelekinesis.ini";

    static float ReadF(const char* sec, const char* key, float def);
    static int   ReadI(const char* sec, const char* key, int   def);
    static bool  ReadB(const char* sec, const char* key, bool  def);
    static void  WriteF(const char* sec, const char* key, float val);
    static void  WriteI(const char* sec, const char* key, int   val);
    static void  WriteB(const char* sec, const char* key, bool  val);
};
