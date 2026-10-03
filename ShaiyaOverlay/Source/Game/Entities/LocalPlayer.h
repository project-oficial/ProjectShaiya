#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct PlayerData
    {
        U32 Id;
        U32 Level;
        U32 CurrentHp;
        U32 MaxHp;
        U32 CurrentMp;
        U32 MaxMp;
        U32 CurrentSp;
        U32 MaxSp;
        Vector3 Position;
        bool Valid;

        F32 GetHpPercentage() const
        {
            if (MaxHp == 0) return 0.0f;
            return (F32)CurrentHp / (F32)MaxHp;
        }

        F32 GetMpPercentage() const
        {
            if (MaxMp == 0) return 0.0f;
            return (F32)CurrentMp / (F32)MaxMp;
        }

        F32 GetSpPercentage() const
        {
            if (MaxSp == 0) return 0.0f;
            return (F32)CurrentSp / (F32)MaxSp;
        }
    };
}
