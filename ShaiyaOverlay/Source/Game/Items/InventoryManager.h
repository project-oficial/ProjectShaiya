#pragma once

#include "InventoryItem.h"
#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class InventoryManager
    {
    public:
        static void Update();
        static const FixedList<InventoryItem, 256>& GetItems();
        static bool IsConsumableType(U8 Type);
        static bool UseItem(U8 Bag, U8 Slot);

    private:
        static FixedList<InventoryItem, 256> Items;
    };
}
