#include "HealManager.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Items/InventoryManager.h"
#include "Core/Logger.h"
#include "Core/StringUtils.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ShaiyaOverlay
{
    AutoHealConfig HealManager::Config;
    bool HealManager::ConfigLoaded = false;
    U32 HealManager::LastPotionTick = 0;
    U32 HealManager::LastUsedHpTick = 0;
    U32 HealManager::LastUsedMpTick = 0;
    U32 HealManager::LastUsedSpTick = 0;
    char HealManager::LastHealAction[96] = { 0 };

    static bool StrContainsCaseInsensitive(const char* Haystack, const char* Needle)
    {
        if (!Haystack || !Needle) return false;
        char LowerH[128], LowerN[128];
        StringUtils::Copy(LowerH, Haystack, sizeof(LowerH));
        StringUtils::Copy(LowerN, Needle, sizeof(LowerN));
        _strlwr_s(LowerH);
        _strlwr_s(LowerN);
        return strstr(LowerH, LowerN) != nullptr;
    }

    void HealManager::LoadConfig()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\auto_heal.ini");

        Config.Enabled = GetPrivateProfileIntA("AutoHeal", "Enabled", 1, IniPath) != 0;
        Config.AutoHpEnabled = GetPrivateProfileIntA("AutoHeal", "AutoHpEnabled", 1, IniPath) != 0;
        Config.AutoMpEnabled = GetPrivateProfileIntA("AutoHeal", "AutoMpEnabled", 1, IniPath) != 0;
        Config.AutoSpEnabled = GetPrivateProfileIntA("AutoHeal", "AutoSpEnabled", 0, IniPath) != 0;

        char Buf[64];
        GetPrivateProfileStringA("AutoHeal", "HpThresholdPercent", "80.0", Buf, sizeof(Buf), IniPath);
        Config.HpThresholdPercent = static_cast<F32>(atof(Buf));

        GetPrivateProfileStringA("AutoHeal", "SelectedHpItem", "Auto", Config.SelectedHpItem, sizeof(Config.SelectedHpItem), IniPath);
        StringUtils::NormalizeAccents(Config.SelectedHpItem, sizeof(Config.SelectedHpItem), false);

        GetPrivateProfileStringA("AutoHeal", "MpThresholdPercent", "40.0", Buf, sizeof(Buf), IniPath);
        Config.MpThresholdPercent = static_cast<F32>(atof(Buf));

        GetPrivateProfileStringA("AutoHeal", "SelectedMpItem", "Auto", Config.SelectedMpItem, sizeof(Config.SelectedMpItem), IniPath);
        StringUtils::NormalizeAccents(Config.SelectedMpItem, sizeof(Config.SelectedMpItem), false);

        GetPrivateProfileStringA("AutoHeal", "SpThresholdPercent", "30.0", Buf, sizeof(Buf), IniPath);
        Config.SpThresholdPercent = static_cast<F32>(atof(Buf));

        GetPrivateProfileStringA("AutoHeal", "SelectedSpItem", "Auto", Config.SelectedSpItem, sizeof(Config.SelectedSpItem), IniPath);
        StringUtils::NormalizeAccents(Config.SelectedSpItem, sizeof(Config.SelectedSpItem), false);

        Config.PotionCooldownMs = GetPrivateProfileIntA("AutoHeal", "PotionCooldownMs", 1500, IniPath);

        ConfigLoaded = true;
    }

    void HealManager::SaveConfig()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\auto_heal.ini");

        WritePrivateProfileStringA("AutoHeal", "Enabled", Config.Enabled ? "1" : "0", IniPath);
        WritePrivateProfileStringA("AutoHeal", "AutoHpEnabled", Config.AutoHpEnabled ? "1" : "0", IniPath);
        WritePrivateProfileStringA("AutoHeal", "AutoMpEnabled", Config.AutoMpEnabled ? "1" : "0", IniPath);
        WritePrivateProfileStringA("AutoHeal", "AutoSpEnabled", Config.AutoSpEnabled ? "1" : "0", IniPath);

        char Buf[32];
        sprintf_s(Buf, "%.1f", Config.HpThresholdPercent);
        WritePrivateProfileStringA("AutoHeal", "HpThresholdPercent", Buf, IniPath);
        WritePrivateProfileStringA("AutoHeal", "SelectedHpItem", Config.SelectedHpItem, IniPath);

        sprintf_s(Buf, "%.1f", Config.MpThresholdPercent);
        WritePrivateProfileStringA("AutoHeal", "MpThresholdPercent", Buf, IniPath);
        WritePrivateProfileStringA("AutoHeal", "SelectedMpItem", Config.SelectedMpItem, IniPath);

        sprintf_s(Buf, "%.1f", Config.SpThresholdPercent);
        WritePrivateProfileStringA("AutoHeal", "SpThresholdPercent", Buf, IniPath);
        WritePrivateProfileStringA("AutoHeal", "SelectedSpItem", Config.SelectedSpItem, IniPath);

        sprintf_s(Buf, "%u", Config.PotionCooldownMs);
        WritePrivateProfileStringA("AutoHeal", "PotionCooldownMs", Buf, IniPath);
    }

    bool HealManager::IsHpItem(const InventoryItem& it)
    {
        if (it.Type != 25 || it.Count == 0) return false;

        // Explicit known HP food in Shaiya: Romã, Banana Fresca, Maçã (standard), Fruta de Sangue
        if (StrContainsCaseInsensitive(it.Name, "Romã") ||
            StrContainsCaseInsensitive(it.Name, "Roma") ||
            StrContainsCaseInsensitive(it.Name, "Banana Fresca") ||
            StrContainsCaseInsensitive(it.Name, "Fruta de Sangue") ||
            (StrContainsCaseInsensitive(it.Name, "Maçã") && !StrContainsCaseInsensitive(it.Name, "Verde") && !StrContainsCaseInsensitive(it.Name, "Ouro")) ||
            (StrContainsCaseInsensitive(it.Name, "Maca") && !StrContainsCaseInsensitive(it.Name, "Verde") && !StrContainsCaseInsensitive(it.Name, "Ouro")))
        {
            return true;
        }

        // Native HpRecovery field
        if (it.HpRecovery > 0)
            return true;

        // Potion name keywords
        if (it.MpRecovery == 0 && it.SpRecovery == 0)
        {
            return StrContainsCaseInsensitive(it.Name, "HP") ||
                   StrContainsCaseInsensitive(it.Name, "Vida") ||
                   StrContainsCaseInsensitive(it.Name, "Health");
        }

        return false;
    }

    bool HealManager::IsMpItem(const InventoryItem& it)
    {
        if (it.Type != 25 || it.Count == 0) return false;

        // Explicit known MP food in Shaiya: Banana Dourada, Banana Vigorosa, Maçã de Ouro
        if (StrContainsCaseInsensitive(it.Name, "Banana Dourada") ||
            StrContainsCaseInsensitive(it.Name, "Banana Vigorosa") ||
            StrContainsCaseInsensitive(it.Name, "Maçã de Ouro") ||
            StrContainsCaseInsensitive(it.Name, "Maca de Ouro"))
        {
            return true;
        }

        if (it.MpRecovery > 0)
            return true;

        if (it.HpRecovery == 0 && it.SpRecovery == 0)
        {
            return StrContainsCaseInsensitive(it.Name, "MP") ||
                   StrContainsCaseInsensitive(it.Name, "Mana") ||
                   StrContainsCaseInsensitive(it.Name, "Mente");
        }

        return false;
    }

    bool HealManager::IsSpItem(const InventoryItem& it)
    {
        if (it.Type != 25 || it.Count == 0) return false;

        // Explicit known SP food in Shaiya: Banana Verde, Banana Espírito, Maçã Verde
        if (StrContainsCaseInsensitive(it.Name, "Banana Verde") ||
            StrContainsCaseInsensitive(it.Name, "Banana Espírito") ||
            StrContainsCaseInsensitive(it.Name, "Banana Espirito") ||
            StrContainsCaseInsensitive(it.Name, "Maçã Verde") ||
            StrContainsCaseInsensitive(it.Name, "Maca Verde"))
        {
            return true;
        }

        if (it.SpRecovery > 0)
            return true;

        if (it.HpRecovery == 0 && it.MpRecovery == 0)
        {
            return StrContainsCaseInsensitive(it.Name, "SP") ||
                   StrContainsCaseInsensitive(it.Name, "Stamina") ||
                   StrContainsCaseInsensitive(it.Name, "Vigor");
        }

        return false;
    }

    void HealManager::GetAvailableHpItems(FixedList<InventoryItem, 16>& OutList)
    {
        OutList.Clear();
        const auto& items = InventoryManager::GetItems();
        for (U32 i = 0; i < items.GetCount(); ++i)
        {
            if (IsHpItem(items[i]))
            {
                OutList.Add(items[i]);
            }
        }
    }

    void HealManager::GetAvailableMpItems(FixedList<InventoryItem, 16>& OutList)
    {
        OutList.Clear();
        const auto& items = InventoryManager::GetItems();
        for (U32 i = 0; i < items.GetCount(); ++i)
        {
            if (IsMpItem(items[i]))
            {
                OutList.Add(items[i]);
            }
        }
    }

    void HealManager::GetAvailableSpItems(FixedList<InventoryItem, 16>& OutList)
    {
        OutList.Clear();
        const auto& items = InventoryManager::GetItems();
        for (U32 i = 0; i < items.GetCount(); ++i)
        {
            if (IsSpItem(items[i]))
            {
                OutList.Add(items[i]);
            }
        }
    }

    bool HealManager::FindBestHpItem(InventoryItem* OutItem)
    {
        const auto& items = InventoryManager::GetItems();

        // 1. If user selected a specific item by name, prioritize it if in stock
        if (strcmp(Config.SelectedHpItem, "Auto") != 0 && Config.SelectedHpItem[0] != '\0')
        {
            for (U32 i = 0; i < items.GetCount(); ++i)
            {
                if (IsHpItem(items[i]) && StrContainsCaseInsensitive(items[i].Name, Config.SelectedHpItem))
                {
                    if (OutItem) *OutItem = items[i];
                    return true;
                }
            }
        }

        // 2. Otherwise pick item with highest recovery (or first available)
        InventoryItem Best = { 0 };
        bool Found = false;

        for (U32 i = 0; i < items.GetCount(); ++i)
        {
            const auto& it = items[i];
            if (!IsHpItem(it)) continue;

            if (!Found || it.HpRecovery > Best.HpRecovery)
            {
                Best = it;
                Found = true;
            }
        }

        if (Found && OutItem)
            *OutItem = Best;

        return Found;
    }

    bool HealManager::FindBestMpItem(InventoryItem* OutItem)
    {
        const auto& items = InventoryManager::GetItems();

        if (strcmp(Config.SelectedMpItem, "Auto") != 0 && Config.SelectedMpItem[0] != '\0')
        {
            for (U32 i = 0; i < items.GetCount(); ++i)
            {
                if (IsMpItem(items[i]) && StrContainsCaseInsensitive(items[i].Name, Config.SelectedMpItem))
                {
                    if (OutItem) *OutItem = items[i];
                    return true;
                }
            }
        }

        InventoryItem Best = { 0 };
        bool Found = false;

        for (U32 i = 0; i < items.GetCount(); ++i)
        {
            const auto& it = items[i];
            if (!IsMpItem(it)) continue;

            if (!Found || it.MpRecovery > Best.MpRecovery)
            {
                Best = it;
                Found = true;
            }
        }

        if (Found && OutItem)
            *OutItem = Best;

        return Found;
    }

    bool HealManager::FindBestSpItem(InventoryItem* OutItem)
    {
        const auto& items = InventoryManager::GetItems();

        if (strcmp(Config.SelectedSpItem, "Auto") != 0 && Config.SelectedSpItem[0] != '\0')
        {
            for (U32 i = 0; i < items.GetCount(); ++i)
            {
                if (IsSpItem(items[i]) && StrContainsCaseInsensitive(items[i].Name, Config.SelectedSpItem))
                {
                    if (OutItem) *OutItem = items[i];
                    return true;
                }
            }
        }

        InventoryItem Best = { 0 };
        bool Found = false;

        for (U32 i = 0; i < items.GetCount(); ++i)
        {
            const auto& it = items[i];
            if (!IsSpItem(it)) continue;

            if (!Found || it.SpRecovery > Best.SpRecovery)
            {
                Best = it;
                Found = true;
            }
        }

        if (Found && OutItem)
            *OutItem = Best;

        return Found;
    }

    bool HealManager::TestHealNow(const char* Type)
    {
        InventoryItem item = { 0 };
        bool found = false;
        if (strcmp(Type, "MP") == 0)
            found = FindBestMpItem(&item);
        else if (strcmp(Type, "SP") == 0)
            found = FindBestSpItem(&item);
        else
            found = FindBestHpItem(&item);

        if (!found)
            return false;

        if (InventoryManager::UseItem(item.Bag, item.Slot))
        {
            U32 now = GetTickCount();
            LastPotionTick = now;
            if (strcmp(Type, "MP") == 0) LastUsedMpTick = now;
            else if (strcmp(Type, "SP") == 0) LastUsedSpTick = now;
            else LastUsedHpTick = now;

            StringUtils::Format(LastHealAction, sizeof(LastHealAction), "Used %s (%ux) [Bag %u, Slot %u]",
                item.Name, item.Count, item.Bag, item.Slot);
            Logger::Info("AutoHeal: Manual test triggered. %s", LastHealAction);
            return true;
        }

        return false;
    }

    void HealManager::Update()
    {
        if (!ConfigLoaded)
            LoadConfig();

        if (!Config.Enabled)
            return;

        const auto& player = EntityManager::GetLocalPlayer();
        // As long as player has ID and MaxHp > 0, player stats are valid
        if (player.Id == 0 || player.MaxHp == 0)
            return;

        U32 now = GetTickCount();
        if (now - LastPotionTick < Config.PotionCooldownMs)
            return;

        // 1. HP Check (Highest Priority)
        if (Config.AutoHpEnabled && player.MaxHp > 0)
        {
            F32 hpPct = (static_cast<F32>(player.CurrentHp) / static_cast<F32>(player.MaxHp)) * 100.0f;
            if (hpPct <= Config.HpThresholdPercent)
            {
                InventoryItem hpItem = { 0 };
                if (FindBestHpItem(&hpItem))
                {
                    if (InventoryManager::UseItem(hpItem.Bag, hpItem.Slot))
                    {
                        LastPotionTick = now;
                        LastUsedHpTick = now;
                        StringUtils::Format(LastHealAction, sizeof(LastHealAction), "HP: Used %s (%.0f%% <= %.0f%%)",
                            hpItem.Name, hpPct, Config.HpThresholdPercent);
                        Logger::Info("AutoHeal: HP at %.1f%% (<= %.1f%%). Used %s (Bag %u, Slot %u)",
                            hpPct, Config.HpThresholdPercent, hpItem.Name, hpItem.Bag, hpItem.Slot);
                        return;
                    }
                }
            }
        }

        // 2. MP Check
        if (Config.AutoMpEnabled && player.MaxMp > 0)
        {
            F32 mpPct = (static_cast<F32>(player.CurrentMp) / static_cast<F32>(player.MaxMp)) * 100.0f;
            if (mpPct <= Config.MpThresholdPercent)
            {
                InventoryItem mpItem = { 0 };
                if (FindBestMpItem(&mpItem))
                {
                    if (InventoryManager::UseItem(mpItem.Bag, mpItem.Slot))
                    {
                        LastPotionTick = now;
                        LastUsedMpTick = now;
                        StringUtils::Format(LastHealAction, sizeof(LastHealAction), "MP: Used %s (%.0f%% <= %.0f%%)",
                            mpItem.Name, mpPct, Config.MpThresholdPercent);
                        Logger::Info("AutoHeal: MP at %.1f%% (<= %.1f%%). Used %s (Bag %u, Slot %u)",
                            mpPct, Config.MpThresholdPercent, mpItem.Name, mpItem.Bag, mpItem.Slot);
                        return;
                    }
                }
            }
        }

        // 3. SP Check
        if (Config.AutoSpEnabled && player.MaxSp > 0)
        {
            F32 spPct = (static_cast<F32>(player.CurrentSp) / static_cast<F32>(player.MaxSp)) * 100.0f;
            if (spPct <= Config.SpThresholdPercent)
            {
                InventoryItem spItem = { 0 };
                if (FindBestSpItem(&spItem))
                {
                    if (InventoryManager::UseItem(spItem.Bag, spItem.Slot))
                    {
                        LastPotionTick = now;
                        LastUsedSpTick = now;
                        StringUtils::Format(LastHealAction, sizeof(LastHealAction), "SP: Used %s (%.0f%% <= %.0f%%)",
                            spItem.Name, spPct, Config.SpThresholdPercent);
                        Logger::Info("AutoHeal: SP at %.1f%% (<= %.1f%%). Used %s (Bag %u, Slot %u)",
                            spPct, Config.SpThresholdPercent, spItem.Name, spItem.Bag, spItem.Slot);
                        return;
                    }
                }
            }
        }
    }
}
