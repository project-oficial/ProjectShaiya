#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct QuestObjective
    {
        U16 TargetMobId;
        U16 CountNeeded;
        U16 CurrentCount;
        bool Completed;
        char TargetMobName[64];
    };

    struct QuestItemObjective
    {
        U8 ItemType;
        U8 ItemTypeId;
        U8 CountNeeded;
        U8 CurrentCount;
        bool Completed;
        char ItemName[64];
        U16 DroppedByMobId;
        char DroppedByMobName[64];
    };

    struct ActiveQuest
    {
        U16 QuestId;
        U8 Step;
        U32 EndNpcId;
        Vector3 DestinationPos;
        bool HasDestination;
        F32 Distance;
        QuestObjective Objectives[3];
        U32 ObjectiveCount;
        QuestItemObjective ItemObjectives[3];
        U32 ItemObjectiveCount;
        char Title[64];
        char DestinationName[64];
    };

    struct QuestMarker
    {
        Vector3 Position;
        F32 Distance;
        bool IsTurnIn; // true = Turn-in (?), false = Start/Available (!)
        U16 QuestId;
        char NpcName[64];
    };
}
