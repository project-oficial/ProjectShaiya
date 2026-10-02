#include "QuickSlotManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"
#include "Game/Skills/SkillManager.h"

namespace ShaiyaOverlay
{
    FixedList<QuickSlotEntry, 30> QuickSlotManager::Slots;

    const char* QuickSlotManager::GetActionName(U16 ActionId)
    {
        switch (ActionId)
        {
        case 1:  return "Sit / Stand";
        case 2:  return "Walk / Run";
        case 3:  return "Pick Up";
        case 4:  return "Normal Attack";
        case 5:  return "Auto Attack";
        case 6:  return "Trade";
        case 7:  return "Party Invite";
        case 8:  return "Whisper";
        default: return "Command Action";
        }
    }

    void QuickSlotManager::Update()
    {
        Slots.Clear();

        if (!Offsets.QuickSlotBase)
            return;

        U64 SkillVecFirst = 0;
        U64 SkillVecLast = 0;
        if (Offsets.SkillVector)
        {
            Memory::ReadSafe(Offsets.SkillVector + 8, &SkillVecFirst);
            Memory::ReadSafe(Offsets.SkillVector + 16, &SkillVecLast);
        }

        constexpr U32 TotalSlotsToRead = 30; // 3 bars of 10 slots each

        for (U32 I = 0; I < TotalSlotsToRead; ++I)
        {
            U64 EntryAddr = Offsets.QuickSlotBase + I * 20;

            QuickSlotEntry Entry = { 0 };
            Entry.BarIndex = I / 10;
            Entry.SlotIndex = I % 10;

            U32 ActiveFlag = 0;
            Memory::ReadSafe(EntryAddr, &Entry.RawType);
            Memory::ReadSafe(EntryAddr + 1, &Entry.SubType);
            Memory::ReadSafe(EntryAddr + 2, &Entry.TargetId);
            Memory::ReadSafe(EntryAddr + 4, &ActiveFlag);

            Entry.Active = (ActiveFlag != 0);

            if (!Entry.Active)
            {
                Entry.Type = QuickSlotType::Empty;
                StringUtils::Copy(Entry.Name, "[Empty]", sizeof(Entry.Name));
                StringUtils::Copy(Entry.Details, "-", sizeof(Entry.Details));
            }
            else if (Entry.RawType == 100)
            {
                Entry.Type = QuickSlotType::Skill;
                U16 SkillId = 0;
                U8 SkillLvl = 0;
                bool SkillFound = false;

                if (SkillVecFirst && SkillVecLast && SkillVecLast > SkillVecFirst)
                {
                    U64 MaxSkills = (SkillVecLast - SkillVecFirst) / sizeof(U64);
                    if (Entry.TargetId < MaxSkills)
                    {
                        U64 SkillDataPtr = 0;
                        if (Memory::ReadSafe(SkillVecFirst + Entry.TargetId * sizeof(U64), &SkillDataPtr) && SkillDataPtr)
                        {
                            Memory::ReadSafe(SkillDataPtr + 2, &SkillId);
                            Memory::ReadSafe(SkillDataPtr + 4, &SkillLvl);
                            SkillFound = true;
                        }
                    }
                }

                if (SkillFound)
                {
                    if (!SkillManager::ResolveSkillName(SkillId, SkillLvl, Entry.Name, sizeof(Entry.Name)))
                    {
                        StringUtils::Format(Entry.Name, sizeof(Entry.Name), "Skill #%u", SkillId);
                    }
                    StringUtils::Format(Entry.Details, sizeof(Entry.Details), "Lvl %u", SkillLvl);
                }
                else
                {
                    StringUtils::Format(Entry.Name, sizeof(Entry.Name), "Skill Slot #%u", Entry.TargetId);
                    StringUtils::Copy(Entry.Details, "Skill", sizeof(Entry.Details));
                }
            }
            else if (Entry.RawType == 101)
            {
                Entry.Type = QuickSlotType::Action;
                StringUtils::Copy(Entry.Name, GetActionName(Entry.TargetId), sizeof(Entry.Name));
                StringUtils::Format(Entry.Details, sizeof(Entry.Details), "Action ID: %u", Entry.TargetId);
            }
            else
            {
                Entry.Type = QuickSlotType::Item;
                StringUtils::Format(Entry.Name, sizeof(Entry.Name), "Bag %u / Slot %u", Entry.RawType + 1, Entry.TargetId + 1);
                StringUtils::Format(Entry.Details, sizeof(Entry.Details), "Inventory Item");
            }

            Slots.Add(Entry);
        }
    }
}
