#pragma once

#include "Core/Types.h"
#include "GroundItem.h"

namespace ShaiyaOverlay
{
    struct AutoLootConfig
    {
        bool Enabled = false;
        bool OnlyMyDrops = true;
        F32 PickupRadius = 3.5f;
        bool AutoWalkToLoot = false;
        F32 MaxWalkDistance = 25.0f;
    };

    class GroundItemManager
    {
    public:
        static void Update();

        static bool PickUp(U32 ItemWorldId);

        static const FixedList<GroundItem, 128>& GetGroundItems() { return Items; }
        static U32 GetItemCount() { return Items.GetCount(); }

        static bool ResolveItemName(U8 Type, U8 TypeId, char* OutName, U32 MaxLen);
        static const char* GetCategoryName(U8 Type);

        static AutoLootConfig& GetConfig() { return Config; }

    private:
        static FixedList<GroundItem, 128> Items;
        static AutoLootConfig Config;
        static U32 LastLootTick;
    };
}
