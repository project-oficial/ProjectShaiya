#pragma once

#include "Core/Types.h"
#include "Skill.h"

namespace ShaiyaOverlay
{
    class SkillManager
    {
    public:
        static void Update();
        static const FixedList<SkillInfo, 64>& GetSkills() { return Skills; }
        static U32 GetSkillCount() { return Skills.GetCount(); }

        static bool ResolveSkillName(U16 SkillId, U16 Level, char* OutName, U32 MaxLen);
        static bool ResolveSkillDetails(U16 SkillId, U16 Level, char* OutName, U32 MaxLen, bool* OutPassive = nullptr, U8* OutTargetType = nullptr, U16* OutBaseCooldownSec = nullptr);
        static bool CastSkill(U8 LearnedSlot, U8 TargetType = 3, U32 ExplicitTargetId = 0);
        static U32 GetSelectedTargetWorldId();
        static U32 GetGameTimeMs();

    private:
        static FixedList<SkillInfo, 64> Skills;
    };
}
