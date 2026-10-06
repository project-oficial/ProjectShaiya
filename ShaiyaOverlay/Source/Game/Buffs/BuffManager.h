#pragma once

#include "Core/Types.h"
#include "Buff.h"

namespace ShaiyaOverlay
{
    struct AutoBuffConfig
    {
        bool Enabled = true;
        U32 RecastThresholdSeconds = 3;
    };

    class BuffManager
    {
    public:
        static void Update();
        static const FixedList<BuffInfo, 32>& GetBuffs() { return ActiveBuffs; }
        static U32 GetBuffCount() { return ActiveBuffs.GetCount(); }

        static bool HasBuff(U16 SkillId, U32* OutRemainingSeconds = nullptr);
        static bool IsAutoBuff(U16 SkillId);
        static void SetAutoBuff(U16 SkillId, bool Enable);
        static AutoBuffConfig& GetConfig() { return Config; }

        static bool CastBuff(U8 LearnedSlot);

    private:
        static void ProcessAutoBuffs();
        static void LoadAutoBuffConfig();
        static void SaveAutoBuffConfig();

        static FixedList<BuffInfo, 32> ActiveBuffs;
        static FixedList<U16, 16> AutoBuffSkillIds;
        static AutoBuffConfig Config;
        static U32 LastBuffCastTick;
        static bool ConfigLoaded;
    };
}
