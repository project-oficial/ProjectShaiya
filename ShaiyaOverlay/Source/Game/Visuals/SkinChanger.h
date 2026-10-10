#pragma once

#include "Core/Types.h"
#include <vector>

namespace ShaiyaOverlay
{
    struct SkinItemInfo
    {
        U8 TypeId = 0;
        char Name[64] = { 0 };
    };

    struct EquipmentVisualSlot
    {
        bool Override = false;
        U8 Type = 0;
        U8 TypeId = 0;
    };

    struct SkinChangerConfig
    {
        bool Enabled = false;

        // Costume / Transformation (Type 150)
        bool OverrideCostume = false;
        U8 CostumeTypeId = 0; // 93 = Dragão Negro, 52 = Lâmina de Ouro, 45 = O Corvo, 67 = Dracula, 19 = Urso

        // Wings (Type 121)
        bool OverrideWings = false;
        U8 WingsTypeId = 0; // 1 = Anjo, 4 = Demônio, 5 = Thor, 6 = Loki

        // Weapons Glow
        bool OverrideGlow = false;
        U8 Weapon1Glow = 20; // 0..20
        U8 Weapon2Glow = 20; // 0..20

        // Armor parts
        EquipmentVisualSlot Helmet;
        EquipmentVisualSlot Upper;
        EquipmentVisualSlot Lower;
        EquipmentVisualSlot Gloves;
        EquipmentVisualSlot Boots;

        // Weapons
        EquipmentVisualSlot Weapon1;
        EquipmentVisualSlot Weapon2;
        EquipmentVisualSlot Mount;

        // Hair & Face
        bool OverrideHair = false;
        U8 Hair = 0;
        bool OverrideFace = false;
        U8 Face = 0;
    };

    struct CharacterVisualBackup
    {
        bool Saved = false;
        U8 HelmetType, HelmetTypeId;
        U8 UpperType, UpperTypeId;
        U8 LowerType, LowerTypeId;
        U8 GlovesType, GlovesTypeId;
        U8 BootsType, BootsTypeId;
        U8 Weapon1Type, Weapon1TypeId;
        U8 Weapon2Type, Weapon2TypeId;
        U8 WingsType, WingsTypeId;
        U8 CostumeType, CostumeTypeId;
        U8 MountType, MountTypeId;
        U8 Weapon1Glow, Weapon2Glow;
        U8 Hair, Face;
    };

    class SkinChanger
    {
    public:
        static void Update();

        static SkinChangerConfig& GetConfig() { return Config; }
        static void ApplySkins();
        static void RestoreOriginal();

        static void SaveOriginalAppearance(U64 LocalPlayerPtr);
        static void SetTransformation(int costumeTypeId, int wingsTypeId = 0, int glowLevel = 20);
        static void SetWings(int wingsTypeId);
        static void SetGlowPreset(int glowLevel);
        static void SetArmorPreset(int presetIndex);
        static U64 GetLocalPlayerCharacterPtr();

        static const std::vector<SkinItemInfo>& GetAvailableCostumes();
        static const std::vector<SkinItemInfo>& GetAvailableWings();
        static void EnsureSkinCatalogLoaded();

    private:
        static SkinChangerConfig Config;
        static CharacterVisualBackup Backup;
        static U32 LastAppliedTick;
    };
}
