#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct GroundItem
    {
        U32 WorldId;
        U32 OwnerId;
        U32 DespawnTime;
        U8 Type;
        U8 TypeId;
        U32 Count;
        U8 Quality;
        Vector3 Position;
        F32 Distance;
        char Name[64];
        char Category[32];
    };
}
