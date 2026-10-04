#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    enum class ThreatLevel : U8
    {
        None = 0,
        Low,
        Medium,
        High,
        Fatal
    };

    struct MonsterEntity
    {
        U32 WorldId;
        U32 MobId;
        U16 Level;
        U32 CurrentHp;
        U32 MaxHp;
        Vector3 Position;
        F32 Distance;
        char Name[64];
        ThreatLevel Threat;
        bool Alive;
        bool IsQuestTarget;

        F32 GetHpPercentage() const
        {
            if (MaxHp == 0) return 0.0f;
            return (F32)CurrentHp / (F32)MaxHp;
        }
    };
}
