#include "SkinChanger.h"
#include "Core/Logger.h"
#include "Core/Memory.h"
#include "Game/GameOffsets.h"
#include <windows.h>

namespace ShaiyaOverlay
{
    SkinChangerConfig SkinChanger::Config;
    CharacterVisualBackup SkinChanger::Backup;
    U32 SkinChanger::LastAppliedTick = 0;

    U64 SkinChanger::GetLocalPlayerCharacterPtr()
    {
        if (!Offsets.WorldManager)
            return 0;

        U64 LocalPlayerPtr = 0;
        if (!Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &LocalPlayerPtr) || !LocalPlayerPtr)
            return 0;

        return LocalPlayerPtr;
    }

    void SkinChanger::SaveOriginalAppearance(U64 LocalPlayerPtr)
    {
        if (!LocalPlayerPtr || Backup.Saved)
            return;

        Memory::ReadSafe(LocalPlayerPtr + 860, &Backup.Hair);
        Memory::ReadSafe(LocalPlayerPtr + 861, &Backup.Face);

        Memory::ReadSafe(LocalPlayerPtr + 876, &Backup.HelmetType);
        Memory::ReadSafe(LocalPlayerPtr + 893, &Backup.HelmetTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 877, &Backup.UpperType);
        Memory::ReadSafe(LocalPlayerPtr + 894, &Backup.UpperTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 878, &Backup.LowerType);
        Memory::ReadSafe(LocalPlayerPtr + 895, &Backup.LowerTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 879, &Backup.GlovesType);
        Memory::ReadSafe(LocalPlayerPtr + 896, &Backup.GlovesTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 880, &Backup.BootsType);
        Memory::ReadSafe(LocalPlayerPtr + 897, &Backup.BootsTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 881, &Backup.Weapon1Type);
        Memory::ReadSafe(LocalPlayerPtr + 898, &Backup.Weapon1TypeId);

        Memory::ReadSafe(LocalPlayerPtr + 882, &Backup.Weapon2Type);
        Memory::ReadSafe(LocalPlayerPtr + 899, &Backup.Weapon2TypeId);

        Memory::ReadSafe(LocalPlayerPtr + 890, &Backup.WingsType);
        Memory::ReadSafe(LocalPlayerPtr + 907, &Backup.WingsTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 891, &Backup.CostumeType);
        Memory::ReadSafe(LocalPlayerPtr + 908, &Backup.CostumeTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 892, &Backup.MountType);
        Memory::ReadSafe(LocalPlayerPtr + 909, &Backup.MountTypeId);

        Memory::ReadSafe(LocalPlayerPtr + 915, &Backup.Weapon1Glow);
        Memory::ReadSafe(LocalPlayerPtr + 916, &Backup.Weapon2Glow);

        Backup.Saved = true;
    }

    void SkinChanger::ApplySkins()
    {
        U64 LocalPlayerPtr = GetLocalPlayerCharacterPtr();
        if (!LocalPlayerPtr)
            return;

        SaveOriginalAppearance(LocalPlayerPtr);

        // 1. Costume / Transformation Override (Type 150)
        if (Config.OverrideCostume && Config.CostumeTypeId > 0)
        {
            *reinterpret_cast<U8*>(LocalPlayerPtr + 891) = 150;
            *reinterpret_cast<U8*>(LocalPlayerPtr + 908) = Config.CostumeTypeId;
        }
        else
        {
            *reinterpret_cast<U8*>(LocalPlayerPtr + 891) = Backup.CostumeType;
            *reinterpret_cast<U8*>(LocalPlayerPtr + 908) = Backup.CostumeTypeId;
            *reinterpret_cast<U32*>(LocalPlayerPtr + 348) = 0;
        }

        // 2. Wings Override (Type 121)
        if (Config.OverrideWings && Config.WingsTypeId > 0)
        {
            *reinterpret_cast<U8*>(LocalPlayerPtr + 890) = 121;
            *reinterpret_cast<U8*>(LocalPlayerPtr + 907) = Config.WingsTypeId;
        }
        else if (Backup.Saved)
        {
            *reinterpret_cast<U8*>(LocalPlayerPtr + 890) = Backup.WingsType;
            *reinterpret_cast<U8*>(LocalPlayerPtr + 907) = Backup.WingsTypeId;
        }

        // 3. Weapons Glow (+0 .. +20)
        if (Config.OverrideGlow)
        {
            *reinterpret_cast<U8*>(LocalPlayerPtr + 915) = Config.Weapon1Glow;
            *reinterpret_cast<U8*>(LocalPlayerPtr + 916) = Config.Weapon2Glow;
        }
        else if (Backup.Saved)
        {
            *reinterpret_cast<U8*>(LocalPlayerPtr + 915) = Backup.Weapon1Glow;
            *reinterpret_cast<U8*>(LocalPlayerPtr + 916) = Backup.Weapon2Glow;
        }

        // 4. Hair & Face
        if (Config.OverrideHair)
            *reinterpret_cast<U8*>(LocalPlayerPtr + 860) = Config.Hair;

        if (Config.OverrideFace)
            *reinterpret_cast<U8*>(LocalPlayerPtr + 861) = Config.Face;

        // 5. Individual armor parts (if not in costume)
        if (!Config.OverrideCostume)
        {
            if (Config.Helmet.Override)
            {
                *reinterpret_cast<U8*>(LocalPlayerPtr + 876) = Config.Helmet.Type;
                *reinterpret_cast<U8*>(LocalPlayerPtr + 893) = Config.Helmet.TypeId;
            }
            if (Config.Upper.Override)
            {
                *reinterpret_cast<U8*>(LocalPlayerPtr + 877) = Config.Upper.Type;
                *reinterpret_cast<U8*>(LocalPlayerPtr + 894) = Config.Upper.TypeId;
            }
            if (Config.Lower.Override)
            {
                *reinterpret_cast<U8*>(LocalPlayerPtr + 878) = Config.Lower.Type;
                *reinterpret_cast<U8*>(LocalPlayerPtr + 895) = Config.Lower.TypeId;
            }
            if (Config.Gloves.Override)
            {
                *reinterpret_cast<U8*>(LocalPlayerPtr + 879) = Config.Gloves.Type;
                *reinterpret_cast<U8*>(LocalPlayerPtr + 896) = Config.Gloves.TypeId;
            }
            if (Config.Boots.Override)
            {
                *reinterpret_cast<U8*>(LocalPlayerPtr + 880) = Config.Boots.Type;
                *reinterpret_cast<U8*>(LocalPlayerPtr + 897) = Config.Boots.TypeId;
            }
        }

        // 6. Rebuild equipment 3D meshes natively
        if (Offsets.ReloadEquipmentAddr)
        {
            typedef void (__fastcall *ReloadEquipFn)(void*);
            reinterpret_cast<ReloadEquipFn>(Offsets.ReloadEquipmentAddr)(reinterpret_cast<void*>(LocalPlayerPtr));
        }

        // 7. Rebuild weapons and particle auras natively
        if (Offsets.UpdateWeaponsAddr)
        {
            typedef void (__fastcall *UpdateWeaponsFn)(void*);
            reinterpret_cast<UpdateWeaponsFn>(Offsets.UpdateWeaponsAddr)(reinterpret_cast<void*>(LocalPlayerPtr));
        }

        LastAppliedTick = GetTickCount();
    }

    void SkinChanger::RestoreOriginal()
    {
        U64 LocalPlayerPtr = GetLocalPlayerCharacterPtr();
        if (!LocalPlayerPtr || !Backup.Saved)
            return;

        Config.Enabled = false;
        Config.OverrideCostume = false;
        Config.CostumeTypeId = 0;
        Config.OverrideWings = false;
        Config.WingsTypeId = 0;
        Config.OverrideGlow = false;
        Config.OverrideHair = false;
        Config.OverrideFace = false;
        Config.Helmet.Override = false;
        Config.Upper.Override = false;
        Config.Lower.Override = false;
        Config.Gloves.Override = false;
        Config.Boots.Override = false;

        *reinterpret_cast<U8*>(LocalPlayerPtr + 860) = Backup.Hair;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 861) = Backup.Face;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 876) = Backup.HelmetType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 893) = Backup.HelmetTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 877) = Backup.UpperType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 894) = Backup.UpperTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 878) = Backup.LowerType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 895) = Backup.LowerTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 879) = Backup.GlovesType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 896) = Backup.GlovesTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 880) = Backup.BootsType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 897) = Backup.BootsTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 881) = Backup.Weapon1Type;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 898) = Backup.Weapon1TypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 882) = Backup.Weapon2Type;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 899) = Backup.Weapon2TypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 890) = Backup.WingsType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 907) = Backup.WingsTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 891) = Backup.CostumeType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 908) = Backup.CostumeTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 892) = Backup.MountType;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 909) = Backup.MountTypeId;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 915) = Backup.Weapon1Glow;
        *reinterpret_cast<U8*>(LocalPlayerPtr + 916) = Backup.Weapon2Glow;

        // Reset shape / costume flag
        *reinterpret_cast<U32*>(LocalPlayerPtr + 348) = 0;
        *reinterpret_cast<U32*>(LocalPlayerPtr + 380) = 0;

        if (Offsets.ReloadEquipmentAddr)
        {
            typedef void (__fastcall *ReloadEquipFn)(void*);
            reinterpret_cast<ReloadEquipFn>(Offsets.ReloadEquipmentAddr)(reinterpret_cast<void*>(LocalPlayerPtr));
        }

        if (Offsets.UpdateWeaponsAddr)
        {
            typedef void (__fastcall *UpdateWeaponsFn)(void*);
            reinterpret_cast<UpdateWeaponsFn>(Offsets.UpdateWeaponsAddr)(reinterpret_cast<void*>(LocalPlayerPtr));
        }

        Backup.Saved = false;
    }

    void SkinChanger::SetTransformation(int costumeTypeId, int wingsTypeId, int glowLevel)
    {
        Config.Enabled = true;
        Config.OverrideCostume = (costumeTypeId > 0);
        Config.CostumeTypeId = static_cast<U8>(costumeTypeId);

        if (wingsTypeId > 0)
        {
            Config.OverrideWings = true;
            Config.WingsTypeId = static_cast<U8>(wingsTypeId);
        }

        if (glowLevel >= 0)
        {
            Config.OverrideGlow = true;
            Config.Weapon1Glow = static_cast<U8>(glowLevel);
            Config.Weapon2Glow = static_cast<U8>(glowLevel);
        }

        ApplySkins();
    }

    void SkinChanger::SetWings(int wingsTypeId)
    {
        Config.Enabled = true;
        Config.OverrideWings = (wingsTypeId > 0);
        Config.WingsTypeId = static_cast<U8>(wingsTypeId);
        ApplySkins();
    }

    void SkinChanger::SetGlowPreset(int glowLevel)
    {
        Config.Enabled = true;
        Config.OverrideGlow = true;
        Config.Weapon1Glow = static_cast<U8>(glowLevel);
        Config.Weapon2Glow = static_cast<U8>(glowLevel);
        ApplySkins();
    }

    void SkinChanger::SetArmorPreset(int presetIndex)
    {
        U64 LocalPlayerPtr = GetLocalPlayerCharacterPtr();
        if (!LocalPlayerPtr)
            return;

        // Disable costume when choosing standard armor preset
        Config.OverrideCostume = false;
        Config.CostumeTypeId = 0;

        // Match current worn armor types for clean rendering
        U8 curHelmType = *reinterpret_cast<U8*>(LocalPlayerPtr + 876);
        U8 curUpperType = *reinterpret_cast<U8*>(LocalPlayerPtr + 877);
        U8 curLowerType = *reinterpret_cast<U8*>(LocalPlayerPtr + 878);
        U8 curGlovesType = *reinterpret_cast<U8*>(LocalPlayerPtr + 879);
        U8 curBootsType = *reinterpret_cast<U8*>(LocalPlayerPtr + 880);

        if (curHelmType == 0) curHelmType = 16;
        if (curUpperType == 0) curUpperType = 19;
        if (curLowerType == 0) curLowerType = 20;
        if (curGlovesType == 0) curGlovesType = 21;
        if (curBootsType == 0) curBootsType = 22;

        U8 targetTypeId = 45;
        switch (presetIndex)
        {
        case 1: targetTypeId = 41; break; // Eltaphen
        case 2: targetTypeId = 43; break; // Eltaphen Heróico
        case 3: targetTypeId = 44; break; // Eltaphen Atroz
        case 4: targetTypeId = 45; break; // Eltaphen Legendário
        case 5: targetTypeId = 54; break; // Escudo Fênix Atroz / Phoenix Set
        default: targetTypeId = 45; break;
        }

        Config.Helmet.Override = true;
        Config.Helmet.Type = curHelmType;
        Config.Helmet.TypeId = targetTypeId;

        Config.Upper.Override = true;
        Config.Upper.Type = curUpperType;
        Config.Upper.TypeId = targetTypeId;

        Config.Lower.Override = true;
        Config.Lower.Type = curLowerType;
        Config.Lower.TypeId = targetTypeId;

        Config.Gloves.Override = true;
        Config.Gloves.Type = curGlovesType;
        Config.Gloves.TypeId = targetTypeId;

        Config.Boots.Override = true;
        Config.Boots.Type = curBootsType;
        Config.Boots.TypeId = targetTypeId;

        Config.Enabled = true;
        ApplySkins();
    }

    void SkinChanger::Update()
    {
        static bool WasEnabled = false;
        if (Config.Enabled)
        {
            U32 Now = GetTickCount();
            if (!WasEnabled)
            {
                ApplySkins();
                WasEnabled = true;
            }
            else if (Now - LastAppliedTick > 5000)
            {
                ApplySkins();
            }
        }
        else if (WasEnabled)
        {
            RestoreOriginal();
            WasEnabled = false;
        }
    }
}
