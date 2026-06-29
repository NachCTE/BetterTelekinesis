#include "Settings.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Settings* Settings::GetSingleton() {
    static Settings instance;
    return &instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// Windows INI helpers (no extra dependency needed)
// ─────────────────────────────────────────────────────────────────────────────
float Settings::ReadF(const char* sec, const char* key, float def) {
    char buf[64];
    char defStr[64];
    ::snprintf(defStr, sizeof(defStr), "%f", def);
    ::GetPrivateProfileStringA(sec, key, defStr, buf, sizeof(buf), kIniPath);
    return ::strtof(buf, nullptr);
}

int Settings::ReadI(const char* sec, const char* key, int def) {
    return static_cast<int>(::GetPrivateProfileIntA(sec, key, def, kIniPath));
}

bool Settings::ReadB(const char* sec, const char* key, bool def) {
    return ReadI(sec, key, def ? 1 : 0) != 0;
}

void Settings::WriteF(const char* sec, const char* key, float val) {
    char buf[64];
    ::snprintf(buf, sizeof(buf), "%f", val);
    ::WritePrivateProfileStringA(sec, key, buf, kIniPath);
}

void Settings::WriteI(const char* sec, const char* key, int val) {
    char buf[32];
    ::snprintf(buf, sizeof(buf), "%d", val);
    ::WritePrivateProfileStringA(sec, key, buf, kIniPath);
}

void Settings::WriteB(const char* sec, const char* key, bool val) {
    WriteI(sec, key, val ? 1 : 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Load
// ─────────────────────────────────────────────────────────────────────────────
void Settings::Load() {
    // [Targeting]
    maxGrabDistance  = ReadF("Targeting", "fMaxGrabDistance",  maxGrabDistance);
    coneHalfAngleDeg = ReadF("Targeting", "fConeHalfAngleDeg", coneHalfAngleDeg);
    pullSpeed        = ReadF("Targeting", "fPullSpeed",         pullSpeed);

    // [Hold]
    holdDistance       = ReadF("Hold", "fHoldDistance",       holdDistance);
    orbitRadius        = ReadF("Hold", "fOrbitRadius",         orbitRadius);
    springConstant     = ReadF("Hold", "fSpringConstant",      springConstant);
    dampingCoefficient = ReadF("Hold", "fDampingCoefficient",  dampingCoefficient);
    maxHoldSpeed       = ReadF("Hold", "fMaxHoldSpeed",        maxHoldSpeed);
    maxObjects         = ReadI("Hold", "iMaxObjects",           maxObjects);
    holdToggleMode     = ReadB("Hold", "bHoldToggleMode",       holdToggleMode);
    maxObjects         = std::clamp(maxObjects, 1, 5);

    // [Throw]
    throwBaseForce  = ReadF("Throw", "fThrowBaseForce",  throwBaseForce);
    throwChargeMax  = ReadF("Throw", "fThrowChargeMax",  throwChargeMax);
    throwChargeTime = ReadF("Throw", "fThrowChargeTime", throwChargeTime);

    // [Damage]
    damageScalar     = ReadF("Damage", "fDamageScalar",     damageScalar);
    minDamage        = ReadF("Damage", "fMinDamage",         minDamage);
    maxDamage        = ReadF("Damage", "fMaxDamage",         maxDamage);
    staggerThreshold = ReadF("Damage", "fStaggerThreshold",  staggerThreshold);
    heavyObjectMass  = ReadF("Damage", "fHeavyObjectMass",   heavyObjectMass);
    friendlyFire     = ReadB("Damage", "bFriendlyFire",       friendlyFire);

    // [Shield]
    orbitShieldEnabled = ReadB("Shield", "bOrbitShieldEnabled", orbitShieldEnabled);
    interceptRadius    = ReadF("Shield", "fInterceptRadius",    interceptRadius);
    interceptChance    = ReadF("Shield", "fInterceptChance",    interceptChance);

    // [Magicka]
    magickaCostBase   = ReadF("Magicka", "fMagickaCostBase",   magickaCostBase);
    magickaCostPerObj = ReadF("Magicka", "fMagickaCostPerObj",  magickaCostPerObj);

    // [Perks]
    perkLevelDamage = ReadI("Perks", "iPerkLevelDamage", perkLevelDamage);
    perkLevelOrbit  = ReadI("Perks", "iPerkLevelOrbit",  perkLevelOrbit);
    perkLevelCharge = ReadI("Perks", "iPerkLevelCharge", perkLevelCharge);
    perkLevelMax5   = ReadI("Perks", "iPerkLevelMax5",   perkLevelMax5);

    // [Hotkeys]
    keyAddObject = static_cast<uint32_t>(ReadI("Hotkeys", "iKeyAddObject", static_cast<int>(keyAddObject)));
    keyDropAll   = static_cast<uint32_t>(ReadI("Hotkeys", "iKeyDropAll",   static_cast<int>(keyDropAll)));
    keyThrow     = static_cast<uint32_t>(ReadI("Hotkeys", "iKeyThrow",     static_cast<int>(keyThrow)));

    // [VR]
    vrAimConeHalfDeg = ReadF("VR", "fVRAimConeHalfDeg", vrAimConeHalfDeg);

    logger::info("Settings loaded from {}", kIniPath);
}

// ─────────────────────────────────────────────────────────────────────────────
// Save — called by MCM when the user applies changes
// ─────────────────────────────────────────────────────────────────────────────
void Settings::Save() const {
    WriteF("Targeting", "fMaxGrabDistance",  maxGrabDistance);
    WriteF("Targeting", "fConeHalfAngleDeg", coneHalfAngleDeg);
    WriteF("Targeting", "fPullSpeed",        pullSpeed);

    WriteF("Hold", "fHoldDistance",      holdDistance);
    WriteF("Hold", "fOrbitRadius",        orbitRadius);
    WriteF("Hold", "fSpringConstant",     springConstant);
    WriteF("Hold", "fDampingCoefficient", dampingCoefficient);
    WriteF("Hold", "fMaxHoldSpeed",       maxHoldSpeed);
    WriteI("Hold", "iMaxObjects",          maxObjects);
    WriteB("Hold", "bHoldToggleMode",      holdToggleMode);

    WriteF("Throw", "fThrowBaseForce",  throwBaseForce);
    WriteF("Throw", "fThrowChargeMax",  throwChargeMax);
    WriteF("Throw", "fThrowChargeTime", throwChargeTime);

    WriteF("Damage", "fDamageScalar",    damageScalar);
    WriteF("Damage", "fMinDamage",        minDamage);
    WriteF("Damage", "fMaxDamage",        maxDamage);
    WriteF("Damage", "fStaggerThreshold", staggerThreshold);
    WriteF("Damage", "fHeavyObjectMass",  heavyObjectMass);
    WriteB("Damage", "bFriendlyFire",      friendlyFire);

    WriteB("Shield", "bOrbitShieldEnabled", orbitShieldEnabled);
    WriteF("Shield", "fInterceptRadius",    interceptRadius);
    WriteF("Shield", "fInterceptChance",    interceptChance);

    WriteF("Magicka", "fMagickaCostBase",   magickaCostBase);
    WriteF("Magicka", "fMagickaCostPerObj",  magickaCostPerObj);

    WriteI("Perks", "iPerkLevelDamage", perkLevelDamage);
    WriteI("Perks", "iPerkLevelOrbit",  perkLevelOrbit);
    WriteI("Perks", "iPerkLevelCharge", perkLevelCharge);
    WriteI("Perks", "iPerkLevelMax5",   perkLevelMax5);

    WriteI("Hotkeys", "iKeyAddObject", static_cast<int>(keyAddObject));
    WriteI("Hotkeys", "iKeyDropAll",   static_cast<int>(keyDropAll));
    WriteI("Hotkeys", "iKeyThrow",     static_cast<int>(keyThrow));

    WriteF("VR", "fVRAimConeHalfDeg", vrAimConeHalfDeg);
}
