#pragma once

#include "Core/Types.h"
#include "GroundItem.h"

namespace ShaiyaOverlay
{
    class GroundItemManager
    {
    public:
        static void Update();

        static const FixedList<GroundItem, 128>& GetGroundItems() { return Items; }
        static U32 GetItemCount() { return Items.GetCount(); }

        static bool ResolveItemName(U8 Type, U8 TypeId, char* OutName, U32 MaxLen);
        static const char* GetCategoryName(U8 Type);

    private:
        static FixedList<GroundItem, 128> Items;
    };
}
