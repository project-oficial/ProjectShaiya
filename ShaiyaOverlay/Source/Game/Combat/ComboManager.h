#pragma once

#include "Core/Types.h"
#include "Game/Skills/Skill.h"

namespace ShaiyaOverlay
{
    enum class TargetFilterMode : U8
    {
        AllMonsters = 0,
        QuestMonstersOnly = 1
    };

    struct ComboEntry
    {
        U16 SkillId;
        char Name[64];
    };

    struct ComboConfig
    {
        bool Enabled = true;
        bool Active = false;
        bool HoldKeyMode = false;
        U32 Hotkey = 'C';
        U32 CastDelayMs = 1100;

        // Auto-Attack / Next Target switching
        bool AutoTargetNext = false;
        TargetFilterMode TargetFilter = TargetFilterMode::AllMonsters;
        F32 MaxTargetRange = 25.0f;
    };

    class ComboManager
    {
    public:
        static void Update();

        static const FixedList<ComboEntry, 16>& GetComboSequence() { return Sequence; }
        static U32 GetSequenceCount() { return Sequence.GetCount(); }

        static bool AddSkillToSequence(U16 SkillId, const char* SkillName = nullptr);
        static bool RemoveSkillFromSequence(U32 Index);
        static void MoveSkillUp(U32 Index);
        static void MoveSkillDown(U32 Index);
        static void ClearSequence();

        static void ToggleActive();
        static void SetActive(bool Active);
        static bool IsActive() { return Config.Active; }

        static ComboConfig& GetConfig() { return Config; }
        static void SaveConfig();

        static void HandleHotkeyState(bool KeyDown);
        static U32 FindNextMonsterTarget();

    private:
        static void LoadConfig();
        static bool ExecuteNextSkill();

        static FixedList<ComboEntry, 16> Sequence;
        static ComboConfig Config;
        static U32 CurrentIndex;
        static U32 LastCastTick;
        static bool ConfigLoaded;
    };
}
