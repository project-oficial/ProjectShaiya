#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct SkillInfo
    {
        U16 SkillId;
        U16 Level;
        F32 CooldownRemaining;
        F32 CooldownDuration;
        char Name[64];
        bool IsReady;
        bool IsLearned;
        bool IsPassive;
        U8 TargetType;
        U8 LearnedSlot;
    };
}
