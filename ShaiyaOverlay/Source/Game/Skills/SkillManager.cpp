#include "SkillManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"

namespace ShaiyaOverlay
{
    FixedList<SkillInfo, 64> SkillManager::Skills;

    bool SkillManager::ResolveSkillName(U16 SkillId, U16 Level, char* OutName, U32 MaxLen)
    {
        if (!OutName || MaxLen == 0 || SkillId == 0)
            return false;

        OutName[0] = '\0';

        if (!Offsets.ItemDb)
            return false;

        U64 SkillMapBase = Offsets.ItemDb + 0x88;
        U64 NilNode = 0;
        U64 BucketsPtr = 0;
        U64 Mask = 0;

        if (!Memory::ReadSafe(SkillMapBase + 0x08, &NilNode)) return false;
        if (!Memory::ReadSafe(SkillMapBase + 0x18, &BucketsPtr)) return false;
        if (!Memory::ReadSafe(SkillMapBase + 0x30, &Mask)) return false;

        if (!BucketsPtr || Mask == 0) return false;

        // FNV-1a hash of SkillId
        U64 H1 = (static_cast<U64>(SkillId & 0xFF) ^ 0xCBF29CE484222325ULL) * 0x100000001B3ULL;
        U64 H2 = ((static_cast<U64>(SkillId) >> 8) ^ H1) * 0x8A97B0004E7FEABULL;
        U64 Bucket = H2 & Mask;

        U64 FirstInBucket = 0;
        U64 BucketEntry = 0;
        if (!Memory::ReadSafe(BucketsPtr + Bucket * 16, &FirstInBucket)) return false;
        if (!Memory::ReadSafe(BucketsPtr + Bucket * 16 + 8, &BucketEntry)) return false;

        U64 Node = BucketEntry;
        U32 Walk = 0;
        while (Node && Node != NilNode && Walk < 64)
        {
            U32 Key = 0;
            Memory::ReadSafe(Node + 16, &Key);
            if (Key == SkillId)
            {
                U64 InnerNil = 0;
                U64 InnerBuckets = 0;
                U64 InnerMask = 0;
                Memory::ReadSafe(Node + 0x20, &InnerNil);
                Memory::ReadSafe(Node + 0x30, &InnerBuckets);
                Memory::ReadSafe(Node + 0x48, &InnerMask);

                // Try matching exact Level first
                if (InnerBuckets && InnerMask > 0)
                {
                    U64 LH1 = (static_cast<U64>(Level & 0xFF) ^ 0xCBF29CE484222325ULL) * 0x100000001B3ULL;
                    U64 LH2 = ((static_cast<U64>(Level) >> 8) ^ LH1) * 0x8A97B0004E7FEABULL;
                    U64 LvlBucket = LH2 & InnerMask;

                    U64 LvlFirst = 0;
                    U64 LvlEntry = 0;
                    if (Memory::ReadSafe(InnerBuckets + LvlBucket * 16, &LvlFirst) &&
                        Memory::ReadSafe(InnerBuckets + LvlBucket * 16 + 8, &LvlEntry))
                    {
                        U64 LNode = LvlEntry;
                        U32 LWalk = 0;
                        while (LNode && LNode != InnerNil && LWalk < 32)
                        {
                            U16 LSid = 0;
                            U16 LLvl = 0;
                            U64 NamePtr = 0;
                            Memory::ReadSafe(LNode + 0x18, &LSid);
                            Memory::ReadSafe(LNode + 0x1A, &LLvl);
                            Memory::ReadSafe(LNode + 0x20, &NamePtr);

                            if (LSid == SkillId && LLvl == Level && NamePtr)
                            {
                                char Temp[64] = { 0 };
                                if (Memory::ReadBytesSafe(NamePtr, Temp, sizeof(Temp) - 1))
                                {
                                    if (Temp[0] != '\0' && !StringUtils::Equals(Temp, "???") && !StringUtils::Equals(Temp, "placeholder"))
                                    {
                                        StringUtils::Copy(OutName, Temp, MaxLen);
                                        return true;
                                    }
                                }
                            }
                            if (LNode == LvlFirst) break;
                            if (!Memory::ReadSafe(LNode + 8, &LNode)) break;
                            ++LWalk;
                        }
                    }

                    // Fallback: iterate buckets in inner map to find any valid name for this skill
                    U32 ScanBuckets = static_cast<U32>(InnerMask + 1);
                    if (ScanBuckets > 16) ScanBuckets = 16;
                    for (U32 B = 0; B < ScanBuckets; ++B)
                    {
                        U64 FB = 0;
                        U64 EB = 0;
                        if (!Memory::ReadSafe(InnerBuckets + B * 16, &FB) ||
                            !Memory::ReadSafe(InnerBuckets + B * 16 + 8, &EB))
                            continue;

                        U64 LN = EB;
                        U32 LW = 0;
                        while (LN && LN != InnerNil && LW < 16)
                        {
                            U64 NP = 0;
                            Memory::ReadSafe(LN + 0x20, &NP);
                            if (NP)
                            {
                                char Temp[64] = { 0 };
                                if (Memory::ReadBytesSafe(NP, Temp, sizeof(Temp) - 1))
                                {
                                    if (Temp[0] != '\0' && !StringUtils::Equals(Temp, "???") && !StringUtils::Equals(Temp, "placeholder"))
                                    {
                                        StringUtils::Copy(OutName, Temp, MaxLen);
                                        return true;
                                    }
                                }
                            }
                            if (LN == FB) break;
                            if (!Memory::ReadSafe(LN + 8, &LN)) break;
                            ++LW;
                        }
                    }
                }
                return false;
            }

            if (Node == FirstInBucket) break;
            if (!Memory::ReadSafe(Node + 8, &Node)) break;
            ++Walk;
        }

        return false;
    }

    void SkillManager::Update()
    {
        Skills.Clear();

        U64 SkillVecAddr = Offsets.SkillVector;
        U64 FirstPtr = 0;
        U64 LastPtr = 0;

        if (!Memory::ReadSafe(SkillVecAddr + 8, &FirstPtr) || !FirstPtr)
            return;

        if (!Memory::ReadSafe(SkillVecAddr + 16, &LastPtr) || !LastPtr || LastPtr <= FirstPtr)
            return;

        U64 ElementCount = (LastPtr - FirstPtr) / sizeof(U64);
        if (ElementCount > 64)
            ElementCount = 64;

        for (U32 I = 0; I < (U32)ElementCount; ++I)
        {
            U64 SkillDataPtr = 0;
            if (!Memory::ReadSafe(FirstPtr + I * sizeof(U64), &SkillDataPtr) || !SkillDataPtr)
                continue;

            SkillInfo Info = { 0 };
            U8 Lvl = 0;
            Memory::ReadSafe(SkillDataPtr + 2, &Info.SkillId);
            Memory::ReadSafe(SkillDataPtr + 4, &Lvl);
            Info.Level = Lvl;

            if (Info.SkillId == 0)
                continue;

            Info.CooldownRemaining = 0.0f;
            Info.IsReady = true;

            if (!ResolveSkillName(Info.SkillId, Info.Level, Info.Name, sizeof(Info.Name)))
            {
                StringUtils::Format(Info.Name, sizeof(Info.Name), "Skill #%u", Info.SkillId);
            }

            Skills.Add(Info);
        }
    }
}
