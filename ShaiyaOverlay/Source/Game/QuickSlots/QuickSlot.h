#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    enum class QuickSlotType : U8
    {
        Empty = 0,
        Item,
        Skill,
        Action
    };

    struct QuickSlotEntry
    {
        U32 SlotIndex; // 0..9
        U32 BarIndex;  // 0..2 (Bar 1, 2, 3)
        U8 RawType;
        U8 SubType;
        U16 TargetId;
        QuickSlotType Type;
        bool Active;
        char Name[64];
        char Details[64];
    };
}
