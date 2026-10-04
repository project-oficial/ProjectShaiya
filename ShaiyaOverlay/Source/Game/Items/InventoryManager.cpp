#include "InventoryManager.h"
#include "GroundItemManager.h"
#include "Game/GameOffsets.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"

namespace ShaiyaOverlay
{
    FixedList<InventoryItem, 128> InventoryManager::Items;

    bool InventoryManager::IsConsumableType(U8 Type)
    {
        // Shaiya Consumable Item Categories:
        // 25: Potions, Foods, Drinks, Recovery Items (HP/MP/SP)
        // 27: Teleport Runes, Movement Runes, Gate Stones
        // 28: Special Consumables
        // 29: Special Consumables
        // 31: Town Return Scrolls, Teleport Scrolls
        // 32: Resurrection Runes, Summoning Runes
        // 38: Mystery Boxes, Gift Packages, Lucky Pouches (Right-click to open)
        // 42: Mounts (Right-click to summon/ride)
        // 43: Pets (Right-click to summon)
        // 90..100: Event/Special Consumables
        if (Type == 25 || Type == 27 || Type == 28 || Type == 29 ||
            Type == 31 || Type == 32 || Type == 38 || Type == 42 || Type == 43)
        {
            return true;
        }

        if (Type >= 90 && Type <= 100)
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

        // Player inventory bags start at +17521 in PlayerInventory block
        U64 SlotStart = Offsets.PlayerInventory + 17521;

        constexpr U32 TotalSlots = 120; // 5 bags * 24 slots
        for (U32 I = 0; I < TotalSlots; ++I)
        {
            U8 Data[4] = { 0 };
            if (!Memory::ReadBytesSafe(SlotStart + I * 132, Data, 3))
                continue;

            U8 Type = Data[0];
            U8 TypeId = Data[1];
            U8 Count = Data[2];

            if (Type == 0 || Count == 0)
                continue;

            InventoryItem Item = { 0 };
            Item.Bag = static_cast<U8>((I / 24) + 1);
            Item.Slot = static_cast<U8>((I % 24) + 1);
            Item.GlobalIndex = static_cast<U8>(I);
            Item.Type = Type;
            Item.TypeId = TypeId;
            Item.Count = Count;
            Item.IsConsumable = IsConsumableType(Type);

            if (!GroundItemManager::ResolveItemName(Type, TypeId, Item.Name, sizeof(Item.Name)))
            {
                StringUtils::Format(Item.Name, sizeof(Item.Name), "Item [%u-%u]", Type, TypeId);
            }

            Items.Add(Item);
        }
    }

    const FixedList<InventoryItem, 128>& InventoryManager::GetItems()
    {
        return Items;
    }
}
