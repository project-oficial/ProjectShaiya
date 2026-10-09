#pragma once

#include "Core/Types.h"
#include "Quest.h"

namespace ShaiyaOverlay
{
    struct SavedQuestMob
    {
        U16 QuestId;
        U16 MobId;
        Vector3 Position;
        char MobName[64];
    };

    class QuestManager
    {
    public:
        static void Update();

        static const FixedList<ActiveQuest, 16>& GetActiveQuests() { return Quests; }
        static U32 GetQuestCount() { return Quests.GetCount(); }

        static const FixedList<QuestMarker, 32>& GetQuestMarkers() { return Markers; }
        static U32 GetMarkerCount() { return Markers.GetCount(); }

        static bool FindNpcPosition(U32 NpcId, Vector3& OutPos);
        static bool FindRadarNpcPosition(U8 NpcType, U16 NpcId, Vector3& OutPos);
        static bool FindQuestStartNpc(U16 QuestId, U8& OutType, U16& OutId, Vector3& OutPos, char* OutName, U32 MaxLen);
        static U32 CountInventoryItem(U8 ItemType, U8 ItemTypeId);
        static bool ExtractTagFromDescription(const char* Desc, char* OutTag, U32 MaxLen);
        static U32 ExtractAllTagsFromDescription(const char* Desc, char OutTags[4][64]);

        static bool GetSavedMobPosition(U16 QuestId, U16 MobId, Vector3& OutPos, char* OutName = nullptr, U32 MaxLen = 0);
        static void SaveMobPosition(U16 QuestId, U16 MobId, const Vector3& Pos, const char* MobName);

    private:
        static void UpdateActiveQuests();
        static void UpdateRadarMarkers();
        static void AutoRecordNearbyQuestMobs();
        static void LoadMobCache();

        static FixedList<ActiveQuest, 16> Quests;
        static FixedList<QuestMarker, 32> Markers;
        static FixedList<SavedQuestMob, 128> SavedMobCache;
        static bool MobCacheLoaded;
    };
}
