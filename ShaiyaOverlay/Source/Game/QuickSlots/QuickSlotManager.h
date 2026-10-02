#pragma once

#include "Core/Types.h"
#include "QuickSlot.h"

namespace ShaiyaOverlay
{
    class QuickSlotManager
    {
    public:
        static void Update();

        static const FixedList<QuickSlotEntry, 30>& GetSlots() { return Slots; }
        static U32 GetTotalSlots() { return Slots.GetCount(); }

        static const char* GetActionName(U16 ActionId);

    private:
        static FixedList<QuickSlotEntry, 30> Slots;
    };
}
