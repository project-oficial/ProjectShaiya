#include "InventoryManager.h"
#include "GroundItemManager.h"
#include "Game/GameOffsets.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"

namespace ShaiyaOverlay
{
    FixedList<InventoryItem, 256> InventoryManager::Items;

    bool InventoryManager::IsConsumableType(U8 Type)
    {
        // Shaiya Consumable Item Categories:
        // 25: Potions, Foods, Drinks, Recovery Items (HP/MP/SP)
        // 27: Teleport Runes, Movement Runes, Gate Stones
        // 28: Special Consumables
        // 29: Special Consumables
        // 31: Town Return Scrolls, Teleport Scrolls
        // 38: Mystery Boxes, Gift Packages, Lucky Pouches (Right-click to open)
        // 42: Mounts (Right-click to summon/ride)
        // 43: Pets (Right-click to summon)
        // 90..110: Event/Special Consumables, EXP Buffs, Service Stones, Resurrection Runes
        if (Type == 25 || Type == 27 || Type == 28 || Type == 29 ||
            Type == 31 || Type == 38 || Type == 42 || Type == 43)
        {
            return true;
        }

        if (Type >= 90 && Type <= 110)
        {
            return true;
        }

        return false;
    }

    void InventoryManager::Update()
    {
        Items.Clear();

        if (!Offsets.PlayerInventory)
            return;

        // Player inventory bags start at PlayerInventoryBagsOffset in PlayerInventory block
        // 5 bags, each having 48 slots (48 * 132 bytes = 6336 bytes per bag)
        U64 SlotStart = Offsets.PlayerInventory + Offsets.PlayerInventoryBagsOffset;

        constexpr U32 TotalSlots = 240; // 5 bags * 48 slots
        for (U32 I = 0; I < TotalSlots; ++I)
        {
            U8 Data[4] = { 0 };
            if (!Memory::ReadBytesSafe(SlotStart + I * Offsets.InventorySlotStride, Data, 3))
                continue;

            U8 Type = Data[0];
            U8 TypeId = Data[1];
            U8 Count = Data[2];

            if (Type == 0 || Count == 0)
                continue;

            InventoryItem Item = { 0 };
            Item.Bag = static_cast<U8>((I / 48) + 1);
            Item.Slot = static_cast<U8>((I % 48) + 1);
            Item.GlobalIndex = static_cast<U8>(I);
            Item.Type = Type;
            Item.TypeId = TypeId;
            Item.Count = Count;
            Item.IsConsumable = IsConsumableType(Type);

            if (!GroundItemManager::ResolveItemName(Type, TypeId, Item.Name, sizeof(Item.Name)))
            {
                StringUtils::Format(Item.Name, sizeof(Item.Name), "Item [%u-%u]", Type, TypeId);
            }
            StringUtils::NormalizeAccents(Item.Name, sizeof(Item.Name), false);

            if (Offsets.GetItemRecordAddr && Offsets.ItemDb)
            {
                using GetItemRecordFn = U64(__fastcall*)(U64, U8, U32);
                auto Fn = reinterpret_cast<GetItemRecordFn>(Offsets.GetItemRecordAddr);
                __try
                {
                    U64 RecPtr = Fn(Offsets.ItemDb, Type, static_cast<U32>(TypeId));
                    if (RecPtr)
                    {
                        Memory::ReadSafe(RecPtr + Offsets.ItemRecordHpRecovery, &Item.HpRecovery);
                        Memory::ReadSafe(RecPtr + Offsets.ItemRecordMpRecovery, &Item.MpRecovery);
                        Memory::ReadSafe(RecPtr + Offsets.ItemRecordSpRecovery, &Item.SpRecovery);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }

            Items.Add(Item);
        }
    }

    bool InventoryManager::UseItem(U8 Bag, U8 Slot)
    {
        if (!Offsets.SendUseItemAddr || Bag < 1 || Bag > 5 || Slot < 1 || Slot > 48)
            return false;

        using tSendPacketUseItem = void(__fastcall*)(U8, U8);
        auto Fn = reinterpret_cast<tSendPacketUseItem>(Offsets.SendUseItemAddr);

        // Bag is 1-based (1..5), engine Slot is 0-based (0..47)
        Fn(Bag, Slot - 1);
        return true;
    }

    const FixedList<InventoryItem, 256>& InventoryManager::GetItems()
    {
        return Items;
    }
}
