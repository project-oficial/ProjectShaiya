#pragma once

#include "Core/Types.h"
#include "Quest.h"

namespace ShaiyaOverlay
{
    class QuestManager
    {
    public:
        static void Update();

        static const FixedList<ActiveQuest, 16>& GetActiveQuests() { return Quests; }
        static U32 GetQuestCount() { return Quests.GetCount(); }

        static const FixedList<QuestMarker, 32>& GetQuestMarkers() { return Markers; }
        static U32 GetMarkerCount() { return Markers.GetCount(); }

        static bool FindNpcPosition(U32 NpcId, Vector3& OutPos);
        static U32 CountInventoryItem(U8 ItemType, U8 ItemTypeId);
        static bool ExtractTagFromDescription(const char* Desc, char* OutTag, U32 MaxLen);
        static U32 ExtractAllTagsFromDescription(const char* Desc, char OutTags[4][64]);

    private:
        static void UpdateActiveQuests();
        static void UpdateRadarMarkers();

        static FixedList<ActiveQuest, 16> Quests;
        static FixedList<QuestMarker, 32> Markers;
    };
}
