#include "GroundItemManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Navigation/NavigationManager.h"
#include <stdio.h>

namespace ShaiyaOverlay
{
    FixedList<GroundItem, 128> GroundItemManager::Items;
    FixedList<LootFilterEntry, 64> GroundItemManager::FilterList;
    AutoLootConfig GroundItemManager::Config;
    U32 GroundItemManager::LastLootTick = 0;
    static bool ConfigLoaded = false;

    void GroundItemManager::LoadConfig()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\auto_loot.ini");

        Config.Enabled = (GetPrivateProfileIntA("AutoLoot", "Enabled", 0, IniPath) != 0);
        Config.OnlyMyDrops = (GetPrivateProfileIntA("AutoLoot", "OnlyMyDrops", 1, IniPath) != 0);

        char BufRadius[32] = { 0 };
        GetPrivateProfileStringA("AutoLoot", "PickupRadius", "3.5", BufRadius, sizeof(BufRadius), IniPath);
        Config.PickupRadius = static_cast<F32>(atof(BufRadius));
        if (Config.PickupRadius <= 0.0f) Config.PickupRadius = 3.5f;

        Config.AutoWalkToLoot = (GetPrivateProfileIntA("AutoLoot", "AutoWalkToLoot", 0, IniPath) != 0);

        char BufWalk[32] = { 0 };
        GetPrivateProfileStringA("AutoLoot", "MaxWalkDistance", "25.0", BufWalk, sizeof(BufWalk), IniPath);
        Config.MaxWalkDistance = static_cast<F32>(atof(BufWalk));
        if (Config.MaxWalkDistance <= 0.0f) Config.MaxWalkDistance = 25.0f;

        Config.LootAllIgnoreFilter = (GetPrivateProfileIntA("AutoLoot", "LootAllIgnoreFilter", 0, IniPath) != 0);

        FilterList.Clear();
        char sectionBuffer[4096] = { 0 };
        DWORD bytesRead = GetPrivateProfileSectionA("LootFilterList", sectionBuffer, sizeof(sectionBuffer), IniPath);
        if (bytesRead > 0)
        {
            char* p = sectionBuffer;
            while (*p)
            {
                char* eq = strchr(p, '=');
                if (eq)
                {
                    const char* val = eq + 1;
                    if (val[0] != '\0')
                    {
                        LootFilterEntry Entry = { 0 };
                        StringUtils::Copy(Entry.Name, val, sizeof(Entry.Name));
                        StringUtils::NormalizeAccents(Entry.Name, sizeof(Entry.Name), false);
                        FilterList.Add(Entry);
                    }
                }
                p += strlen(p) + 1;
            }
        }

        ConfigLoaded = true;
    }

    void GroundItemManager::SaveConfig()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\auto_loot.ini");

        WritePrivateProfileStringA("AutoLoot", "Enabled", Config.Enabled ? "1" : "0", IniPath);
        WritePrivateProfileStringA("AutoLoot", "OnlyMyDrops", Config.OnlyMyDrops ? "1" : "0", IniPath);

        char BufRadius[32];
        sprintf_s(BufRadius, "%.1f", Config.PickupRadius);
        WritePrivateProfileStringA("AutoLoot", "PickupRadius", BufRadius, IniPath);

        WritePrivateProfileStringA("AutoLoot", "AutoWalkToLoot", Config.AutoWalkToLoot ? "1" : "0", IniPath);

        char BufWalk[32];
        sprintf_s(BufWalk, "%.1f", Config.MaxWalkDistance);
        WritePrivateProfileStringA("AutoLoot", "MaxWalkDistance", BufWalk, IniPath);

        WritePrivateProfileStringA("AutoLoot", "LootAllIgnoreFilter", Config.LootAllIgnoreFilter ? "1" : "0", IniPath);

        WritePrivateProfileStringA("LootFilterList", nullptr, nullptr, IniPath);
        for (U32 i = 0; i < FilterList.GetCount(); ++i)
        {
            char key[16];
            sprintf_s(key, "Item%u", i);
            WritePrivateProfileStringA("LootFilterList", key, FilterList[i].Name, IniPath);
        }
    }

    bool GroundItemManager::AddFilterItem(const char* Name)
    {
        if (!Name || Name[0] == '\0')
            return false;

        char Clean[64] = { 0 };
        StringUtils::Copy(Clean, Name, sizeof(Clean));
        StringUtils::NormalizeAccents(Clean, sizeof(Clean), false);

        for (U32 i = 0; i < FilterList.GetCount(); ++i)
        {
            if (StringUtils::Equals(FilterList[i].Name, Clean))
                return false;
        }

        LootFilterEntry Entry = { 0 };
        StringUtils::Copy(Entry.Name, Clean, sizeof(Entry.Name));
        bool ok = FilterList.Add(Entry);
        if (ok)
            SaveConfig();
        return ok;
    }

    bool GroundItemManager::RemoveFilterItem(U32 Index)
    {
        bool ok = FilterList.RemoveAt(Index);
        if (ok)
            SaveConfig();
        return ok;
    }

    void GroundItemManager::ClearFilterList()
    {
        FilterList.Clear();
        SaveConfig();
    }

    bool GroundItemManager::IsFilterMatching(const char* ItemName)
    {
        if (Config.LootAllIgnoreFilter)
            return true;

        if (FilterList.GetCount() == 0)
            return true;

        if (!ItemName || ItemName[0] == '\0')
            return false;

        for (U32 i = 0; i < FilterList.GetCount(); ++i)
        {
            if (StringUtils::ContainsCaseInsensitive(ItemName, FilterList[i].Name))
                return true;
        }
        return false;
    }

    bool GroundItemManager::PickUp(U32 ItemWorldId)
    {
        if (!Offsets.SendPickUpAddr || ItemWorldId == 0)
            return false;

        using tSendPickUp = __int64(__fastcall*)(U32);
        auto Fn = reinterpret_cast<tSendPickUp>(Offsets.SendPickUpAddr);
        Fn(ItemWorldId);
        return true;
    }

    const char* GroundItemManager::GetCategoryName(U8 Type)
    {
        switch (Type)
        {
        case 1:  return "1H Sword";
        case 2:  return "2H Sword";
        case 3:  return "1H Axe";
        case 4:  return "2H Axe";
        case 5:  return "Dual Weapon";
        case 6:  return "Spear";
        case 7:  return "1H Blunt";
        case 8:  return "2H Blunt";
        case 9:  return "Reverse Dagger";
        case 10: return "Bow";
        case 11: return "Crossbow";
        case 12: return "Staff";
        case 13: return "Javelin";
        case 14: return "Knuckle";
        case 15: return "Shield";
        case 16: return "Helm";
        case 17: return "Upper Armor";
        case 18: return "Lower Armor";
        case 19: return "Shield";
        case 20: return "Gloves";
        case 21: return "Boots";
        case 22: return "Ring";
        case 23: return "Necklace";
        case 24: return "Bracelet";
        case 25: return "Potion";
        case 30: return "Lapis";
        case 44: return "Gold";
        default: return "Misc";
        }
    }

    bool GroundItemManager::ResolveItemName(U8 Type, U8 TypeId, char* OutName, U32 MaxLen)
    {
        if (!OutName || MaxLen == 0)
            return false;

        if (Type == 44)
        {
            StringUtils::Copy(OutName, "Gold", MaxLen);
            return true;
        }

        // ponytail: native GetItemRecord handles FNV-1a hash map & lapis transformations; fallback formats Type-TypeId
        if (Offsets.GetItemRecordAddr && Offsets.ItemDb)
        {
            using GetItemRecordFn = U64(__fastcall*)(U64 ItemDb, U8 Type, U32 TypeId);
            auto Fn = reinterpret_cast<GetItemRecordFn>(Offsets.GetItemRecordAddr);

            __try
            {
                U64 RecPtr = Fn(Offsets.ItemDb, Type, static_cast<U32>(TypeId));
                if (RecPtr)
                {
                    U64 NamePtr = 0;
                    if (Memory::ReadSafe(RecPtr, &NamePtr) && NamePtr)
                    {
                        char TempName[64] = { 0 };
                        if (Memory::ReadBytesSafe(NamePtr, TempName, sizeof(TempName) - 1))
                        {
                            TempName[sizeof(TempName) - 1] = '\0';
                            if (TempName[0] != '\0')
                            {
                                StringUtils::Copy(OutName, TempName, MaxLen);
                                StringUtils::NormalizeAccents(OutName, MaxLen, false);
                                return true;
                            }
                        }
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        StringUtils::Format(OutName, MaxLen, "%s [%u-%u]", GetCategoryName(Type), Type, TypeId);
        return false;
    }

    void GroundItemManager::Update()
    {
        if (!ConfigLoaded)
            LoadConfig();

        Items.Clear();

        if (!Offsets.WorldManager)
            return;

        U64 ItemMapAddr = Offsets.WorldManager + Offsets.ItemMapOffset;
        U64 HeadNode = 0;
        if (!Memory::ReadSafe(ItemMapAddr + 0x10, &HeadNode) || !HeadNode)
            return;

        U64 CurrentNode = 0;
        if (!Memory::ReadSafe(HeadNode, &CurrentNode) || !CurrentNode)
            return;

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        U32 Iterations = 0;
        constexpr U32 MaxItemsScan = 128;

        while (CurrentNode && CurrentNode != HeadNode && Iterations < MaxItemsScan)
        {
            U64 NextNode = 0;
            Memory::ReadSafe(CurrentNode, &NextNode);

            U64 ItemPtr = 0;
            Memory::ReadSafe(CurrentNode + 0x18, &ItemPtr);

            if (ItemPtr)
            {
                GroundItem Item = { 0 };
                Memory::ReadSafe(ItemPtr + Offsets.ItemWorldId, &Item.WorldId);
                Memory::ReadSafe(ItemPtr + Offsets.ItemOwnerId, &Item.OwnerId);
                Memory::ReadSafe(ItemPtr + Offsets.ItemDespawnTime, &Item.DespawnTime);

                Memory::ReadSafe(ItemPtr + Offsets.ItemPosX, &Item.Position.X);
                Memory::ReadSafe(ItemPtr + Offsets.ItemPosY, &Item.Position.Y);
                Memory::ReadSafe(ItemPtr + Offsets.ItemPosZ, &Item.Position.Z);

                Memory::ReadSafe(ItemPtr + Offsets.ItemType, &Item.Type);
                Memory::ReadSafe(ItemPtr + Offsets.ItemTypeId, &Item.TypeId);
                Memory::ReadSafe(ItemPtr + Offsets.ItemCount, &Item.Count);
                Memory::ReadSafe(ItemPtr + Offsets.ItemQuality, &Item.Quality);

                if (Player.Valid)
                    Item.Distance = Item.Position.DistanceTo(Player.Position);
                else
                    Item.Distance = 0.0f;

                StringUtils::Copy(Item.Category, GetCategoryName(Item.Type), sizeof(Item.Category));

                if (Item.Type == 44)
                {
                    StringUtils::Format(Item.Name, sizeof(Item.Name), "Gold (%u)", Item.Count);
                }
                else
                {
                    ResolveItemName(Item.Type, Item.TypeId, Item.Name, sizeof(Item.Name));
                }

                Items.Add(Item);
            }

            CurrentNode = NextNode;
            ++Iterations;
        }

        // Auto-Loot / PickUp processing
        if (Config.Enabled && Player.Valid && Items.GetCount() > 0)
        {
            U32 Now = GetTickCount();
            const GroundItem* ClosestWalkTarget = nullptr;
            F32 ClosestWalkDist = 99999.0f;

            for (U32 i = 0; i < Items.GetCount(); ++i)
            {
                const GroundItem& Item = Items[i];
                if (Item.WorldId == 0)
                    continue;

                // Ownership filter: only pick up items owned by player or free-for-all
                if (Config.OnlyMyDrops && Item.OwnerId != 0 && Item.OwnerId != Player.Id)
                    continue;

                // Name filter list: only pick up items matching the filter list (or all if list is empty)
                if (!IsFilterMatching(Item.Name))
                    continue;

                // 1. Direct pickup if within pickup radius
                if (Item.Distance <= Config.PickupRadius)
                {
                    // Rate limit: 1 pickup packet every 120ms
                    if (Now - LastLootTick >= 120)
                    {
                        PickUp(Item.WorldId);
                        LastLootTick = Now;
                        Logger::Info("AutoLoot: Picked up %s (WorldId: %u, Dist: %.1fm)",
                            Item.Name, Item.WorldId, Item.Distance);
                        break;
                    }
                }
                // 2. Candidate for auto-walk if enabled and within walk distance
                else if (Config.AutoWalkToLoot && Item.Distance <= Config.MaxWalkDistance)
                {
                    if (Item.Distance < ClosestWalkDist)
                    {
                        ClosestWalkDist = Item.Distance;
                        ClosestWalkTarget = &Item;
                    }
                }
            }

            // Auto-walk to nearest drop if not already in range and not navigating elsewhere
            if (ClosestWalkTarget && !NavigationManager::IsNavigating())
            {
                NavigationManager::WalkTo(ClosestWalkTarget->Position, ClosestWalkTarget->Name, 1.5f);
            }
        }
    }
}
