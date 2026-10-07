#pragma once

#include "Core/Types.h"
#include "Game/Items/InventoryItem.h"

namespace ShaiyaOverlay
{
    struct AutoHealConfig
    {
        bool Enabled = true;

        bool AutoHpEnabled = true;
        F32 HpThresholdPercent = 80.0f;
        char SelectedHpItem[64] = "Auto"; // "Auto" or specific name (e.g. "Romã", "Banana Fresca", "Maçã")

        bool AutoMpEnabled = true;
        F32 MpThresholdPercent = 40.0f;
        char SelectedMpItem[64] = "Auto";

        bool AutoSpEnabled = false;
        F32 SpThresholdPercent = 30.0f;
        char SelectedSpItem[64] = "Auto";

        U32 PotionCooldownMs = 1500;
    };

    class HealManager
    {
    public:
        static void Update();

        static AutoHealConfig& GetConfig() { return Config; }
        static void SaveConfig();

        static bool IsHpItem(const InventoryItem& it);
        static bool IsMpItem(const InventoryItem& it);
        static bool IsSpItem(const InventoryItem& it);

        static bool FindBestHpItem(InventoryItem* OutItem);
        static bool FindBestMpItem(InventoryItem* OutItem);
        static bool FindBestSpItem(InventoryItem* OutItem);

        static void GetAvailableHpItems(FixedList<InventoryItem, 16>& OutList);
        static void GetAvailableMpItems(FixedList<InventoryItem, 16>& OutList);
        static void GetAvailableSpItems(FixedList<InventoryItem, 16>& OutList);

        static U32 GetLastHpUseTick() { return LastUsedHpTick; }
        static U32 GetLastMpUseTick() { return LastUsedMpTick; }
        static U32 GetLastSpUseTick() { return LastUsedSpTick; }
        static const char* GetLastHealAction() { return LastHealAction; }

        static bool TestHealNow(const char* Type = "HP");

    private:
        static void LoadConfig();

        static AutoHealConfig Config;
        static bool ConfigLoaded;

        static U32 LastPotionTick;
        static U32 LastUsedHpTick;
        static U32 LastUsedMpTick;
        static U32 LastUsedSpTick;
        static char LastHealAction[96];
    };
}
