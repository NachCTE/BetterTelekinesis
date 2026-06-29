; ============================================================================
; BetterTelekinesis_MCM.psc — MCM script for Ultimate Telekinesis
; Requires: SkyUI 5+
; Place at: Data/Scripts/Source/BetterTelekinesis_MCM.psc
; ============================================================================
Scriptname BetterTelekinesis_MCM extends SKI_ConfigBase

; ─── Page indices ────────────────────────────────────────────────────────────
int _page_targeting
int _page_hold
int _page_combat
int _page_hotkeys

; ─── Option IDs ──────────────────────────────────────────────────────────────
int _oid_maxDistance
int _oid_coneAngle
int _oid_maxObjects
int _oid_holdToggle
int _oid_holdDistance
int _oid_orbitRadius
int _oid_pullSpeed
int _oid_throwForce
int _oid_chargeTime
int _oid_chargeMax
int _oid_damageScalar
int _oid_minDamage
int _oid_maxDamage
int _oid_staggerThresh
int _oid_friendlyFire
int _oid_orbitShield
int _oid_interceptChance
int _oid_magickaCostBase
int _oid_magickaCostMult
int _oid_keyAddObject
int _oid_keyDropAll
int _oid_vrConeAngle

; ─── OnConfigInit ────────────────────────────────────────────────────────────
Event OnConfigInit()
    ModName = "Ultimate Telekinesis"

    Pages  = new String[4]
    Pages[0] = "$BT_Page_Targeting"
    Pages[1] = "$BT_Page_Hold"
    Pages[2] = "$BT_Page_Combat"
    Pages[3] = "$BT_Page_Hotkeys"
EndEvent

; ─── OnPageReset ─────────────────────────────────────────────────────────────
Event OnPageReset(String a_page)
    If a_page == "$BT_Page_Targeting"
        SetCursorFillMode(TOP_TO_BOTTOM)
        AddHeaderOption("$BT_Header_Targeting")

        _oid_maxDistance = AddSliderOption("$BT_MaxDistance",
            GetModSettingFloat("fMaxGrabDistance:Targeting"), "{0} units")
        _oid_coneAngle   = AddSliderOption("$BT_ConeAngle",
            GetModSettingFloat("fConeHalfAngleDeg:Targeting"), "{1}°")
        _oid_pullSpeed   = AddSliderOption("$BT_PullSpeed",
            GetModSettingFloat("fPullSpeed:Targeting"), "{0} u/s")

        AddHeaderOption("$BT_Header_VR")
        _oid_vrConeAngle = AddSliderOption("$BT_VRConeAngle",
            GetModSettingFloat("fVRAimConeHalfDeg:VR"), "{1}°")

    ElseIf a_page == "$BT_Page_Hold"
        SetCursorFillMode(TOP_TO_BOTTOM)
        AddHeaderOption("$BT_Header_Hold")

        _oid_maxObjects  = AddSliderOption("$BT_MaxObjects",
            GetModSettingInt("iMaxObjects:Hold"), "{0}")
        _oid_holdToggle  = AddToggleOption("$BT_HoldToggle",
            GetModSettingBool("bHoldToggleMode:Hold"))
        _oid_holdDistance= AddSliderOption("$BT_HoldDistance",
            GetModSettingFloat("fHoldDistance:Hold"), "{0} units")
        _oid_orbitRadius = AddSliderOption("$BT_OrbitRadius",
            GetModSettingFloat("fOrbitRadius:Hold"), "{0} units")

        AddHeaderOption("$BT_Header_Magicka")
        _oid_magickaCostBase = AddSliderOption("$BT_MagickaCostBase",
            GetModSettingFloat("fMagickaCostBase:Magicka"), "{1}/s")
        _oid_magickaCostMult = AddSliderOption("$BT_MagickaCostMult",
            GetModSettingFloat("fMagickaCostPerObj:Magicka"), "×{2}")

    ElseIf a_page == "$BT_Page_Combat"
        SetCursorFillMode(TOP_TO_BOTTOM)
        AddHeaderOption("$BT_Header_Throw")

        _oid_throwForce  = AddSliderOption("$BT_ThrowForce",
            GetModSettingFloat("fThrowBaseForce:Throw"), "{0}")
        _oid_chargeMax   = AddSliderOption("$BT_ChargeMax",
            GetModSettingFloat("fThrowChargeMax:Throw"), "×{1}")
        _oid_chargeTime  = AddSliderOption("$BT_ChargeTime",
            GetModSettingFloat("fThrowChargeTime:Throw"), "{1}s")

        AddHeaderOption("$BT_Header_Damage")
        _oid_damageScalar  = AddSliderOption("$BT_DamageScalar",
            GetModSettingFloat("fDamageScalar:Damage"), "{3}")
        _oid_minDamage     = AddSliderOption("$BT_MinDamage",
            GetModSettingFloat("fMinDamage:Damage"), "{1}")
        _oid_maxDamage     = AddSliderOption("$BT_MaxDamage",
            GetModSettingFloat("fMaxDamage:Damage"), "{0}")
        _oid_staggerThresh = AddSliderOption("$BT_StaggerThreshold",
            GetModSettingFloat("fStaggerThreshold:Damage"), "{1}")
        _oid_friendlyFire  = AddToggleOption("$BT_FriendlyFire",
            GetModSettingBool("bFriendlyFire:Damage"))

        AddHeaderOption("$BT_Header_Shield")
        _oid_orbitShield     = AddToggleOption("$BT_OrbitShield",
            GetModSettingBool("bOrbitShieldEnabled:Shield"))
        _oid_interceptChance = AddSliderOption("$BT_InterceptChance",
            GetModSettingFloat("fInterceptChance:Shield") * 100.0, "{0}%")

    ElseIf a_page == "$BT_Page_Hotkeys"
        SetCursorFillMode(TOP_TO_BOTTOM)
        AddHeaderOption("$BT_Header_Hotkeys")

        _oid_keyAddObject = AddKeyMapOption("$BT_KeyAddObject",
            GetModSettingInt("iKeyAddObject:Hotkeys"))
        _oid_keyDropAll   = AddKeyMapOption("$BT_KeyDropAll",
            GetModSettingInt("iKeyDropAll:Hotkeys"))
    EndIf
EndEvent

; ─── OnOptionSliderOpen ──────────────────────────────────────────────────────
Event OnOptionSliderOpen(Int a_option)
    If a_option == _oid_maxDistance
        SetSliderDialogStartValue(GetModSettingFloat("fMaxGrabDistance:Targeting"))
        SetSliderDialogDefaultValue(2000.0)
        SetSliderDialogRange(200.0, 5000.0)
        SetSliderDialogInterval(50.0)
    ElseIf a_option == _oid_coneAngle
        SetSliderDialogStartValue(GetModSettingFloat("fConeHalfAngleDeg:Targeting"))
        SetSliderDialogDefaultValue(15.0)
        SetSliderDialogRange(5.0, 45.0)
        SetSliderDialogInterval(1.0)
    ElseIf a_option == _oid_pullSpeed
        SetSliderDialogStartValue(GetModSettingFloat("fPullSpeed:Targeting"))
        SetSliderDialogDefaultValue(3500.0)
        SetSliderDialogRange(500.0, 8000.0)
        SetSliderDialogInterval(100.0)
    ElseIf a_option == _oid_maxObjects
        SetSliderDialogStartValue(GetModSettingInt("iMaxObjects:Hold") as Float)
        SetSliderDialogDefaultValue(5.0)
        SetSliderDialogRange(1.0, 5.0)
        SetSliderDialogInterval(1.0)
    ElseIf a_option == _oid_holdDistance
        SetSliderDialogStartValue(GetModSettingFloat("fHoldDistance:Hold"))
        SetSliderDialogDefaultValue(150.0)
        SetSliderDialogRange(50.0, 400.0)
        SetSliderDialogInterval(10.0)
    ElseIf a_option == _oid_orbitRadius
        SetSliderDialogStartValue(GetModSettingFloat("fOrbitRadius:Hold"))
        SetSliderDialogDefaultValue(90.0)
        SetSliderDialogRange(30.0, 200.0)
        SetSliderDialogInterval(5.0)
    ElseIf a_option == _oid_throwForce
        SetSliderDialogStartValue(GetModSettingFloat("fThrowBaseForce:Throw"))
        SetSliderDialogDefaultValue(5000.0)
        SetSliderDialogRange(1000.0, 15000.0)
        SetSliderDialogInterval(250.0)
    ElseIf a_option == _oid_chargeMax
        SetSliderDialogStartValue(GetModSettingFloat("fThrowChargeMax:Throw"))
        SetSliderDialogDefaultValue(3.0)
        SetSliderDialogRange(1.0, 5.0)
        SetSliderDialogInterval(0.5)
    ElseIf a_option == _oid_chargeTime
        SetSliderDialogStartValue(GetModSettingFloat("fThrowChargeTime:Throw"))
        SetSliderDialogDefaultValue(2.0)
        SetSliderDialogRange(0.5, 5.0)
        SetSliderDialogInterval(0.25)
    ElseIf a_option == _oid_damageScalar
        SetSliderDialogStartValue(GetModSettingFloat("fDamageScalar:Damage"))
        SetSliderDialogDefaultValue(0.05)
        SetSliderDialogRange(0.001, 0.5)
        SetSliderDialogInterval(0.001)
    ElseIf a_option == _oid_minDamage
        SetSliderDialogStartValue(GetModSettingFloat("fMinDamage:Damage"))
        SetSliderDialogDefaultValue(1.0)
        SetSliderDialogRange(0.0, 50.0)
        SetSliderDialogInterval(1.0)
    ElseIf a_option == _oid_maxDamage
        SetSliderDialogStartValue(GetModSettingFloat("fMaxDamage:Damage"))
        SetSliderDialogDefaultValue(200.0)
        SetSliderDialogRange(10.0, 999.0)
        SetSliderDialogInterval(10.0)
    ElseIf a_option == _oid_staggerThresh
        SetSliderDialogStartValue(GetModSettingFloat("fStaggerThreshold:Damage"))
        SetSliderDialogDefaultValue(40.0)
        SetSliderDialogRange(1.0, 200.0)
        SetSliderDialogInterval(5.0)
    ElseIf a_option == _oid_interceptChance
        SetSliderDialogStartValue(GetModSettingFloat("fInterceptChance:Shield") * 100.0)
        SetSliderDialogDefaultValue(75.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    ElseIf a_option == _oid_magickaCostBase
        SetSliderDialogStartValue(GetModSettingFloat("fMagickaCostBase:Magicka"))
        SetSliderDialogDefaultValue(4.0)
        SetSliderDialogRange(0.0, 30.0)
        SetSliderDialogInterval(0.5)
    ElseIf a_option == _oid_magickaCostMult
        SetSliderDialogStartValue(GetModSettingFloat("fMagickaCostPerObj:Magicka"))
        SetSliderDialogDefaultValue(1.6)
        SetSliderDialogRange(1.0, 3.0)
        SetSliderDialogInterval(0.1)
    ElseIf a_option == _oid_vrConeAngle
        SetSliderDialogStartValue(GetModSettingFloat("fVRAimConeHalfDeg:VR"))
        SetSliderDialogDefaultValue(20.0)
        SetSliderDialogRange(5.0, 60.0)
        SetSliderDialogInterval(1.0)
    EndIf
EndEvent

; ─── OnOptionSliderAccept ────────────────────────────────────────────────────
Event OnOptionSliderAccept(Int a_option, Float a_value)
    If a_option == _oid_maxDistance
        SetModSettingFloat("fMaxGrabDistance:Targeting", a_value)
        SetSliderOptionValue(a_option, a_value, "{0} units")
    ElseIf a_option == _oid_coneAngle
        SetModSettingFloat("fConeHalfAngleDeg:Targeting", a_value)
        SetSliderOptionValue(a_option, a_value, "{1}°")
    ElseIf a_option == _oid_pullSpeed
        SetModSettingFloat("fPullSpeed:Targeting", a_value)
        SetSliderOptionValue(a_option, a_value, "{0} u/s")
    ElseIf a_option == _oid_maxObjects
        SetModSettingInt("iMaxObjects:Hold", a_value as Int)
        SetSliderOptionValue(a_option, a_value, "{0}")
    ElseIf a_option == _oid_holdDistance
        SetModSettingFloat("fHoldDistance:Hold", a_value)
        SetSliderOptionValue(a_option, a_value, "{0} units")
    ElseIf a_option == _oid_orbitRadius
        SetModSettingFloat("fOrbitRadius:Hold", a_value)
        SetSliderOptionValue(a_option, a_value, "{0} units")
    ElseIf a_option == _oid_throwForce
        SetModSettingFloat("fThrowBaseForce:Throw", a_value)
        SetSliderOptionValue(a_option, a_value, "{0}")
    ElseIf a_option == _oid_chargeMax
        SetModSettingFloat("fThrowChargeMax:Throw", a_value)
        SetSliderOptionValue(a_option, a_value, "×{1}")
    ElseIf a_option == _oid_chargeTime
        SetModSettingFloat("fThrowChargeTime:Throw", a_value)
        SetSliderOptionValue(a_option, a_value, "{1}s")
    ElseIf a_option == _oid_damageScalar
        SetModSettingFloat("fDamageScalar:Damage", a_value)
        SetSliderOptionValue(a_option, a_value, "{3}")
    ElseIf a_option == _oid_minDamage
        SetModSettingFloat("fMinDamage:Damage", a_value)
        SetSliderOptionValue(a_option, a_value, "{1}")
    ElseIf a_option == _oid_maxDamage
        SetModSettingFloat("fMaxDamage:Damage", a_value)
        SetSliderOptionValue(a_option, a_value, "{0}")
    ElseIf a_option == _oid_staggerThresh
        SetModSettingFloat("fStaggerThreshold:Damage", a_value)
        SetSliderOptionValue(a_option, a_value, "{1}")
    ElseIf a_option == _oid_interceptChance
        SetModSettingFloat("fInterceptChance:Shield", a_value / 100.0)
        SetSliderOptionValue(a_option, a_value, "{0}%")
    ElseIf a_option == _oid_magickaCostBase
        SetModSettingFloat("fMagickaCostBase:Magicka", a_value)
        SetSliderOptionValue(a_option, a_value, "{1}/s")
    ElseIf a_option == _oid_magickaCostMult
        SetModSettingFloat("fMagickaCostPerObj:Magicka", a_value)
        SetSliderOptionValue(a_option, a_value, "×{2}")
    ElseIf a_option == _oid_vrConeAngle
        SetModSettingFloat("fVRAimConeHalfDeg:VR", a_value)
        SetSliderOptionValue(a_option, a_value, "{1}°")
    EndIf
EndEvent

; ─── OnOptionToggle ───────────────────────────────────────────────────────────
Event OnOptionToggle(Int a_option, Bool a_value)
    If a_option == _oid_holdToggle
        SetModSettingBool("bHoldToggleMode:Hold", a_value)
        SetToggleOptionValue(a_option, a_value)
    ElseIf a_option == _oid_friendlyFire
        SetModSettingBool("bFriendlyFire:Damage", a_value)
        SetToggleOptionValue(a_option, a_value)
    ElseIf a_option == _oid_orbitShield
        SetModSettingBool("bOrbitShieldEnabled:Shield", a_value)
        SetToggleOptionValue(a_option, a_value)
    EndIf
EndEvent

; ─── OnOptionKeyMapChange ────────────────────────────────────────────────────
Event OnOptionKeyMapChange(Int a_option, Int a_keyCode, String a_conflictControl, String a_conflictName)
    If a_option == _oid_keyAddObject
        SetModSettingInt("iKeyAddObject:Hotkeys", a_keyCode)
        SetKeyMapOptionValue(a_option, a_keyCode)
    ElseIf a_option == _oid_keyDropAll
        SetModSettingInt("iKeyDropAll:Hotkeys", a_keyCode)
        SetKeyMapOptionValue(a_option, a_keyCode)
    EndIf
EndEvent

; ─── OnOptionDefault ─────────────────────────────────────────────────────────
Event OnOptionDefault(Int a_option)
    If a_option == _oid_maxDistance
        SetModSettingFloat("fMaxGrabDistance:Targeting", 2000.0)
    ElseIf a_option == _oid_coneAngle
        SetModSettingFloat("fConeHalfAngleDeg:Targeting", 15.0)
    ElseIf a_option == _oid_maxObjects
        SetModSettingInt("iMaxObjects:Hold", 5)
    ElseIf a_option == _oid_holdToggle
        SetModSettingBool("bHoldToggleMode:Hold", False)
    ElseIf a_option == _oid_throwForce
        SetModSettingFloat("fThrowBaseForce:Throw", 5000.0)
    ElseIf a_option == _oid_friendlyFire
        SetModSettingBool("bFriendlyFire:Damage", False)
    ElseIf a_option == _oid_orbitShield
        SetModSettingBool("bOrbitShieldEnabled:Shield", True)
    EndIf
    ForcePageReset()
EndEvent
