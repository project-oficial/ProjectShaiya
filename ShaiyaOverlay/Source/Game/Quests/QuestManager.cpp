#include "QuestManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/Navigation/NavigationManager.h"
#include <stdlib.h>
#include <stdio.h>

namespace ShaiyaOverlay
{
    FixedList<ActiveQuest, 16> QuestManager::Quests;
    FixedList<QuestMarker, 32> QuestManager::Markers;
    FixedList<SavedQuestMob, 128> QuestManager::SavedMobCache;
    bool QuestManager::MobCacheLoaded = false;

    static void GetQuestMobsIniPath(char* OutPath, U32 MaxLen)
    {
        GetModuleFileNameA(NULL, OutPath, MaxLen);
        char* pLastSlash = strrchr(OutPath, '\\');
        if (pLastSlash)
            *(pLastSlash + 1) = '\0';
        strcat_s(OutPath, MaxLen, "quest_mobs.ini");
    }

    void QuestManager::LoadMobCache()
    {
        SavedMobCache.Clear();
        MobCacheLoaded = true;

        char IniPath[MAX_PATH] = { 0 };
        GetQuestMobsIniPath(IniPath, sizeof(IniPath));

        if (GetFileAttributesA(IniPath) == INVALID_FILE_ATTRIBUTES)
            return;

        char SectionNames[8192] = { 0 };
        DWORD BytesRead = GetPrivateProfileSectionNamesA(SectionNames, sizeof(SectionNames), IniPath);
        if (BytesRead == 0)
            return;

        const char* pSection = SectionNames;
        while (*pSection && SavedMobCache.GetCount() < 128)
        {
            U32 Qid = GetPrivateProfileIntA(pSection, "QuestId", 0, IniPath);
            U32 Mid = GetPrivateProfileIntA(pSection, "MobId", 0, IniPath);

            char BufX[32] = { 0 };
            char BufY[32] = { 0 };
            char BufZ[32] = { 0 };
            char BufName[64] = { 0 };

            GetPrivateProfileStringA(pSection, "X", "0", BufX, sizeof(BufX), IniPath);
            GetPrivateProfileStringA(pSection, "Y", "0", BufY, sizeof(BufY), IniPath);
            GetPrivateProfileStringA(pSection, "Z", "0", BufZ, sizeof(BufZ), IniPath);
            GetPrivateProfileStringA(pSection, "Name", "", BufName, sizeof(BufName), IniPath);

            F32 X = static_cast<F32>(atof(BufX));
            F32 Y = static_cast<F32>(atof(BufY));
            F32 Z = static_cast<F32>(atof(BufZ));

            if (Mid > 0 && (X != 0.0f || Z != 0.0f))
            {
                SavedQuestMob Entry;
                Entry.QuestId = static_cast<U16>(Qid);
                Entry.MobId = static_cast<U16>(Mid);
                Entry.Position = { X, Y, Z };
                StringUtils::Copy(Entry.MobName, BufName, sizeof(Entry.MobName));
                SavedMobCache.Add(Entry);
            }

            pSection += strlen(pSection) + 1;
        }
    }

    void QuestManager::SaveMobPosition(U16 QuestId, U16 MobId, const Vector3& Pos, const char* MobName)
    {
        if (MobId == 0)
            return;

        if (!MobCacheLoaded)
            LoadMobCache();

        // If already recorded for this quest and mob, NEVER update or overwrite
        for (U32 i = 0; i < SavedMobCache.GetCount(); ++i)
        {
            const auto& Entry = SavedMobCache[i];
            if (Entry.QuestId == QuestId && Entry.MobId == MobId)
                return;
        }

        if (SavedMobCache.GetCount() < 128)
        {
            SavedQuestMob Entry;
            Entry.QuestId = QuestId;
            Entry.MobId = MobId;
            Entry.Position = Pos;
            Entry.MobName[0] = '\0';
            if (MobName && MobName[0] != '\0')
                StringUtils::Copy(Entry.MobName, MobName, sizeof(Entry.MobName));
            SavedMobCache.Add(Entry);
        }

        char IniPath[MAX_PATH] = { 0 };
        GetQuestMobsIniPath(IniPath, sizeof(IniPath));

        char Section[64];
        StringUtils::Format(Section, sizeof(Section), "Quest_%u_Mob_%u", QuestId, MobId);

        char szVal[32];
        StringUtils::Format(szVal, sizeof(szVal), "%u", QuestId);
        WritePrivateProfileStringA(Section, "QuestId", szVal, IniPath);

        StringUtils::Format(szVal, sizeof(szVal), "%u", MobId);
        WritePrivateProfileStringA(Section, "MobId", szVal, IniPath);

        StringUtils::Format(szVal, sizeof(szVal), "%.2f", Pos.X);
        WritePrivateProfileStringA(Section, "X", szVal, IniPath);

        StringUtils::Format(szVal, sizeof(szVal), "%.2f", Pos.Y);
        WritePrivateProfileStringA(Section, "Y", szVal, IniPath);

        StringUtils::Format(szVal, sizeof(szVal), "%.2f", Pos.Z);
        WritePrivateProfileStringA(Section, "Z", szVal, IniPath);

        if (MobName && MobName[0] != '\0')
        {
            WritePrivateProfileStringA(Section, "Name", MobName, IniPath);
        }
    }

    bool QuestManager::GetSavedMobPosition(U16 QuestId, U16 MobId, Vector3& OutPos, char* OutName, U32 MaxLen)
    {
        if (QuestId == 0 || MobId == 0)
            return false;

        if (!MobCacheLoaded)
            LoadMobCache();

        // Exact match by QuestId + MobId
        for (U32 i = 0; i < SavedMobCache.GetCount(); ++i)
        {
            const auto& Entry = SavedMobCache[i];
            if (Entry.QuestId == QuestId && Entry.MobId == MobId)
            {
                OutPos = Entry.Position;
                if (OutName && MaxLen > 0)
                    StringUtils::Copy(OutName, Entry.MobName, MaxLen);
                return true;
            }
        }

        return false;
    }

    void QuestManager::AutoRecordNearbyQuestMobs()
    {
        if (Quests.GetCount() == 0)
            return;

        const FixedList<MonsterEntity, 128>& Mobs = EntityManager::GetNearbyMonsters();
        if (Mobs.GetCount() == 0)
            return;

        for (U32 Q = 0; Q < Quests.GetCount(); ++Q)
        {
            const ActiveQuest& Quest = Quests[Q];

            for (U32 O = 0; O < Quest.ObjectiveCount; ++O)
            {
                U16 TargetMid = Quest.Objectives[O].TargetMobId;
                if (TargetMid == 0) continue;

                for (U32 M = 0; M < Mobs.GetCount(); ++M)
                {
                    const MonsterEntity& Mob = Mobs[M];
                    if (Mob.Alive && Mob.MobId == TargetMid)
                    {
                        SaveMobPosition(Quest.QuestId, TargetMid, Mob.Position, Mob.Name);
                        break;
                    }
                }
            }

            for (U32 I = 0; I < Quest.ItemObjectiveCount; ++I)
            {
                U16 DropMid = Quest.ItemObjectives[I].DroppedByMobId;
                if (DropMid == 0) continue;

                for (U32 M = 0; M < Mobs.GetCount(); ++M)
                {
                    const MonsterEntity& Mob = Mobs[M];
                    if (Mob.Alive && Mob.MobId == DropMid)
                    {
                        SaveMobPosition(Quest.QuestId, DropMid, Mob.Position, Mob.Name);
                        break;
                    }
                }
            }
        }
    }

    U32 QuestManager::CountInventoryItem(U8 ItemType, U8 ItemTypeId)
    {
        if (!Offsets.PlayerInventory || ItemType == 0)
            return 0;

        U64 SlotStart = Offsets.PlayerInventory + 17521;
        U32 Total = 0;

        for (U32 I = 0; I < 240; ++I)
        {
            U8 Data[4] = { 0 };
            if (Memory::ReadBytesSafe(SlotStart + I * 132, Data, 3))
            {
                if (Data[0] == ItemType && Data[1] == ItemTypeId)
                {
                    Total += Data[2];
                }
            }
        }
        return Total;
    }

    bool QuestManager::ExtractTagFromDescription(const char* Desc, char* OutTag, U32 MaxLen)
    {
        char Tags[4][64];
        U32 Count = ExtractAllTagsFromDescription(Desc, Tags);
        if (Count > 0 && OutTag && MaxLen > 0)
        {
            StringUtils::Copy(OutTag, Tags[0], MaxLen);
            return true;
        }
        return false;
    }

    U32 QuestManager::ExtractAllTagsFromDescription(const char* Desc, char OutTags[4][64])
    {
        if (!Desc || !OutTags)
            return 0;

        U32 TagCount = 0;
        const char* Ptr = Desc;

        while (*Ptr && TagCount < 4)
        {
            if (Ptr[0] == '{' && Ptr[1] == 'c' && Ptr[2] >= '0' && Ptr[2] <= '9' && Ptr[3] == '}')
            {
                const char* TagStart = Ptr + 4;
                const char* TagEnd = nullptr;
                const char* Walk = TagStart;

                while (*Walk)
                {
                    if (Walk[0] == '{' && Walk[1] == '/' && Walk[2] == 'c' && Walk[3] == '}')
                    {
                        TagEnd = Walk;
                        break;
                    }
                    ++Walk;
                }

                if (TagEnd && TagEnd > TagStart)
                {
                    U32 Len = static_cast<U32>(TagEnd - TagStart);
                    if (Len >= 64)
                        Len = 63;

                    for (U32 i = 0; i < Len; ++i)
                        OutTags[TagCount][i] = TagStart[i];
                    OutTags[TagCount][Len] = '\0';
                    TagCount++;

                    Ptr = TagEnd + 4;
                    continue;
                }
            }
            ++Ptr;
        }

        return TagCount;
    }

    bool QuestManager::FindRadarNpcPosition(U8 NpcType, U16 NpcId, Vector3& OutPos)
    {
        if (!Offsets.RadarCountAddr || !Offsets.RadarArrayAddr)
            return false;

        U32 Count = 0;
        if (!Memory::ReadSafe(Offsets.RadarCountAddr, &Count) || Count == 0)
            return false;

        U64 ArrayPtr = 0;
        if (!Memory::ReadSafe(Offsets.RadarArrayAddr, &ArrayPtr) || !ArrayPtr)
            return false;

        constexpr U32 MaxRadarEntries = 512;
        U32 Total = (Count > MaxRadarEntries) ? MaxRadarEntries : Count;

        for (U32 I = 0; I < Total; ++I)
        {
            U64 EntryPtr = ArrayPtr + I * 48;
            U8 Type = 0;
            U16 Id = 0;
            Memory::ReadSafe(EntryPtr + 24, &Type);
            Memory::ReadSafe(EntryPtr + 28, &Id);

            if (Type == NpcType && Id == NpcId)
            {
                Memory::ReadSafe(EntryPtr, &OutPos.X);
                Memory::ReadSafe(EntryPtr + 4, &OutPos.Y);
                Memory::ReadSafe(EntryPtr + 8, &OutPos.Z);
                F32 GroundY = NavigationManager::GetGroundHeight(OutPos.X, OutPos.Z);
                if (GroundY != 0.0f)
                    OutPos.Y = GroundY;
                return true;
            }
        }

        return false;
    }

    bool QuestManager::FindNpcPosition(U32 NpcId, Vector3& OutPos)
    {
        if (!Offsets.NpcFile || NpcId == 0)
            return false;

        U32 TotalNpcs = 0;
        U64 NpcsArrayPtr = 0;

        if (!Memory::ReadSafe(Offsets.NpcFile + 24, &TotalNpcs) || TotalNpcs == 0)
            return false;

        if (!Memory::ReadSafe(Offsets.NpcFile + 32, &NpcsArrayPtr) || !NpcsArrayPtr)
            return false;

        for (U32 I = 0; I < TotalNpcs; ++I)
        {
            U64 NpcRec = NpcsArrayPtr + I * 544;
            U16 CurrentId = 0;
            Memory::ReadSafe(NpcRec + 2, &CurrentId);

            if (CurrentId == (U16)NpcId)
            {
                Memory::ReadSafe(NpcRec + 44, &OutPos.X);
                Memory::ReadSafe(NpcRec + 48, &OutPos.Y);
                Memory::ReadSafe(NpcRec + 52, &OutPos.Z);
                F32 GroundY = NavigationManager::GetGroundHeight(OutPos.X, OutPos.Z);
                if (GroundY != 0.0f)
                    OutPos.Y = GroundY;
                return true;
            }
        }

        return false;
    }

    void QuestManager::Update()
    {
        UpdateActiveQuests();
        UpdateRadarMarkers();
        AutoRecordNearbyQuestMobs();
    }

    void QuestManager::UpdateActiveQuests()
    {
        Quests.Clear();

        if (!Offsets.QuestVector)
            return;

        U64 FirstPtr = 0;
        U64 LastPtr = 0;
        if (!Memory::ReadSafe(Offsets.QuestVector + 8, &FirstPtr) || !FirstPtr)
            return;

        if (!Memory::ReadSafe(Offsets.QuestVector + 16, &LastPtr) || !LastPtr || LastPtr <= FirstPtr)
            return;

        U64 Count = (LastPtr - FirstPtr) / sizeof(U64);
        if (Count > 16)
            Count = 16;

        const PlayerData& Player = EntityManager::GetLocalPlayer();

        U32 TotalDbQuests = 0;
        U64 DbQuestsArray = 0;
        if (Offsets.NpcFile)
        {
            Memory::ReadSafe(Offsets.NpcFile + 8, &TotalDbQuests);
            Memory::ReadSafe(Offsets.NpcFile + 16, &DbQuestsArray);
        }

        for (U32 I = 0; I < (U32)Count; ++I)
        {
            U64 QuestDataPtr = 0;
            if (!Memory::ReadSafe(FirstPtr + I * sizeof(U64), &QuestDataPtr) || !QuestDataPtr)
                continue;

            ActiveQuest Quest;
            Quest.QuestId = 0;
            Quest.Step = 0;
            Quest.EndNpcType = 0;
            Quest.EndNpcId = 0;
            Quest.HasDestination = false;
            Quest.Distance = 0.0f;
            Quest.ObjectiveCount = 0;
            Quest.ItemObjectiveCount = 0;
            Quest.Title[0] = '\0';
            Quest.DestinationName[0] = '\0';

            Memory::ReadSafe(QuestDataPtr, &Quest.QuestId);
            Memory::ReadSafe(QuestDataPtr + 8, &Quest.Step);

            if (Quest.QuestId == 0)
                continue;

            char TargetTags[4][64];
            U32 FoundTags = 0;

            if (Offsets.QuestTextTablePtr && !Offsets.QuestTextTable)
                Memory::ReadSafe(Offsets.QuestTextTablePtr, &Offsets.QuestTextTable);

            // 1. Read Quest Text, Mob Objectives, and Item Collection Objectives
            if (Offsets.QuestTextTable)
            {
                U64 QuestTxtRec = Offsets.QuestTextTable + (Quest.QuestId - 1) * 488;

                // Title string
                U64 TitlePtr = 0;
                if (Memory::ReadSafe(QuestTxtRec + 8, &TitlePtr) && TitlePtr)
                {
                    char TempTitle[64] = { 0 };
                    if (Memory::ReadBytesSafe(TitlePtr, TempTitle, sizeof(TempTitle) - 1))
                        StringUtils::Copy(Quest.Title, TempTitle, sizeof(Quest.Title));
                }

                // Description and target tags
                U64 DescPtr = 0;
                if (Memory::ReadSafe(QuestTxtRec + 16, &DescPtr) && DescPtr)
                {
                    char TempDesc[300] = { 0 };
                    if (Memory::ReadBytesSafe(DescPtr, TempDesc, sizeof(TempDesc) - 1))
                    {
                        FoundTags = ExtractAllTagsFromDescription(TempDesc, TargetTags);
                    }
                }

                // Mob hunt objectives
                U16 Mob1Id = 0;
                U8 Needed1 = 0;
                U8 Current1 = 0;
                Memory::ReadSafe(QuestTxtRec + 106, &Mob1Id);
                Memory::ReadSafe(QuestTxtRec + 110, &Needed1);
                Memory::ReadSafe(QuestDataPtr + 4, &Current1);

                if (Mob1Id > 0 && Needed1 > 0)
                {
                    auto& Obj = Quest.Objectives[Quest.ObjectiveCount];
                    Obj.TargetMobId = Mob1Id;
                    Obj.CountNeeded = Needed1;
                    Obj.CurrentCount = Current1;
                    Obj.Completed = (Current1 >= Needed1);
                    if (!EntityManager::ResolveMonsterInfo(Mob1Id, Obj.TargetMobName, sizeof(Obj.TargetMobName)))
                    {
                        StringUtils::Format(Obj.TargetMobName, sizeof(Obj.TargetMobName), "Mob #%u", Mob1Id);
                    }
                    Quest.ObjectiveCount++;
                }

                U16 Mob2Id = 0;
                U8 Needed2 = 0;
                U8 Current2 = 0;
                Memory::ReadSafe(QuestTxtRec + 108, &Mob2Id);
                Memory::ReadSafe(QuestTxtRec + 111, &Needed2);
                Memory::ReadSafe(QuestDataPtr + 5, &Current2);

                if (Mob2Id > 0 && Needed2 > 0)
                {
                    auto& Obj = Quest.Objectives[Quest.ObjectiveCount];
                    Obj.TargetMobId = Mob2Id;
                    Obj.CountNeeded = Needed2;
                    Obj.CurrentCount = Current2;
                    Obj.Completed = (Current2 >= Needed2);
                    if (!EntityManager::ResolveMonsterInfo(Mob2Id, Obj.TargetMobName, sizeof(Obj.TargetMobName)))
                    {
                        StringUtils::Format(Obj.TargetMobName, sizeof(Obj.TargetMobName), "Mob #%u", Mob2Id);
                    }
                    Quest.ObjectiveCount++;
                }

                // Item Collection objectives (up to 3)
                // Slot 1: Type at 96, TypeId at 97, CountNeeded at 98
                // Slot 2: Type at 99, TypeId at 100, CountNeeded at 101
                // Slot 3: Type at 102, TypeId at 103, CountNeeded at 104
                constexpr U32 ItemOffsets[3] = { 96, 99, 102 };
                for (U32 ItemIdx = 0; ItemIdx < 3; ++ItemIdx)
                {
                    U8 IType = 0;
                    U8 ITypeId = 0;
                    U8 INeeded = 0;
                    Memory::ReadSafe(QuestTxtRec + ItemOffsets[ItemIdx], &IType);
                    Memory::ReadSafe(QuestTxtRec + ItemOffsets[ItemIdx] + 1, &ITypeId);
                    Memory::ReadSafe(QuestTxtRec + ItemOffsets[ItemIdx] + 2, &INeeded);

                    if (IType > 0 && INeeded > 0)
                    {
                        auto& Obj = Quest.ItemObjectives[Quest.ItemObjectiveCount];
                        Obj.ItemType = IType;
                        Obj.ItemTypeId = ITypeId;
                        Obj.CountNeeded = INeeded;
                        Obj.CurrentCount = static_cast<U8>(CountInventoryItem(IType, ITypeId));
                        Obj.Completed = (Obj.CurrentCount >= Obj.CountNeeded);
                        Obj.DroppedByMobId = 0;
                        Obj.DroppedByMobName[0] = '\0';

                        GroundItemManager::ResolveItemName(IType, ITypeId, Obj.ItemName, sizeof(Obj.ItemName));

                        // 1. Check native quest item drop mapping directly from ItemDb monster definitions
                        U16 DroppedMid = EntityManager::FindMobIdByQuestDrop(IType, ITypeId, Obj.DroppedByMobName, sizeof(Obj.DroppedByMobName));
                        if (DroppedMid > 0)
                        {
                            Obj.DroppedByMobId = DroppedMid;
                        }
                        else
                        {
                            // 2. Try each extracted tag to see if it matches a mob in ItemDb
                            for (U32 T = 0; T < FoundTags; ++T)
                            {
                                U16 Mid = EntityManager::FindMobIdByMatchingName(TargetTags[T], Obj.DroppedByMobName, sizeof(Obj.DroppedByMobName));
                                if (Mid > 0)
                                {
                                    Obj.DroppedByMobId = Mid;
                                    break;
                                }
                            }

                            // 3. Fallback: If no tag matched a mob, try matching with item name
                            if (Obj.DroppedByMobId == 0 && Obj.ItemName[0] != '\0')
                            {
                                Obj.DroppedByMobId = EntityManager::FindMobIdByMatchingName(Obj.ItemName, Obj.DroppedByMobName, sizeof(Obj.DroppedByMobName));
                            }
                        }

                        Quest.ItemObjectiveCount++;
                    }
                }
            }

            if (Quest.Title[0] == '\0')
                StringUtils::Format(Quest.Title, sizeof(Quest.Title), "Quest #%u", Quest.QuestId);

            // 2. Read End NPC info from QuestTextTable record (+0x5C = Type, +0x5E = Id)
            if (Offsets.QuestTextTable)
            {
                U64 QuestTxtRec = Offsets.QuestTextTable + (Quest.QuestId - 1) * 488;
                Memory::ReadSafe(QuestTxtRec + 0x5C, &Quest.EndNpcType);
                Memory::ReadSafe(QuestTxtRec + 0x5E, &Quest.EndNpcId);
            }

            if (Quest.EndNpcType > 0 || Quest.EndNpcId > 0)
            {
                Quest.HasDestination = FindRadarNpcPosition(Quest.EndNpcType, Quest.EndNpcId, Quest.DestinationPos);
                if (Quest.HasDestination)
                {
                    if (Player.Valid)
                        Quest.Distance = Quest.DestinationPos.DistanceTo(Player.Position);
                    StringUtils::Format(Quest.DestinationName, sizeof(Quest.DestinationName), "NPC (%u-%u)", Quest.EndNpcType, Quest.EndNpcId);
                }
                else
                {
                    // Fallback to CNPCFile only if radar entry not found on current map
                    if (FindNpcPosition(Quest.EndNpcId, Quest.DestinationPos))
                    {
                        Quest.HasDestination = true;
                        if (Player.Valid)
                            Quest.Distance = Quest.DestinationPos.DistanceTo(Player.Position);
                        StringUtils::Format(Quest.DestinationName, sizeof(Quest.DestinationName), "NPC #%u", Quest.EndNpcId);
                    }
                    else
                    {
                        Quest.HasDestination = false;
                        StringUtils::Format(Quest.DestinationName, sizeof(Quest.DestinationName), "NPC (%u-%u)", Quest.EndNpcType, Quest.EndNpcId);
                    }
                }
            }
            else
            {
                Quest.HasDestination = false;
                StringUtils::Copy(Quest.DestinationName, "-", sizeof(Quest.DestinationName));
            }

            Quests.Add(Quest);
        }
    }

    void QuestManager::UpdateRadarMarkers()
    {
        if (!Offsets.QuestMarkerList || !Offsets.WorldManager)
            return;

        const PlayerData& Player = EntityManager::GetLocalPlayer();

        struct LiveNpcInfo
        {
            Vector3 Pos;
            char Name[64];
        };
        FixedList<LiveNpcInfo, 64> LiveNpcs;

        U64 NpcMapAddr = Offsets.WorldManager + Offsets.NpcMapOffset;
        U64 NpcHeadNode = 0;
        if (Memory::ReadSafe(NpcMapAddr + 0x10, &NpcHeadNode) && NpcHeadNode)
        {
            U64 NpcCurr = 0;
            Memory::ReadSafe(NpcHeadNode, &NpcCurr);
            U32 Walk = 0;
            while (NpcCurr && NpcCurr != NpcHeadNode && Walk < 64)
            {
                U64 NpcPtr = 0;
                Memory::ReadSafe(NpcCurr + 24, &NpcPtr);
                if (NpcPtr)
                {
                    LiveNpcInfo Info;
                    Info.Name[0] = '\0';
                    Memory::ReadSafe(NpcPtr + Offsets.NpcPosX, &Info.Pos.X);
                    Memory::ReadSafe(NpcPtr + Offsets.NpcPosY, &Info.Pos.Y);
                    Memory::ReadSafe(NpcPtr + Offsets.NpcPosZ, &Info.Pos.Z);
                    Memory::ReadBytesSafe(NpcPtr + Offsets.NpcName, Info.Name, sizeof(Info.Name) - 1);
                    LiveNpcs.Add(Info);
                }
                Memory::ReadSafe(NpcCurr, &NpcCurr);
                ++Walk;
            }
        }

        U64 Node = 0;
        if (!Memory::ReadSafe(Offsets.QuestMarkerList + 0x10, &Node) || !Node)
            return;

        FixedList<QuestMarker, 32> NewMarkers;
        U32 MarkerWalk = 0;
        while (Node && Node != Offsets.QuestMarkerList && MarkerWalk < 32)
        {
            U64 ItemPtr = 0;
            U64 NextNode = 0;
            Memory::ReadSafe(Node, &ItemPtr);
            Memory::ReadSafe(Node + 0x10, &NextNode);

            if (ItemPtr && ItemPtr > 0x10000)
            {
                F32 Fx = 0.0f;
                F32 Fz = 0.0f;
                U8 IsStart = 0;
                U16 Qid = 0;

                Memory::ReadSafe(ItemPtr, &Fx);
                Memory::ReadSafe(ItemPtr + 4, &Fz);
                Memory::ReadSafe(ItemPtr + 8, &IsStart);
                Memory::ReadSafe(ItemPtr + 10, &Qid);

                QuestMarker Marker;
                Marker.NpcName[0] = '\0';
                Marker.IsTurnIn = (IsStart == 0);
                Marker.QuestId = Qid;
                Marker.Position.X = Fx;
                Marker.Position.Z = Fz;
                Marker.Position.Y = NavigationManager::GetGroundHeight(Fx, Fz);
                if (Marker.Position.Y == 0.0f && Player.Valid)
                    Marker.Position.Y = Player.Position.Y;
                StringUtils::Copy(Marker.NpcName, Marker.IsTurnIn ? "Turn-in NPC" : "Quest NPC", sizeof(Marker.NpcName));

                for (U32 N = 0; N < LiveNpcs.GetCount(); ++N)
                {
                    const LiveNpcInfo& Npc = LiveNpcs[N];
                    F32 Dx = Npc.Pos.X - Fx;
                    F32 Dz = Npc.Pos.Z - Fz;
                    if (Dx < 0.0f) Dx = -Dx;
                    if (Dz < 0.0f) Dz = -Dz;

                    if (Dx < 3.0f && Dz < 3.0f)
                    {
                        Marker.Position = Npc.Pos;
                        StringUtils::Copy(Marker.NpcName, Npc.Name, sizeof(Marker.NpcName));
                        break;
                    }
                }

                if (Player.Valid)
                    Marker.Distance = Marker.Position.DistanceTo(Player.Position);
                else
                    Marker.Distance = 0.0f;

                NewMarkers.Add(Marker);
            }

            Node = NextNode;
            ++MarkerWalk;
        }

        if (NewMarkers.GetCount() > 0 || Quests.GetCount() == 0)
        {
            Markers = NewMarkers;
        }
    }
}
