#include "EntityManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"

namespace ShaiyaOverlay
{
    PlayerData EntityManager::CurrentPlayer = { 0 };
    FixedList<MonsterEntity, 128> EntityManager::NearbyMonsters;

    bool EntityManager::ResolveMonsterInfo(U16 MobId, char* OutName, U32 MaxLen, U16* OutLevel)
    {
        if (!Offsets.ItemDb || MobId == 0 || !OutName || MaxLen == 0)
            return false;

        U64 NilNode = 0;
        U64 BucketsPtr = 0;
        U64 Mask = 0;

        if (!Memory::ReadSafe(Offsets.ItemDb + 34 * 8, &NilNode)) return false;
        if (!Memory::ReadSafe(Offsets.ItemDb + 36 * 8, &BucketsPtr)) return false;
        if (!Memory::ReadSafe(Offsets.ItemDb + 39 * 8, &Mask)) return false;

        U64 H1 = (static_cast<U64>(MobId & 0xFF) ^ 0xCBF29CE484222325ULL) * 0x100000001B3ULL;
        U64 H2 = ((static_cast<U64>(MobId) >> 8) ^ H1) * 0x8A97B0004E7FEABULL;
        U64 Bucket = H2 & Mask;

        U64 FirstInBucket = 0;
        U64 BucketEntry = 0;
        if (!Memory::ReadSafe(BucketsPtr + Bucket * 16, &FirstInBucket)) return false;
        if (!Memory::ReadSafe(BucketsPtr + Bucket * 16 + 8, &BucketEntry)) return false;

        U64 Node = BucketEntry;
        if (Node != NilNode)
        {
            U32 Key = 0;
            Memory::ReadSafe(Node + 16, &Key);
            if (Key == MobId)
            {
                U64 NamePtr = 0;
                Memory::ReadSafe(Node + 24, &NamePtr);
                if (OutLevel)
                    Memory::ReadSafe(Node + 24 + 10, OutLevel);
                if (NamePtr)
                {
                    char Temp[64] = { 0 };
                    if (Memory::ReadBytesSafe(NamePtr, Temp, sizeof(Temp) - 1))
                    {
                        StringUtils::Copy(OutName, Temp, MaxLen);
                        return true;
                    }
                }
                return false;
            }

            U32 Walk = 0;
            while (Node != FirstInBucket && Walk < 64)
            {
                if (!Memory::ReadSafe(Node + 8, &Node) || !Node) break;
                Memory::ReadSafe(Node + 16, &Key);
                if (Key == MobId)
                {
                    U64 NamePtr = 0;
                    Memory::ReadSafe(Node + 24, &NamePtr);
                    if (OutLevel)
                        Memory::ReadSafe(Node + 24 + 10, OutLevel);
                    if (NamePtr)
                    {
                        char Temp[64] = { 0 };
                        if (Memory::ReadBytesSafe(NamePtr, Temp, sizeof(Temp) - 1))
                        {
                            StringUtils::Copy(OutName, Temp, MaxLen);
                            return true;
                        }
                    }
                    return false;
                }
                ++Walk;
            }
        }

        return false;
    }

    U16 EntityManager::FindMobIdByMatchingName(const char* NameQuery, char* OutFullName, U32 MaxLen)
    {
        if (!Offsets.ItemDb || !NameQuery || NameQuery[0] == '\0')
            return 0;

        U64 NilNode = 0;
        U64 BucketsPtr = 0;
        U64 Mask = 0;

        if (!Memory::ReadSafe(Offsets.ItemDb + 34 * 8, &NilNode)) return 0;
        if (!Memory::ReadSafe(Offsets.ItemDb + 36 * 8, &BucketsPtr)) return 0;
        if (!Memory::ReadSafe(Offsets.ItemDb + 39 * 8, &Mask)) return 0;

        U32 BucketCount = static_cast<U32>(Mask + 1);
        if (BucketCount > 16384) BucketCount = 16384;

        for (U32 B = 0; B < BucketCount; ++B)
        {
            U64 FirstInBucket = 0;
            U64 BucketEntry = 0;
            if (!Memory::ReadSafe(BucketsPtr + B * 16, &FirstInBucket)) continue;
            if (!Memory::ReadSafe(BucketsPtr + B * 16 + 8, &BucketEntry)) continue;

            U64 Node = BucketEntry;
            U32 Walk = 0;
            while (Node && Node != NilNode && Walk < 64)
            {
                U32 Key = 0;
                U64 NamePtr = 0;
                Memory::ReadSafe(Node + 16, &Key);
                Memory::ReadSafe(Node + 24, &NamePtr);

                if (NamePtr && Key > 0)
                {
                    char Temp[64] = { 0 };
                    if (Memory::ReadBytesSafe(NamePtr, Temp, sizeof(Temp) - 1))
                    {
                        if (StringUtils::Length(Temp) >= 3 && StringUtils::ContainsNormalized(Temp, NameQuery))
                        {
                            if (OutFullName && MaxLen > 0)
                                StringUtils::Copy(OutFullName, Temp, MaxLen);
                            return static_cast<U16>(Key);
                        }
                    }
                }

                if (Node == FirstInBucket) break;
                if (!Memory::ReadSafe(Node + 8, &Node)) break;
                ++Walk;
            }
        }

        return 0;
    }

    void EntityManager::Update()
    {
        UpdateLocalPlayer(0);
        UpdateMonsters(0);
    }

    void EntityManager::UpdateLocalPlayer(U64 ImageBase)
    {
        CurrentPlayer.Valid = false;

        U32 PlayerId = 0;
        if (!Memory::ReadSafe(Offsets.PlayerId, &PlayerId) || PlayerId == 0)
            return;

        CurrentPlayer.Id = PlayerId;
        Memory::ReadSafe(Offsets.PlayerLevel, &CurrentPlayer.Level);
        Memory::ReadSafe(Offsets.PlayerCurrentHp, &CurrentPlayer.CurrentHp);
        Memory::ReadSafe(Offsets.PlayerMaxHp, &CurrentPlayer.MaxHp);

        // Retrieve position from CWorldMgr character map
        U64 CharacterMapAddr = Offsets.WorldManager + Offsets.CharacterMapOffset;
        U64 HeadNode = 0;
        if (!Memory::ReadSafe(CharacterMapAddr + 0x10, &HeadNode) || !HeadNode)
            return;

        U64 CurrentNode = 0;
        if (!Memory::ReadSafe(HeadNode, &CurrentNode) || !CurrentNode)
            return;

        U32 MaxIterations = 16;
        while (CurrentNode && CurrentNode != HeadNode && MaxIterations > 0)
        {
            U64 CharacterPtr = 0;
            Memory::ReadSafe(CurrentNode + 0x18, &CharacterPtr);

            if (CharacterPtr)
            {
                U32 CharId = 0;
                Memory::ReadSafe(CharacterPtr + Offsets.PlayerIdOffset, &CharId);

                if (CharId == PlayerId)
                {
                    Memory::ReadSafe(CharacterPtr + Offsets.PlayerPosX, &CurrentPlayer.Position.X);
                    Memory::ReadSafe(CharacterPtr + Offsets.PlayerPosY, &CurrentPlayer.Position.Y);
                    Memory::ReadSafe(CharacterPtr + Offsets.PlayerPosZ, &CurrentPlayer.Position.Z);
                    CurrentPlayer.Valid = true;
                    break;
                }
            }

            Memory::ReadSafe(CurrentNode, &CurrentNode);
            --MaxIterations;
        }
    }

    void EntityManager::UpdateMonsters(U64 ImageBase)
    {
        NearbyMonsters.Clear();

        U64 MonsterMapAddr = Offsets.WorldManager + Offsets.MonsterMapOffset;
        U64 HeadNode = 0;
        if (!Memory::ReadSafe(MonsterMapAddr, &HeadNode) || !HeadNode)
            return;

        U64 CurrentNode = 0;
        if (!Memory::ReadSafe(HeadNode, &CurrentNode) || !CurrentNode)
            return;

        U32 Iterations = 0;
        constexpr U32 MaxScanLimit = 256;

        while (CurrentNode && CurrentNode != HeadNode && Iterations < MaxScanLimit)
        {
            U64 NextNode = 0;
            Memory::ReadSafe(CurrentNode, &NextNode);

            U64 MonsterPtr = 0;
            Memory::ReadSafe(CurrentNode + 0x18, &MonsterPtr);

            if (MonsterPtr)
            {
                MonsterEntity Entity = { 0 };
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterWorldId, &Entity.WorldId);
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterMobId, &Entity.MobId);
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterLevel, &Entity.Level);
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterMaxHp, &Entity.MaxHp);
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterCurrentHp, &Entity.CurrentHp);

                Memory::ReadSafe(MonsterPtr + Offsets.MonsterPosX, &Entity.Position.X);
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterPosY, &Entity.Position.Y);
                Memory::ReadSafe(MonsterPtr + Offsets.MonsterPosZ, &Entity.Position.Z);

                Entity.Alive = (Entity.CurrentHp > 0);

                if (CurrentPlayer.Valid)
                    Entity.Distance = Entity.Position.DistanceTo(CurrentPlayer.Position);
                else
                    Entity.Distance = 0.0f;

                U16 ResolvedLvl = Entity.Level;
                if (!ResolveMonsterInfo(static_cast<U16>(Entity.MobId), Entity.Name, sizeof(Entity.Name), &ResolvedLvl))
                {
                    StringUtils::Format(Entity.Name, sizeof(Entity.Name), "Mob #%u (ID:%u)", Entity.MobId, Entity.WorldId);
                }
                else
                {
                    Entity.Level = ResolvedLvl;
                }

                // Simple threat grading
                if (!Entity.Alive)
                {
                    Entity.Threat = ThreatLevel::None;
                }
                else if (Entity.Distance < 12.0f)
                {
                    Entity.Threat = ThreatLevel::Fatal;
                }
                else if (Entity.Distance < 25.0f)
                {
                    Entity.Threat = ThreatLevel::High;
                }
                else if (Entity.Distance < 45.0f)
                {
                    Entity.Threat = ThreatLevel::Medium;
                }
                else
                {
                    Entity.Threat = ThreatLevel::Low;
                }

                NearbyMonsters.Add(Entity);
            }

            CurrentNode = NextNode;
            ++Iterations;
        }
    }
}
