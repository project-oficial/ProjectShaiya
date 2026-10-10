#include "ComboManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Skills/SkillManager.h"
#include "Game/Bot/GrindBot.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

namespace ShaiyaOverlay
{
    FixedList<ComboEntry, 16> ComboManager::Sequence;
    ComboConfig ComboManager::Config;
    U32 ComboManager::CurrentIndex = 0;
    U32 ComboManager::LastCastTick = 0;
    bool ComboManager::ConfigLoaded = false;

    static void GetAutoComboIniPath(char* OutPath, U32 MaxLen)
    {
        GetModuleFileNameA(NULL, OutPath, MaxLen);
        char* pLastSlash = strrchr(OutPath, '\\');
        if (pLastSlash)
            *(pLastSlash + 1) = '\0';
        strcat_s(OutPath, MaxLen, "auto_combo.ini");
    }

    void ComboManager::LoadConfig()
    {
        Sequence.Clear();
        ConfigLoaded = true;

        char IniPath[MAX_PATH] = { 0 };
        GetAutoComboIniPath(IniPath, sizeof(IniPath));

        if (GetFileAttributesA(IniPath) == INVALID_FILE_ATTRIBUTES)
            return;

        Config.Enabled = (GetPrivateProfileIntA("AutoCombo", "Enabled", 1, IniPath) != 0);
        Config.Hotkey = static_cast<U32>(GetPrivateProfileIntA("AutoCombo", "Hotkey", 'C', IniPath));
        Config.HoldKeyMode = (GetPrivateProfileIntA("AutoCombo", "HoldKeyMode", 0, IniPath) != 0);
        Config.CastDelayMs = static_cast<U32>(GetPrivateProfileIntA("AutoCombo", "CastDelayMs", 1100, IniPath));

        Config.AutoTargetNext = (GetPrivateProfileIntA("AutoCombo", "AutoTargetNext", 0, IniPath) != 0);
        Config.TargetFilter = static_cast<TargetFilterMode>(GetPrivateProfileIntA("AutoCombo", "TargetFilter", 0, IniPath));

        char BufRange[32] = { 0 };
        GetPrivateProfileStringA("AutoCombo", "MaxTargetRange", "25.0", BufRange, sizeof(BufRange), IniPath);
        float r = static_cast<float>(atof(BufRange));
        if (r >= 5.0f && r <= 50.0f)
            Config.MaxTargetRange = r;

        char SectionBuffer[4096] = { 0 };
        DWORD BytesRead = GetPrivateProfileSectionA("Sequence", SectionBuffer, sizeof(SectionBuffer), IniPath);
        if (BytesRead == 0)
            return;

        const char* pKey = SectionBuffer;
        while (*pKey && Sequence.GetCount() < 16)
        {
            // format: Step_<idx>=<SkillId>
            const char* pEq = strchr(pKey, '=');
            if (pEq)
            {
                U32 SkillId = static_cast<U32>(atoi(pEq + 1));
                if (SkillId > 0 && SkillId <= 0xFFFF)
                {
                    AddSkillToSequence(static_cast<U16>(SkillId));
                }
            }
            pKey += strlen(pKey) + 1;
        }
    }

    void ComboManager::SaveConfig()
    {
        char IniPath[MAX_PATH] = { 0 };
        GetAutoComboIniPath(IniPath, sizeof(IniPath));

        WritePrivateProfileStringA("AutoCombo", "Enabled", Config.Enabled ? "1" : "0", IniPath);

        char szVal[32];
        StringUtils::Format(szVal, sizeof(szVal), "%u", Config.Hotkey);
        WritePrivateProfileStringA("AutoCombo", "Hotkey", szVal, IniPath);

        WritePrivateProfileStringA("AutoCombo", "HoldKeyMode", Config.HoldKeyMode ? "1" : "0", IniPath);

        StringUtils::Format(szVal, sizeof(szVal), "%u", Config.CastDelayMs);
        WritePrivateProfileStringA("AutoCombo", "CastDelayMs", szVal, IniPath);

        WritePrivateProfileStringA("AutoCombo", "AutoTargetNext", Config.AutoTargetNext ? "1" : "0", IniPath);

        char szFilter[16];
        StringUtils::Format(szFilter, sizeof(szFilter), "%u", static_cast<U8>(Config.TargetFilter));
        WritePrivateProfileStringA("AutoCombo", "TargetFilter", szFilter, IniPath);

        char szRange[32];
        StringUtils::Format(szRange, sizeof(szRange), "%.1f", Config.MaxTargetRange);
        WritePrivateProfileStringA("AutoCombo", "MaxTargetRange", szRange, IniPath);

        WritePrivateProfileSectionA("Sequence", "", IniPath);
        for (U32 i = 0; i < Sequence.GetCount(); ++i)
        {
            char szKey[32];
            char szNum[16];
            StringUtils::Format(szKey, sizeof(szKey), "Step_%u", i);
            StringUtils::Format(szNum, sizeof(szNum), "%u", Sequence[i].SkillId);
            WritePrivateProfileStringA("Sequence", szKey, szNum, IniPath);
        }
    }

    bool ComboManager::AddSkillToSequence(U16 SkillId, const char* SkillName)
    {
        if (SkillId == 0 || Sequence.GetCount() >= 16)
            return false;

        ComboEntry Entry = { 0 };
        Entry.SkillId = SkillId;

        if (SkillName && SkillName[0] != '\0')
        {
            StringUtils::Copy(Entry.Name, SkillName, sizeof(Entry.Name));
        }
        else
        {
            if (!SkillManager::ResolveSkillName(SkillId, 1, Entry.Name, sizeof(Entry.Name)))
            {
                StringUtils::Format(Entry.Name, sizeof(Entry.Name), "Skill #%u", SkillId);
            }
        }
        StringUtils::NormalizeAccents(Entry.Name, sizeof(Entry.Name), false);

        Sequence.Add(Entry);
        SaveConfig();
        return true;
    }

    bool ComboManager::RemoveSkillFromSequence(U32 Index)
    {
        if (Index >= Sequence.GetCount())
            return false;

        Sequence.RemoveAt(Index);
        if (CurrentIndex >= Sequence.GetCount())
            CurrentIndex = 0;

        SaveConfig();
        return true;
    }

    void ComboManager::MoveSkillUp(U32 Index)
    {
        if (Index == 0 || Index >= Sequence.GetCount())
            return;

        ComboEntry Temp = Sequence[Index];
        Sequence[Index] = Sequence[Index - 1];
        Sequence[Index - 1] = Temp;
        SaveConfig();
    }

    void ComboManager::MoveSkillDown(U32 Index)
    {
        if (Index + 1 >= Sequence.GetCount())
            return;

        ComboEntry Temp = Sequence[Index];
        Sequence[Index] = Sequence[Index + 1];
        Sequence[Index + 1] = Temp;
        SaveConfig();
    }

    void ComboManager::ClearSequence()
    {
        Sequence.Clear();
        CurrentIndex = 0;
        Config.Active = false;
        SaveConfig();
    }

    void ComboManager::ToggleActive()
    {
        Config.Active = !Config.Active;
        if (Config.Active)
            CurrentIndex = 0;
    }

    void ComboManager::SetActive(bool Active)
    {
        Config.Active = Active;
        if (Active)
            CurrentIndex = 0;
    }

    void ComboManager::HandleHotkeyState(bool KeyDown)
    {
        if (!Config.Enabled)
            return;

        if (Config.HoldKeyMode)
        {
            Config.Active = KeyDown;
            if (KeyDown)
                CurrentIndex = 0;
        }
        else
        {
            if (KeyDown)
            {
                ToggleActive();
                Logger::Info("AutoCombo: %s via hotkey [0x%X]", Config.Active ? "STARTED" : "STOPPED", Config.Hotkey);
            }
        }
    }

    U32 ComboManager::FindNextMonsterTarget()
    {
        const FixedList<MonsterEntity, 128>& Mobs = EntityManager::GetNearbyMonsters();
        F32 BestDist = Config.MaxTargetRange;
        U32 BestWorldId = 0;

        for (U32 i = 0; i < Mobs.GetCount(); ++i)
        {
            const MonsterEntity& Mob = Mobs[i];
            if (!Mob.Alive || Mob.CurrentHp == 0)
                continue;

            // Filter: Quest only if configured
            if (Config.TargetFilter == TargetFilterMode::QuestMonstersOnly && !Mob.IsQuestTarget)
                continue;

            if (Mob.Distance <= BestDist)
            {
                BestDist = Mob.Distance;
                BestWorldId = Mob.WorldId;
            }
        }
        return BestWorldId;
    }

    bool ComboManager::ExecuteNextSkill()
    {
        if (Sequence.GetCount() == 0)
            return false;

        U32 Now = GetTickCount();
        if (Now - LastCastTick < Config.CastDelayMs)
            return false;

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        if (!Player.Valid || Player.CurrentHp == 0)
        {
            Config.Active = false;
            return false;
        }

        const FixedList<SkillInfo, 64>& Skills = SkillManager::GetSkills();
        U32 SkillCount = Skills.GetCount();
        U32 SequenceCount = Sequence.GetCount();

        // Check skills starting from CurrentIndex; skip any that are on cooldown or not ready
        for (U32 attempt = 0; attempt < SequenceCount; ++attempt)
        {
            U32 checkIdx = (CurrentIndex + attempt) % SequenceCount;
            U16 targetSkillId = Sequence[checkIdx].SkillId;

            for (U32 s = 0; s < SkillCount; ++s)
            {
                const SkillInfo& Skill = Skills[s];
                if (Skill.SkillId == targetSkillId)
                {
                    if (Skill.IsLearned && !Skill.IsPassive && Skill.LearnedSlot != 0xFF)
                    {
                        const bool IsBuff = (Skill.TargetType == 0 || Skill.TargetType == 2 || Skill.TargetType == 8);

                        // If offensive skill, verify or acquire an alive monster target
                        if (!IsBuff)
                        {
                            if (!SkillManager::HasAliveTarget())
                            {
                                if (Config.AutoTargetNext)
                                {
                                    U32 NextMobId = FindNextMonsterTarget();
                                    if (NextMobId != 0)
                                    {
                                        SkillManager::SetTarget(NextMobId);
                                        Logger::Info("AutoCombo: Auto-targeted next mob (WorldId: %u)", NextMobId);
                                    }
                                    else
                                    {
                                        return false; // No valid alive mob in range matching filter
                                    }
                                }
                                else
                                {
                                    // Standalone mode: wait for manual target
                                    return false;
                                }
                            }
                        }

                        // Check if ready to cast (not on cooldown)
                        if (Skill.IsReady && Skill.CooldownRemaining <= 0.0f)
                        {
                            if (SkillManager::CastSkill(Skill.LearnedSlot, Skill.TargetType))
                            {
                                LastCastTick = Now;
                                CurrentIndex = (checkIdx + 1) % SequenceCount;
                                Logger::Info("AutoCombo: Cast %s (Slot: %u, Step: %u/%u)",
                                    Skill.Name, Skill.LearnedSlot, checkIdx + 1, SequenceCount);
                                return true;
                            }
                        }
                    }
                    break; // Skill found in list, but on cooldown, so continue loop to next skill in sequence
                }
            }
        }

        return false;
    }

    void ComboManager::Update()
    {
        if (!ConfigLoaded)
            LoadConfig();

        // Dynamically resolve skill names if they were loaded before game world was ready
        for (U32 i = 0; i < Sequence.GetCount(); ++i)
        {
            if (Sequence[i].Name[0] == '\0' || strncmp(Sequence[i].Name, "Skill #", 7) == 0)
            {
                const auto& learned = SkillManager::GetSkills();
                bool foundLearned = false;
                for (U32 s = 0; s < learned.GetCount(); ++s)
                {
                    if (learned[s].SkillId == Sequence[i].SkillId && learned[s].Name[0] != '\0')
                    {
                        StringUtils::Copy(Sequence[i].Name, learned[s].Name, sizeof(Sequence[i].Name));
                        foundLearned = true;
                        break;
                    }
                }

                if (!foundLearned)
                {
                    char realName[64] = { 0 };
                    if (SkillManager::ResolveSkillName(Sequence[i].SkillId, 1, realName, sizeof(realName)))
                    {
                        StringUtils::Copy(Sequence[i].Name, realName, sizeof(Sequence[i].Name));
                    }
                }
                StringUtils::NormalizeAccents(Sequence[i].Name, sizeof(Sequence[i].Name), false);
            }
        }

        // Combo execution is strictly bound to Grind Bot combat
        if (!GrindBot::GetConfig().Enabled || !Config.Active)
        {
            Config.Active = false;
            return;
        }

        ExecuteNextSkill();
    }
}
