#include "BuffManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"

namespace ShaiyaOverlay
{
    FixedList<BuffInfo, 32> BuffManager::ActiveBuffs;

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
    }
}
