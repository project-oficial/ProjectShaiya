#pragma once

#include "Core/Types.h"
#include "Entity.h"
#include "LocalPlayer.h"

namespace ShaiyaOverlay
{
    class EntityManager
    {
    public:
        static void Update();

        static const PlayerData& GetLocalPlayer() { return CurrentPlayer; }
        static const FixedList<MonsterEntity, 128>& GetNearbyMonsters() { return NearbyMonsters; }
        static U32 GetTotalMonstersInRange() { return NearbyMonsters.GetCount(); }

        static bool ResolveMonsterInfo(U16 MobId, char* OutName, U32 MaxLen, U16* OutLevel = nullptr);
        static U16 FindMobIdByMatchingName(const char* NameQuery, char* OutFullName = nullptr, U32 MaxLen = 0);

    private:
        static void UpdateLocalPlayer(U64 ImageBase);
        static void UpdateMonsters(U64 ImageBase);

        static PlayerData CurrentPlayer;
        static FixedList<MonsterEntity, 128> NearbyMonsters;
    };
}
