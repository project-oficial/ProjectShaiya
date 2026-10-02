#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct BuffInfo
    {
        U16 BuffId;
        U8 Level;
        U32 DurationSeconds;
        char Name[64];
        bool IsDebuff;
    };
}
