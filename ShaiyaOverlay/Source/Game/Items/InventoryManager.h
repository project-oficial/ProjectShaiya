#pragma once

#include "InventoryItem.h"
#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class InventoryManager
    {
    public:
        static void Update();
        static const FixedList<InventoryItem, 128>& GetItems();
        static bool IsConsumableType(U8 Type);

    private:
        static FixedList<InventoryItem, 128> Items;
    };
}
