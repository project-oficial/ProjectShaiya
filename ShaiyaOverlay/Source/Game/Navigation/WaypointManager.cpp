#include "WaypointManager.h"
#include "NavigationManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

namespace ShaiyaOverlay
{
    FixedList<CustomWaypoint, 32> WaypointManager::CustomWaypoints;
    FixedList<UsefulNpc, 256> WaypointManager::UsefulNpcs;
    U32 WaypointManager::LastScanTick = 0;
    bool WaypointManager::Initialized = false;

    void WaypointManager::Initialize()
    {
        if (Initialized)
            return;

        LoadCustomWaypoints();
        Initialized = true;
    }

    void WaypointManager::Update()
    {
        if (!Initialized)
            Initialize();

        const PlayerData& Player = EntityManager::GetLocalPlayer();

        // Update custom waypoints distances
        if (Player.Valid)
        {
            for (U32 i = 0; i < CustomWaypoints.GetCount(); ++i)
            {
                CustomWaypoints[i].Distance = CustomWaypoints[i].Position.DistanceTo(Player.Position);
            }
        }

        // Refresh NPCs every 1500ms
        U32 now = GetTickCount();
        if (now - LastScanTick > 1500)
        {
            LastScanTick = now;
            RefreshUsefulNpcs();
        }
    }

    bool WaypointManager::AddCustomWaypoint(const char* Name, const Vector3& Pos)
    {
        if (!Name || Name[0] == '\0')
            return false;

        if (CustomWaypoints.GetCount() >= CustomWaypoints.GetCapacity())
            return false;

        CustomWaypoint wp;
        StringUtils::Copy(wp.Name, Name, sizeof(wp.Name));
        wp.Position = Pos;
        wp.Distance = 0.0f;

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        if (Player.Valid)
            wp.Distance = wp.Position.DistanceTo(Player.Position);

        bool added = CustomWaypoints.Add(wp);
        if (added)
            SaveCustomWaypoints();

        return added;
    }

    bool WaypointManager::RemoveCustomWaypoint(U32 Index)
    {
        if (Index >= CustomWaypoints.GetCount())
            return false;

        bool removed = CustomWaypoints.RemoveAt(Index);
        if (removed)
            SaveCustomWaypoints();

        return removed;
    }

    void WaypointManager::SaveCustomWaypoints()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\waypoints.ini");

        // Clear existing section
        WritePrivateProfileStringA("Waypoints", nullptr, nullptr, IniPath);

        char bufVal[64];
        snprintf(bufVal, sizeof(bufVal), "%u", CustomWaypoints.GetCount());
        WritePrivateProfileStringA("Waypoints", "Count", bufVal, IniPath);

        for (U32 i = 0; i < CustomWaypoints.GetCount(); ++i)
        {
            const auto& wp = CustomWaypoints[i];
            char keyName[32], keyX[32], keyY[32], keyZ[32];
            snprintf(keyName, sizeof(keyName), "WP%u_Name", i);
            snprintf(keyX, sizeof(keyX), "WP%u_X", i);
            snprintf(keyY, sizeof(keyY), "WP%u_Y", i);
            snprintf(keyZ, sizeof(keyZ), "WP%u_Z", i);

            WritePrivateProfileStringA("Waypoints", keyName, wp.Name, IniPath);

            snprintf(bufVal, sizeof(bufVal), "%.2f", wp.Position.X);
            WritePrivateProfileStringA("Waypoints", keyX, bufVal, IniPath);

            snprintf(bufVal, sizeof(bufVal), "%.2f", wp.Position.Y);
            WritePrivateProfileStringA("Waypoints", keyY, bufVal, IniPath);

            snprintf(bufVal, sizeof(bufVal), "%.2f", wp.Position.Z);
            WritePrivateProfileStringA("Waypoints", keyZ, bufVal, IniPath);
        }
    }

    void WaypointManager::LoadCustomWaypoints()
    {
        CustomWaypoints.Clear();

        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\waypoints.ini");

        U32 count = GetPrivateProfileIntA("Waypoints", "Count", 0, IniPath);
        if (count > CustomWaypoints.GetCapacity())
            count = CustomWaypoints.GetCapacity();

        for (U32 i = 0; i < count; ++i)
        {
            char keyName[32], keyX[32], keyY[32], keyZ[32];
            snprintf(keyName, sizeof(keyName), "WP%u_Name", i);
            snprintf(keyX, sizeof(keyX), "WP%u_X", i);
            snprintf(keyY, sizeof(keyY), "WP%u_Y", i);
            snprintf(keyZ, sizeof(keyZ), "WP%u_Z", i);

            CustomWaypoint wp;
            wp.Name[0] = '\0';
            GetPrivateProfileStringA("Waypoints", keyName, "", wp.Name, sizeof(wp.Name), IniPath);

            if (wp.Name[0] == '\0')
                continue;

            char bufVal[64];
            GetPrivateProfileStringA("Waypoints", keyX, "0", bufVal, sizeof(bufVal), IniPath);
            wp.Position.X = static_cast<F32>(atof(bufVal));

            GetPrivateProfileStringA("Waypoints", keyY, "0", bufVal, sizeof(bufVal), IniPath);
            wp.Position.Y = static_cast<F32>(atof(bufVal));

            GetPrivateProfileStringA("Waypoints", keyZ, "0", bufVal, sizeof(bufVal), IniPath);
            wp.Position.Z = static_cast<F32>(atof(bufVal));

            wp.Distance = 0.0f;
            CustomWaypoints.Add(wp);
        }
    }

    bool WaypointManager::ResolveNpcName(U8 NpcType, U16 NpcId, char* OutName, U32 MaxLen)
    {
        if (!Offsets.NpcFile || NpcType == 0 || NpcId == 0 || !OutName || MaxLen == 0)
            return false;

        OutName[0] = '\0';
        U64 rec = 0;

        if (NpcType == 1) // Merchant
        {
            U32 total = 0;
            U64 arr = 0;
            Memory::ReadSafe(Offsets.NpcFile + 0x08, &total);
            Memory::ReadSafe(Offsets.NpcFile + 0x10, &arr);
            if (arr && NpcId <= total)
                rec = arr + (static_cast<U64>(NpcId) - 1) * 656;
        }
        else if (NpcType == 2) // Gatekeeper
        {
            U32 total = 0;
            U64 arr = 0;
            Memory::ReadSafe(Offsets.NpcFile + 0x18, &total);
            Memory::ReadSafe(Offsets.NpcFile + 0x20, &arr);
            if (arr && NpcId <= total)
                rec = arr + (static_cast<U64>(NpcId) - 1) * 544;
        }
        else if (NpcType >= 3 && NpcType <= 13) // Categories 0..10
        {
            U32 cat = NpcType - 3;
            U32 total = 0;
            U64 arr = 0;
            Memory::ReadSafe(Offsets.NpcFile + 0x28 + cat * 4, &total);
            Memory::ReadSafe(Offsets.NpcFile + 0x60 + cat * 8, &arr);
            if (arr && NpcId <= total)
                rec = arr + (static_cast<U64>(NpcId) - 1) * 448;
        }

        if (rec)
        {
            // 1. Translated string pointer at record + 24
            U64 namePtr = 0;
            if (Memory::ReadSafe(rec + 24, &namePtr) && namePtr > 0x10000)
            {
                Memory::ReadBytesSafe(namePtr, OutName, MaxLen - 1);
                StringUtils::NormalizeAccents(OutName, MaxLen, false);
                if (OutName[0] != '\0')
                    return true;
            }

            // 2. Wide char fallback if present in record (e.g. +248, +140, +44)
            wchar_t wBuf[64] = { 0 };
            U32 wOffset = (NpcType == 1) ? 248 : ((NpcType == 2) ? 140 : 44);
            if (Memory::ReadBytesSafe(rec + wOffset, (char*)wBuf, sizeof(wBuf) - sizeof(wchar_t)) && wBuf[0] != L'\0')
            {
                WideCharToMultiByte(CP_ACP, 0, wBuf, -1, OutName, static_cast<int>(MaxLen), nullptr, nullptr);
                StringUtils::NormalizeAccents(OutName, MaxLen, false);
                if (OutName[0] != '\0')
                    return true;
            }
        }

        return false;
    }

    void WaypointManager::RefreshUsefulNpcs()
    {
        UsefulNpcs.Clear();

        if (!Offsets.RadarCountAddr || !Offsets.RadarArrayAddr)
            return;

        U32 RadarCount = 0;
        if (!Memory::ReadSafe(Offsets.RadarCountAddr, &RadarCount) || RadarCount == 0)
            return;

        U64 ArrayPtr = 0;
        if (!Memory::ReadSafe(Offsets.RadarArrayAddr, &ArrayPtr) || !ArrayPtr)
            return;

        const PlayerData& Player = EntityManager::GetLocalPlayer();

        // 1. Collect live streamed NPCs from WorldManager->NpcMap
        struct LiveNpc
        {
            Vector3 Pos;
            char Name[64];
        };
        FixedList<LiveNpc, 64> liveList;

        if (Offsets.WorldManager && Offsets.NpcMapOffset)
        {
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
                    Memory::ReadSafe(NpcCurr + 0x18, &NpcPtr);
                    if (NpcPtr)
                    {
                        LiveNpc info;
                        info.Name[0] = '\0';
                        Memory::ReadSafe(NpcPtr + Offsets.NpcPosX, &info.Pos.X);
                        Memory::ReadSafe(NpcPtr + Offsets.NpcPosY, &info.Pos.Y);
                        Memory::ReadSafe(NpcPtr + Offsets.NpcPosZ, &info.Pos.Z);
                        Memory::ReadBytesSafe(NpcPtr + Offsets.NpcName, info.Name, sizeof(info.Name) - 1);
                        StringUtils::NormalizeAccents(info.Name, sizeof(info.Name), false);
                        if (info.Name[0] != '\0')
                            liveList.Add(info);
                    }
                    Memory::ReadSafe(NpcCurr, &NpcCurr);
                    ++Walk;
                }
            }
        }

        // 2. Read static radar entries
        constexpr U32 MaxRadarEntries = 512;
        U32 Total = (RadarCount > MaxRadarEntries) ? MaxRadarEntries : RadarCount;

        for (U32 i = 0; i < Total && UsefulNpcs.GetCount() < UsefulNpcs.GetCapacity(); ++i)
        {
            U64 EntryPtr = ArrayPtr + i * 0x30;
            U8 Type = 0;
            U16 Id = 0;
            Vector3 Pos;

            Memory::ReadSafe(EntryPtr, &Pos.X);
            Memory::ReadSafe(EntryPtr + 0x04, &Pos.Y);
            Memory::ReadSafe(EntryPtr + 0x08, &Pos.Z);
            Memory::ReadSafe(EntryPtr + 0x18, &Type);
            Memory::ReadSafe(EntryPtr + 0x1C, &Id);

            if (Type == 0 || Id == 0)
                continue;

            UsefulNpc npc;
            npc.Type = Type;
            npc.Id = Id;
            npc.Position = Pos;

            F32 GroundY = NavigationManager::GetGroundHeight(Pos.X, Pos.Z);
            if (GroundY != 0.0f)
                npc.Position.Y = GroundY;

            npc.Distance = Player.Valid ? npc.Position.DistanceTo(Player.Position) : 0.0f;
            npc.Name[0] = '\0';

            switch (Type)
            {
                case 1:  StringUtils::Copy(npc.Role, "Merchant", sizeof(npc.Role)); break;
                case 2:  StringUtils::Copy(npc.Role, "Gatekeeper", sizeof(npc.Role)); break;
                case 3:  StringUtils::Copy(npc.Role, "Quest", sizeof(npc.Role)); break;
                case 6:  StringUtils::Copy(npc.Role, "Warehouse", sizeof(npc.Role)); break;
                case 7:  StringUtils::Copy(npc.Role, "Blacksmith", sizeof(npc.Role)); break;
                case 8:  StringUtils::Copy(npc.Role, "Guard", sizeof(npc.Role)); break;
                case 10: StringUtils::Copy(npc.Role, "Service", sizeof(npc.Role)); break;
                default: StringUtils::Copy(npc.Role, "NPC", sizeof(npc.Role)); break;
            }

            // 1. Resolve name from static database table (covers 100% of all static map NPCs)
            ResolveNpcName(Type, Id, npc.Name, sizeof(npc.Name));

            // 2. If still empty, check live NPC stream
            if (npc.Name[0] == '\0')
            {
                for (U32 l = 0; l < liveList.GetCount(); ++l)
                {
                    if (liveList[l].Pos.DistanceTo(npc.Position) < 4.0f)
                    {
                        StringUtils::Copy(npc.Name, liveList[l].Name, sizeof(npc.Name));
                        break;
                    }
                }
            }

            // 3. Fallback only if both failed
            if (npc.Name[0] == '\0')
            {
                snprintf(npc.Name, sizeof(npc.Name), "%s #%u", npc.Role, npc.Id);
            }

            // Deduplicate: avoid duplicate entries at same coordinates
            bool dup = false;
            for (U32 u = 0; u < UsefulNpcs.GetCount(); ++u)
            {
                if (UsefulNpcs[u].Position.DistanceTo(npc.Position) < 2.0f)
                {
                    dup = true;
                    break;
                }
            }
            if (!dup)
            {
                UsefulNpcs.Add(npc);
            }
        }

        // Sort by distance ascending
        U32 n = UsefulNpcs.GetCount();
        for (U32 a = 0; a < n; ++a)
        {
            for (U32 b = a + 1; b < n; ++b)
            {
                if (UsefulNpcs[b].Distance < UsefulNpcs[a].Distance)
                {
                    UsefulNpc tmp = UsefulNpcs[a];
                    UsefulNpcs[a] = UsefulNpcs[b];
                    UsefulNpcs[b] = tmp;
                }
            }
        }
    }
}
