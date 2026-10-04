#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct InventoryItem
    {
        U8 Bag;         // 1..5
        U8 Slot;        // 1..24
        U8 GlobalIndex; // 0..119
        U8 Type;
        U8 TypeId;
        U8 Count;
        bool IsConsumable;
        char Name[64];
    };
}
