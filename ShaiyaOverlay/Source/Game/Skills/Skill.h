#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct SkillInfo
    {
        U16 SkillId;
        U16 Level;
        F32 CooldownRemaining;
        char Name[64];
        bool IsReady;
    };
}
