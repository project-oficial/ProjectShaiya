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

    private:
        static FixedList<SkillInfo, 64> Skills;
    };
}
