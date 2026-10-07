#pragma once

#include "Core/Types.h"
#include "Game/Entities/Entity.h"

namespace ShaiyaOverlay
{
    enum class GrindBotState : U8
    {
        Idle = 0,
        SearchingTarget,
        Approaching,
        Combat,
        Looting,
        Resting,
        ReturningHome
    };

    struct GrindBotConfig
    {
        bool Enabled = false;
        bool QuestMonstersOnly = false;
        F32 LeashRadius = 35.0f;
        F32 CombatApproachDistance = 18.0f;
        F32 RestHpThresholdPercent = 35.0f;
        Vector3 AnchorPosition = { 0.0f, 0.0f, 0.0f };
        bool HasAnchor = false;
    };

    struct GrindBotStats
    {
        U32 MonstersKilled = 0;
        U32 ItemsLooted = 0;
        U32 SessionStartTick = 0;
    };

    class GrindBot
    {
    public:
        static void Update();

        static GrindBotConfig& GetConfig() { return Config; }
        static const GrindBotStats& GetStats() { return Stats; }
        static GrindBotState GetState() { return State; }
        static const char* GetStateName();

        static void SetAnchor(const Vector3& Pos);
        static void ClearAnchor();
        static void ResetStats();
        static void ToggleActive();

    private:
        static void LoadConfig();
        static void SaveConfig();

        static bool SelectNextTarget();

        static GrindBotConfig Config;
        static GrindBotStats Stats;
        static GrindBotState State;
        static bool ConfigLoaded;
        static U32 CurrentTargetWorldId;
        static U32 LastStateTick;
    };
}
