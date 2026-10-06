#include "BuffManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Skills/SkillManager.h"

namespace ShaiyaOverlay
{
    FixedList<BuffInfo, 32> BuffManager::ActiveBuffs;
    FixedList<U16, 16> BuffManager::AutoBuffSkillIds;
    AutoBuffConfig BuffManager::Config;
    U32 BuffManager::LastBuffCastTick = 0;
    bool BuffManager::ConfigLoaded = false;

    static void GetAutoBuffIniPath(char* OutPath, U32 MaxLen)
    {
        GetModuleFileNameA(NULL, OutPath, MaxLen);
        char* pLastSlash = strrchr(OutPath, '\\');
        if (pLastSlash)
            *(pLastSlash + 1) = '\0';
        strcat_s(OutPath, MaxLen, "auto_buff.ini");
    }

    void BuffManager::LoadAutoBuffConfig()
    {
        AutoBuffSkillIds.Clear();
        ConfigLoaded = true;

        char IniPath[MAX_PATH] = { 0 };
        GetAutoBuffIniPath(IniPath, sizeof(IniPath));

        if (GetFileAttributesA(IniPath) == INVALID_FILE_ATTRIBUTES)
            return;

        Config.Enabled = (GetPrivateProfileIntA("AutoBuff", "Enabled", 1, IniPath) != 0);
        Config.RecastThresholdSeconds = GetPrivateProfileIntA("AutoBuff", "RecastThresholdSeconds", 3, IniPath);

        char SectionBuffer[4096] = { 0 };
        DWORD BytesRead = GetPrivateProfileSectionA("AutoBuffSkills", SectionBuffer, sizeof(SectionBuffer), IniPath);
        if (BytesRead == 0)
            return;

        const char* pKey = SectionBuffer;
        while (*pKey && AutoBuffSkillIds.GetCount() < 16)
        {
            // format: Skill_<Id>=1
            if (strncmp(pKey, "Skill_", 6) == 0)
            {
                U32 Id = static_cast<U32>(atoi(pKey + 6));
                if (Id > 0 && Id <= 0xFFFF)
                {
                    AutoBuffSkillIds.Add(static_cast<U16>(Id));
                }
            }
            pKey += strlen(pKey) + 1;
        }
    }

    void BuffManager::SaveAutoBuffConfig()
    {
        char IniPath[MAX_PATH] = { 0 };
        GetAutoBuffIniPath(IniPath, sizeof(IniPath));

        WritePrivateProfileStringA("AutoBuff", "Enabled", Config.Enabled ? "1" : "0", IniPath);

        char szVal[16];
        StringUtils::Format(szVal, sizeof(szVal), "%u", Config.RecastThresholdSeconds);
        WritePrivateProfileStringA("AutoBuff", "RecastThresholdSeconds", szVal, IniPath);

        WritePrivateProfileSectionA("AutoBuffSkills", "", IniPath);
        for (U32 i = 0; i < AutoBuffSkillIds.GetCount(); ++i)
        {
            char szKey[32];
            StringUtils::Format(szKey, sizeof(szKey), "Skill_%u", AutoBuffSkillIds[i]);
            WritePrivateProfileStringA("AutoBuffSkills", szKey, "1", IniPath);
        }
    }

    bool BuffManager::HasBuff(U16 SkillId, U32* OutRemainingSeconds)
    {
        for (U32 i = 0; i < ActiveBuffs.GetCount(); ++i)
        {
            if (ActiveBuffs[i].BuffId == SkillId)
            {
                if (OutRemainingSeconds)
                    *OutRemainingSeconds = ActiveBuffs[i].DurationSeconds;
                return true;
            }
        }
        if (OutRemainingSeconds)
            *OutRemainingSeconds = 0;
        return false;
    }

    bool BuffManager::IsAutoBuff(U16 SkillId)
    {
        if (!ConfigLoaded)
            LoadAutoBuffConfig();

        for (U32 i = 0; i < AutoBuffSkillIds.GetCount(); ++i)
        {
            if (AutoBuffSkillIds[i] == SkillId)
                return true;
        }
        return false;
    }

    void BuffManager::SetAutoBuff(U16 SkillId, bool Enable)
    {
        if (!ConfigLoaded)
            LoadAutoBuffConfig();

        if (Enable)
        {
            for (U32 i = 0; i < AutoBuffSkillIds.GetCount(); ++i)
            {
                if (AutoBuffSkillIds[i] == SkillId)
                    return;
            }
            if (AutoBuffSkillIds.GetCount() < 16)
            {
                AutoBuffSkillIds.Add(SkillId);
                SaveAutoBuffConfig();
            }
        }
        else
        {
            for (U32 i = 0; i < AutoBuffSkillIds.GetCount(); ++i)
            {
                if (AutoBuffSkillIds[i] == SkillId)
                {
                    AutoBuffSkillIds.RemoveAt(i);
                    SaveAutoBuffConfig();
                    return;
                }
            }
        }
    }

    bool BuffManager::CastBuff(U8 LearnedSlot)
    {
        if (!Offsets.SendCharBuffPacketAddr || LearnedSlot == 0xFF)
            return false;

        using tSendCharBuffPacket = __int64(__fastcall*)(U8 SlotIndex, U32 TargetId);
        auto Fn = reinterpret_cast<tSendCharBuffPacket>(Offsets.SendCharBuffPacketAddr);

        __try
        {
            Fn(LearnedSlot, 0);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void BuffManager::ProcessAutoBuffs()
    {
        if (!ConfigLoaded)
            LoadAutoBuffConfig();

        if (!Config.Enabled || AutoBuffSkillIds.GetCount() == 0)
            return;

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        if (!Player.Valid || Player.CurrentHp == 0)
            return;

        U32 Now = GetTickCount();
        if (Now - LastBuffCastTick < 1400) // 1.4s between buff casts
            return;

        const FixedList<SkillInfo, 64>& Skills = SkillManager::GetSkills();
        U32 SkillCount = Skills.GetCount();

        for (U32 i = 0; i < AutoBuffSkillIds.GetCount(); ++i)
        {
            U16 TargetSkillId = AutoBuffSkillIds[i];

            // 1. Check if buff is currently active with remaining duration > threshold
            U32 RemSec = 0;
            if (HasBuff(TargetSkillId, &RemSec) && RemSec > Config.RecastThresholdSeconds)
                continue;

            // 2. Find skill in learned skill list
            for (U32 S = 0; S < SkillCount; ++S)
            {
                const SkillInfo& Skill = Skills[S];
                if (Skill.SkillId == TargetSkillId)
                {
                    if (Skill.IsLearned && Skill.LearnedSlot != 0xFF && Skill.IsReady)
                    {
                        if (CastBuff(Skill.LearnedSlot))
                        {
                            LastBuffCastTick = Now;
                            Logger::Info("AutoBuff: Recasting %s (SkillId: %u, Slot: %u)",
                                Skill.Name, Skill.SkillId, Skill.LearnedSlot);
                            return; // One buff per tick
                        }
                    }
                    break;
                }
            }
        }
    }

    void BuffManager::Update()
    {
        ActiveBuffs.Clear();

        if (!Offsets.MainUIPtr)
            return;

        U64 pGame = 0;
        if (!Memory::ReadSafe(Offsets.MainUIPtr, &pGame) || !pGame)
            return;

        U64 pMainUI = 0;
        if (!Memory::ReadSafe(pGame, &pMainUI) || !pMainUI)
            return;

        U64 FirstPtr = 0;
        U64 LastPtr = 0;
        if (!Memory::ReadSafe(pMainUI + 72, &FirstPtr) || !FirstPtr ||
            !Memory::ReadSafe(pMainUI + 80, &LastPtr) || LastPtr < FirstPtr)
        {
            return;
        }

        U64 Count = (LastPtr - FirstPtr) / 36;
        if (Count > 32)
            Count = 32;

        for (U32 I = 0; I < Count; ++I)
        {
            U64 EntryAddr = FirstPtr + I * 36;
            U16 SkillId = 0;
            U8 Level = 0;
            U32 TotalDurMs = 0;
            U32 RemMs = 0;

            Memory::ReadSafe(EntryAddr + 4, &SkillId);
            Memory::ReadSafe(EntryAddr + 6, &Level);
            Memory::ReadSafe(EntryAddr + 8, &TotalDurMs);
            Memory::ReadSafe(EntryAddr + 16, &RemMs);

            if (SkillId == 0 || RemMs == 0)
                continue;

            BuffInfo Buff = { 0 };
            Buff.BuffId = SkillId;
            Buff.Level = Level;
            Buff.DurationSeconds = (RemMs + 999) / 1000;
            Buff.TotalDurationSeconds = (TotalDurMs + 999) / 1000;
            Buff.IsDebuff = false;

            // Resolve name and debuff status via native GetSkillRecord
            if (Offsets.GetSkillRecordAddr && Offsets.ItemDb)
            {
                using GetSkillRecordFn = U64(__fastcall*)(U64 ItemDb, U16 SkillId, U8 Level);
                auto Fn = reinterpret_cast<GetSkillRecordFn>(Offsets.GetSkillRecordAddr);

                __try
                {
                    U64 RecPtr = Fn(Offsets.ItemDb, SkillId, Level);
                    if (RecPtr)
                    {
                        U64 NamePtr = 0;
                        if (Memory::ReadSafe(RecPtr + 8, &NamePtr) && NamePtr)
                        {
                            char Temp[64] = { 0 };
                            if (Memory::ReadBytesSafe(NamePtr, Temp, sizeof(Temp) - 1))
                            {
                                StringUtils::AnsiToUtf8(Temp, Buff.Name, sizeof(Buff.Name));
                            }
                        }

                        U8 TargetType = 0;
                        if (Memory::ReadSafe(RecPtr + 39, &TargetType))
                        {
                            Buff.IsDebuff = (TargetType == 3);
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }

            if (Buff.Name[0] == '\0')
            {
                StringUtils::Format(Buff.Name, sizeof(Buff.Name), "Buff #%u", SkillId);
            }

            ActiveBuffs.Add(Buff);
        }

        ProcessAutoBuffs();
    }
}
