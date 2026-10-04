#include "MCPBridge.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Core/Logger.h"
#include "Core/Camera.h"
#include "Hooks/WndProcHook.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/Items/InventoryManager.h"
#include "Game/Quests/QuestManager.h"
#include "Game/Navigation/NavigationManager.h"
#include "Game/Skills/SkillManager.h"
#include "Game/Buffs/BuffManager.h"
#include "Game/Login/AutoLoginManager.h"
#include "UI/Menu.h"

#include <windows.h>
#include <stdio.h>
#include <cmath>
#include <cstring>

#ifdef MCP_TOOL

namespace ShaiyaOverlay
{
    namespace MCPBridge
    {
        static volatile bool g_bServerRunning = false;
        static HANDLE g_hPipeThread = nullptr;
        static HANDLE g_hPipe = INVALID_HANDLE_VALUE;
        static HANDLE g_hShutdownEvent = nullptr;

        typedef void (*RenderThreadTaskFn)(void* pContext);
        static volatile RenderThreadTaskFn g_pRenderTask = nullptr;
        static void* volatile g_pRenderTaskContext = nullptr;
        static HANDLE g_hTaskDoneEvent = nullptr;

        void OnRenderTick()
        {
            if (g_pRenderTask)
            {
                auto fn = (RenderThreadTaskFn)InterlockedExchangePointer((void**)&g_pRenderTask, nullptr);
                if (fn)
                {
                    void* ctx = g_pRenderTaskContext;
                    __try
                    {
                        fn(ctx);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        Logger::Error("[MCP] Exception executing task on render thread");
                    }
                    if (g_hTaskDoneEvent)
                        SetEvent(g_hTaskDoneEvent);
                }
            }
        }

        bool RunOnRenderThreadInternal(void (*fn)(void*), void* ctx, uint32_t timeoutMs)
        {
            if (!fn)
                return false;

            if (!g_hTaskDoneEvent)
                g_hTaskDoneEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
            else
                ResetEvent(g_hTaskDoneEvent);

            g_pRenderTaskContext = ctx;
            InterlockedExchangePointer((void**)&g_pRenderTask, (void*)fn);

            DWORD waitRes = WaitForSingleObject(g_hTaskDoneEvent, timeoutMs);
            if (waitRes != WAIT_OBJECT_0)
            {
                InterlockedExchangePointer((void**)&g_pRenderTask, nullptr);
                g_pRenderTaskContext = nullptr;
                Logger::Error("[MCP] RunOnRenderThread timed out after %lu ms", timeoutMs);
                return false;
            }

            return true;
        }

        static bool extract_json_string(const char* json, const char* key, char* outVal, size_t maxLen)
        {
            char searchKey[64];
            sprintf_s(searchKey, "\"%s\"", key);
            const char* pKey = strstr(json, searchKey);
            if (!pKey) return false;
            const char* pColon = strchr(pKey + strlen(searchKey), ':');
            if (!pColon) return false;
            const char* pStart = strchr(pColon, '\"');
            if (!pStart) return false;
            pStart++;
            const char* pEnd = strchr(pStart, '\"');
            if (!pEnd) return false;
            size_t len = pEnd - pStart;
            if (len >= maxLen) len = maxLen - 1;
            memcpy(outVal, pStart, len);
            outVal[len] = '\0';
            return true;
        }

        static bool extract_json_bool(const char* json, const char* key, bool& outVal)
        {
            char searchKey[64];
            sprintf_s(searchKey, "\"%s\"", key);
            const char* pKey = strstr(json, searchKey);
            if (!pKey) return false;
            const char* pColon = strchr(pKey + strlen(searchKey), ':');
            if (!pColon) return false;
            while (*pColon == ':' || *pColon == ' ' || *pColon == '\t') pColon++;
            if (strncmp(pColon, "true", 4) == 0 || *pColon == '1')
            {
                outVal = true;
                return true;
            }
            if (strncmp(pColon, "false", 5) == 0 || *pColon == '0')
            {
                outVal = false;
                return true;
            }
            return false;
        }

        static bool extract_json_double(const char* json, const char* key, double& outVal)
        {
            char searchKey[64];
            sprintf_s(searchKey, "\"%s\"", key);
            const char* pKey = strstr(json, searchKey);
            if (!pKey) return false;
            const char* pColon = strchr(pKey + strlen(searchKey), ':');
            if (!pColon) return false;
            while (*pColon == ':' || *pColon == ' ' || *pColon == '\t' || *pColon == '\"') pColon++;
            outVal = atof(pColon);
            return true;
        }

        static uintptr_t parse_hex_or_dec(const char* s)
        {
            if (!s) return 0;
            while (*s == ' ' || *s == '\t') s++;
            uintptr_t val = 0;
            if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            {
                s += 2;
                while (*s)
                {
                    char c = *s++;
                    if (c >= '0' && c <= '9') val = (val << 4) | (c - '0');
                    else if (c >= 'a' && c <= 'f') val = (val << 4) | (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') val = (val << 4) | (c - 'A' + 10);
                    else break;
                }
            }
            else
            {
                while (*s >= '0' && *s <= '9')
                {
                    val = val * 10 + (*s++ - '0');
                }
            }
            return val;
        }

        static void sanitize_string(const char* src, char* dst, size_t dstSize)
        {
            if (!src || dstSize == 0)
            {
                if (dstSize > 0) dst[0] = '\0';
                return;
            }
            size_t j = 0;
            for (size_t i = 0; src[i] && (j + 1) < dstSize; ++i)
            {
                char c = src[i];
                if (c == '\"' || c == '\\' || c == '\r' || c == '\n' || c == '\t')
                    c = ' ';
                dst[j++] = c;
            }
            dst[j] = '\0';
        }

        static void GetScreenDimensions(float& width, float& height)
        {
            width = 1920.0f;
            height = 1080.0f;
            HWND hWnd = WndProcHook::GetWindowHandle();
            if (!hWnd) hWnd = GetActiveWindow();
            if (hWnd)
            {
                RECT rc;
                if (GetClientRect(hWnd, &rc) && (rc.right - rc.left) > 0)
                {
                    width = static_cast<float>(rc.right - rc.left);
                    height = static_cast<float>(rc.bottom - rc.top);
                }
            }
        }

        static void HandleGetStatus(char* pResponse, size_t nMaxLen)
        {
            __try
            {
                const PlayerData& player = EntityManager::GetLocalPlayer();
                Vector3 navTarget = NavigationManager::GetTargetPosition();
                GameState state = AutoLoginManager::GetCurrentGameState();

                sprintf_s(pResponse, nMaxLen,
                    "{\"status\":\"ok\",\"pid\":%lu,\"overlay_active\":true,\"menu_open\":%s,"
                    "\"game_state\":%u,\"game_state_name\":\"%s\",\"auto_login_status\":\"%s\","
                    "\"navigation\":{\"active\":%s,\"target_name\":\"%s\",\"target_pos\":[%.2f,%.2f,%.2f],\"remaining_dist\":%.1f,\"waypoint_count\":%u,\"cur_waypoint\":%u},"
                    "\"player\":{\"valid\":%s,\"id\":%u,\"level\":%u,\"hp\":%u,\"max_hp\":%u,\"mp\":%u,\"max_mp\":%u,\"sp\":%u,\"max_sp\":%u,\"pos\":[%.2f,%.2f,%.2f]}}",
                    GetCurrentProcessId(),
                    WndProcHook::IsMenuOpen() ? "true" : "false",
                    static_cast<U8>(state),
                    AutoLoginManager::GetGameStateName(state),
                    AutoLoginManager::GetStatusMessage(),
                    NavigationManager::IsNavigating() ? "true" : "false",
                    NavigationManager::GetTargetName(),
                    navTarget.X, navTarget.Y, navTarget.Z,
                    NavigationManager::GetRemainingDistance(),
                    NavigationManager::GetWaypointCount(),
                    NavigationManager::GetCurrentWaypointIndex(),
                    player.Valid ? "true" : "false",
                    player.Id,
                    player.Level,
                    player.CurrentHp,
                    player.MaxHp,
                    player.CurrentMp,
                    player.MaxMp,
                    player.CurrentSp,
                    player.MaxSp,
                    player.Position.X, player.Position.Y, player.Position.Z
                );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"exception in get_status\"}");
            }
        }

        static void HandleGetPlayer(char* pResponse, size_t nMaxLen)
        {
            __try
            {
                const PlayerData& player = EntityManager::GetLocalPlayer();
                U64 localPlayerPtr = 0;
                if (Offsets.WorldManager)
                    Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &localPlayerPtr);

                Vector3 dir = { 0 };
                Vector3 dest = { 0 };
                U32 playerState = 0;

                if (localPlayerPtr)
                {
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerDirX, &dir.X);
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerDirY, &dir.Y);
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerDirZ, &dir.Z);
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerDestX, &dest.X);
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerDestY, &dest.Y);
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerDestZ, &dest.Z);
                    Memory::ReadSafe(localPlayerPtr + Offsets.PlayerState, &playerState);
                }

                Vector3 camEye = { 0 };
                if (Offsets.CameraEye)
                {
                    Memory::ReadSafe(Offsets.CameraEye, &camEye.X);
                    Memory::ReadSafe(Offsets.CameraEye + 4, &camEye.Y);
                    Memory::ReadSafe(Offsets.CameraEye + 8, &camEye.Z);
                }

                sprintf_s(pResponse, nMaxLen,
                    "{\"status\":\"ok\",\"player\":{"
                    "\"valid\":%s,\"id\":%u,\"level\":%u,\"hp\":%u,\"max_hp\":%u,\"hp_pct\":%.1f,"
                    "\"mp\":%u,\"max_mp\":%u,\"mp_pct\":%.1f,\"sp\":%u,\"max_sp\":%u,\"sp_pct\":%.1f,"
                    "\"pos\":[%.2f,%.2f,%.2f],\"dir\":[%.3f,%.3f,%.3f],\"dest\":[%.2f,%.2f,%.2f],"
                    "\"player_state\":%u,\"camera_eye\":[%.2f,%.2f,%.2f],\"ptr\":\"0x%llX\"}}",
                    player.Valid ? "true" : "false",
                    player.Id,
                    player.Level,
                    player.CurrentHp,
                    player.MaxHp,
                    player.GetHpPercentage() * 100.0f,
                    player.CurrentMp,
                    player.MaxMp,
                    player.GetMpPercentage() * 100.0f,
                    player.CurrentSp,
                    player.MaxSp,
                    player.GetSpPercentage() * 100.0f,
                    player.Position.X, player.Position.Y, player.Position.Z,
                    dir.X, dir.Y, dir.Z,
                    dest.X, dest.Y, dest.Z,
                    playerState,
                    camEye.X, camEye.Y, camEye.Z,
                    localPlayerPtr
                );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"exception in get_player\"}");
            }
        }

        static void HandleGetEntities(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            __try
            {
                char filterType[32] = { 0 };
                extract_json_string(pRequest, "type", filterType, sizeof(filterType));

                double dMaxDist = 300.0;
                extract_json_double(pRequest, "max_distance", dMaxDist);
                if (dMaxDist <= 0.0) dMaxDist = 300.0;

                double dLimit = 50.0;
                extract_json_double(pRequest, "limit", dLimit);
                if (dLimit <= 0.0 || dLimit > 200.0) dLimit = 50.0;
                size_t maxEntities = static_cast<size_t>(dLimit);

                const PlayerData& player = EntityManager::GetLocalPlayer();

                int written = sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"entities\":[");
                if (written <= 0) return;
                size_t offset = static_cast<size_t>(written);

                size_t totalFound = 0;
                size_t returnedCount = 0;

                // 1. Monsters
                if (filterType[0] == '\0' || strcmp(filterType, "all") == 0 || strcmp(filterType, "monster") == 0)
                {
                    const auto& monsters = EntityManager::GetNearbyMonsters();
                    for (U32 i = 0; i < monsters.GetCount() && returnedCount < maxEntities; ++i)
                    {
                        const auto& mob = monsters[i];
                        if (mob.Distance > dMaxDist) continue;
                        totalFound++;

                        char safeName[64] = { 0 };
                        sanitize_string(mob.Name, safeName, sizeof(safeName));

                        char itemBuf[256];
                        int itemLen = sprintf_s(itemBuf, sizeof(itemBuf),
                            "%s{\"type\":\"monster\",\"world_id\":%u,\"mob_id\":%u,\"name\":\"%s\",\"level\":%u,\"hp\":%u,\"max_hp\":%u,\"is_quest\":%s,\"pos\":[%.1f,%.1f,%.1f],\"distance\":%.1f}",
                            (returnedCount > 0) ? "," : "",
                            mob.WorldId, mob.MobId, safeName, mob.Level, mob.CurrentHp, mob.MaxHp,
                            mob.IsQuestTarget ? "true" : "false",
                            mob.Position.X, mob.Position.Y, mob.Position.Z, mob.Distance
                        );

                        if (itemLen > 0 && (offset + itemLen + 64) < nMaxLen)
                        {
                            memcpy(pResponse + offset, itemBuf, itemLen);
                            offset += itemLen;
                            returnedCount++;
                        }
                    }
                }

                // 2. Ground Loot Items
                if (filterType[0] == '\0' || strcmp(filterType, "all") == 0 || strcmp(filterType, "item") == 0)
                {
                    const auto& items = GroundItemManager::GetGroundItems();
                    for (U32 i = 0; i < items.GetCount() && returnedCount < maxEntities; ++i)
                    {
                        const auto& it = items[i];
                        if (it.Distance > dMaxDist) continue;
                        totalFound++;

                        char safeName[64] = { 0 };
                        sanitize_string(it.Name, safeName, sizeof(safeName));

                        char itemBuf[256];
                        int itemLen = sprintf_s(itemBuf, sizeof(itemBuf),
                            "%s{\"type\":\"item\",\"world_id\":%u,\"item_type\":%u,\"type_id\":%u,\"count\":%u,\"name\":\"%s\",\"pos\":[%.1f,%.1f,%.1f],\"distance\":%.1f}",
                            (returnedCount > 0) ? "," : "",
                            it.WorldId, it.Type, it.TypeId, it.Count, safeName,
                            it.Position.X, it.Position.Y, it.Position.Z, it.Distance
                        );

                        if (itemLen > 0 && (offset + itemLen + 64) < nMaxLen)
                        {
                            memcpy(pResponse + offset, itemBuf, itemLen);
                            offset += itemLen;
                            returnedCount++;
                        }
                    }
                }

                // 3. Quest Markers / NPCs
                if (filterType[0] == '\0' || strcmp(filterType, "all") == 0 || strcmp(filterType, "npc") == 0)
                {
                    const auto& markers = QuestManager::GetQuestMarkers();
                    for (U32 i = 0; i < markers.GetCount() && returnedCount < maxEntities; ++i)
                    {
                        const auto& mk = markers[i];
                        if (mk.Distance > dMaxDist) continue;
                        totalFound++;

                        char safeName[64] = { 0 };
                        sanitize_string(mk.NpcName, safeName, sizeof(safeName));

                        char itemBuf[256];
                        int itemLen = sprintf_s(itemBuf, sizeof(itemBuf),
                            "%s{\"type\":\"npc\",\"quest_id\":%u,\"is_turn_in\":%s,\"name\":\"%s\",\"pos\":[%.1f,%.1f,%.1f],\"distance\":%.1f}",
                            (returnedCount > 0) ? "," : "",
                            mk.QuestId, mk.IsTurnIn ? "true" : "false", safeName,
                            mk.Position.X, mk.Position.Y, mk.Position.Z, mk.Distance
                        );

                        if (itemLen > 0 && (offset + itemLen + 64) < nMaxLen)
                        {
                            memcpy(pResponse + offset, itemBuf, itemLen);
                            offset += itemLen;
                            returnedCount++;
                        }
                    }
                }

                sprintf_s(pResponse + offset, nMaxLen - offset, "],\"total_found\":%zu,\"returned_count\":%zu}", totalFound, returnedCount);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"exception in get_entities\"}");
            }
        }

        static void HandleGetNearestTarget(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            __try
            {
                char targetType[32] = "any";
                extract_json_string(pRequest, "type", targetType, sizeof(targetType));

                double dMaxDist = 300.0;
                extract_json_double(pRequest, "max_distance", dMaxDist);
                if (dMaxDist <= 0.0) dMaxDist = 300.0;

                const PlayerData& player = EntityManager::GetLocalPlayer();
                if (!player.Valid)
                {
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"player not valid\"}");
                    return;
                }

                float fMinDist = static_cast<float>(dMaxDist);
                char bestType[32] = { 0 };
                char bestName[64] = { 0 };
                Vector3 bestPos = { 0 };
                U32 bestId = 0;
                bool found = false;

                // Check monsters
                if (strcmp(targetType, "any") == 0 || strcmp(targetType, "monster") == 0 || strcmp(targetType, "quest") == 0 || strcmp(targetType, "quest_mob") == 0)
                {
                    bool questOnly = (strcmp(targetType, "quest") == 0 || strcmp(targetType, "quest_mob") == 0);
                    const auto& monsters = EntityManager::GetNearbyMonsters();
                    for (U32 i = 0; i < monsters.GetCount(); ++i)
                    {
                        const auto& mob = monsters[i];
                        if (questOnly && !mob.IsQuestTarget)
                            continue;
                        if (mob.CurrentHp > 0 && mob.Distance < fMinDist)
                        {
                            fMinDist = mob.Distance;
                            strcpy_s(bestType, "monster");
                            StringUtils::Copy(bestName, mob.Name, sizeof(bestName));
                            bestPos = mob.Position;
                            bestId = mob.MobId;
                            found = true;
                        }
                    }
                }

                // Check items
                if (strcmp(targetType, "any") == 0 || strcmp(targetType, "item") == 0)
                {
                    const auto& items = GroundItemManager::GetGroundItems();
                    for (U32 i = 0; i < items.GetCount(); ++i)
                    {
                        const auto& it = items[i];
                        if (it.Distance < fMinDist)
                        {
                            fMinDist = it.Distance;
                            strcpy_s(bestType, "item");
                            StringUtils::Copy(bestName, it.Name, sizeof(bestName));
                            bestPos = it.Position;
                            bestId = it.WorldId;
                            found = true;
                        }
                    }
                }

                // Check NPCs
                if (strcmp(targetType, "any") == 0 || strcmp(targetType, "npc") == 0)
                {
                    const auto& markers = QuestManager::GetQuestMarkers();
                    for (U32 i = 0; i < markers.GetCount(); ++i)
                    {
                        const auto& mk = markers[i];
                        if (mk.Distance < fMinDist)
                        {
                            fMinDist = mk.Distance;
                            strcpy_s(bestType, "npc");
                            StringUtils::Copy(bestName, mk.NpcName, sizeof(bestName));
                            bestPos = mk.Position;
                            bestId = mk.QuestId;
                            found = true;
                        }
                    }
                }

                if (!found)
                {
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"found\":false,\"message\":\"no entity found within distance\"}");
                    return;
                }

                char safeName[64] = { 0 };
                sanitize_string(bestName, safeName, sizeof(safeName));

                sprintf_s(pResponse, nMaxLen,
                    "{\"status\":\"ok\",\"found\":true,\"target\":{"
                    "\"type\":\"%s\",\"id\":%u,\"name\":\"%s\",\"distance\":%.1f,\"pos\":[%.2f,%.2f,%.2f]}}",
                    bestType, bestId, safeName, fMinDist, bestPos.X, bestPos.Y, bestPos.Z
                );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"exception in get_nearest_target\"}");
            }
        }

        static void HandleGetNavigation(char* pResponse, size_t nMaxLen)
        {
            __try
            {
                bool isNav = NavigationManager::IsNavigating();
                Vector3 targetPos = NavigationManager::GetTargetPosition();
                F32 remDist = NavigationManager::GetRemainingDistance();
                U32 wpCount = NavigationManager::GetWaypointCount();
                U32 curWp = NavigationManager::GetCurrentWaypointIndex();
                const Vector3* wps = NavigationManager::GetWaypoints();

                int written = sprintf_s(pResponse, nMaxLen,
                    "{\"status\":\"ok\",\"is_navigating\":%s,\"target_name\":\"%s\",\"target_pos\":[%.2f,%.2f,%.2f],"
                    "\"remaining_dist\":%.1f,\"waypoint_count\":%u,\"cur_waypoint_index\":%u,\"waypoints\":[",
                    isNav ? "true" : "false",
                    NavigationManager::GetTargetName(),
                    targetPos.X, targetPos.Y, targetPos.Z,
                    remDist, wpCount, curWp
                );
                if (written <= 0) return;
                size_t offset = static_cast<size_t>(written);

                for (U32 i = 0; i < wpCount && i < 32; ++i)
                {
                    char wpBuf[64];
                    int wpLen = sprintf_s(wpBuf, sizeof(wpBuf), "%s[%.2f,%.2f,%.2f]",
                        (i > 0) ? "," : "", wps[i].X, wps[i].Y, wps[i].Z);
                    if (wpLen > 0 && (offset + wpLen + 32) < nMaxLen)
                    {
                        memcpy(pResponse + offset, wpBuf, wpLen);
                        offset += wpLen;
                    }
                }

                sprintf_s(pResponse + offset, nMaxLen - offset, "]}");
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"exception in get_navigation\"}");
            }
        }

        static void HandleWalkTo(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            double x = 0, y = 0, z = 0;
            if (!extract_json_double(pRequest, "x", x) || !extract_json_double(pRequest, "z", z))
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing x or z parameter\"}");
                return;
            }

            extract_json_double(pRequest, "y", y);
            if (y == 0.0)
            {
                y = NavigationManager::GetGroundHeight(static_cast<float>(x), static_cast<float>(z));
            }

            char targetName[64] = "MCP_Target";
            extract_json_string(pRequest, "name", targetName, sizeof(targetName));

            double stopDist = 2.5;
            extract_json_double(pRequest, "stop_distance", stopDist);
            if (stopDist < 0.5) stopDist = 1.5;

            Vector3 target = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
            RunOnRenderThread([=]() {
                NavigationManager::WalkTo(target, targetName, static_cast<float>(stopDist));
            }, 1000);

            sprintf_s(pResponse, nMaxLen,
                "{\"status\":\"ok\",\"action\":\"walk_to\",\"target\":[%.2f,%.2f,%.2f],\"name\":\"%s\","
                "\"waypoint_count\":%u,\"distance\":%.1f}",
                target.X, target.Y, target.Z, targetName,
                NavigationManager::GetWaypointCount(),
                NavigationManager::GetRemainingDistance()
            );
        }

        static void HandleStopWalk(char* pResponse, size_t nMaxLen)
        {
            RunOnRenderThread([]() {
                NavigationManager::Stop();
            }, 500);
            sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"action\":\"stop_walk\"}");
        }

        static void HandleCheckCollision(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            double sx = 0, sy = 0, sz = 0;
            double ex = 0, ey = 0, ez = 0;
            double radius = 0.75;

            extract_json_double(pRequest, "start_x", sx);
            extract_json_double(pRequest, "start_y", sy);
            extract_json_double(pRequest, "start_z", sz);
            extract_json_double(pRequest, "end_x", ex);
            extract_json_double(pRequest, "end_y", ey);
            extract_json_double(pRequest, "end_z", ez);
            extract_json_double(pRequest, "radius", radius);

            Vector3 start = { static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(sz) };
            Vector3 end   = { static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez) };

            if (start.Y == 0.0f) start.Y = NavigationManager::GetGroundHeight(start.X, start.Z);
            if (end.Y == 0.0f)   end.Y   = NavigationManager::GetGroundHeight(end.X, end.Z);

            bool los = NavigationManager::CheckLineOfSight(start, end);
            bool clearance = NavigationManager::CheckWalkableClearance(start, end, static_cast<float>(radius));
            bool walkable = NavigationManager::IsSegmentWalkable(start, end);

            sprintf_s(pResponse, nMaxLen,
                "{\"status\":\"ok\",\"line_of_sight\":%s,\"walkable_clearance\":%s,\"segment_walkable\":%s,"
                "\"start\":[%.2f,%.2f,%.2f],\"end\":[%.2f,%.2f,%.2f]}",
                los ? "true" : "false",
                clearance ? "true" : "false",
                walkable ? "true" : "false",
                start.X, start.Y, start.Z,
                end.X, end.Y, end.Z
            );
        }

        static void HandleGetGroundHeight(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            double x = 0, z = 0;
            if (!extract_json_double(pRequest, "x", x) || !extract_json_double(pRequest, "z", z))
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing x or z parameter\"}");
                return;
            }

            float y = NavigationManager::GetGroundHeight(static_cast<float>(x), static_cast<float>(z));
            sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"x\":%.2f,\"z\":%.2f,\"ground_y\":%.2f}", x, z, y);
        }

        static void HandleDiagnosePath(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            __try
            {
                double gx = 0, gy = 0, gz = 0;
                if (!extract_json_double(pRequest, "goal_x", gx) || !extract_json_double(pRequest, "goal_z", gz))
                {
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing goal_x or goal_z parameter\"}");
                    return;
                }
                extract_json_double(pRequest, "goal_y", gy);

                Vector3 goal = { static_cast<float>(gx), static_cast<float>(gy), static_cast<float>(gz) };
                if (goal.Y == 0.0f) goal.Y = NavigationManager::GetGroundHeight(goal.X, goal.Z);

                Vector3 start = { 0 };
                double sx = 0, sy = 0, sz = 0;
                if (extract_json_double(pRequest, "start_x", sx) && extract_json_double(pRequest, "start_z", sz))
                {
                    extract_json_double(pRequest, "start_y", sy);
                    start = { static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(sz) };
                    if (start.Y == 0.0f) start.Y = NavigationManager::GetGroundHeight(start.X, start.Z);
                }
                else
                {
                    const PlayerData& player = EntityManager::GetLocalPlayer();
                    start = player.Position;
                }

                Vector3 waypoints[32];
                U32 wpCount = NavigationManager::BuildPath(start, goal, waypoints, 32);

                int written = sprintf_s(pResponse, nMaxLen,
                    "{\"status\":\"ok\",\"waypoint_count\":%u,\"start\":[%.2f,%.2f,%.2f],\"goal\":[%.2f,%.2f,%.2f],\"waypoints\":[",
                    wpCount, start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z
                );
                if (written <= 0) return;
                size_t offset = static_cast<size_t>(written);

                for (U32 i = 0; i < wpCount; ++i)
                {
                    char wpBuf[64];
                    int wpLen = sprintf_s(wpBuf, sizeof(wpBuf), "%s[%.2f,%.2f,%.2f]",
                        (i > 0) ? "," : "", waypoints[i].X, waypoints[i].Y, waypoints[i].Z);
                    if (wpLen > 0 && (offset + wpLen + 32) < nMaxLen)
                    {
                        memcpy(pResponse + offset, wpBuf, wpLen);
                        offset += wpLen;
                    }
                }

                sprintf_s(pResponse + offset, nMaxLen - offset, "]}");
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"exception in diagnose_path\"}");
            }
        }

        static void HandleReadMemory(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            char szAddr[64] = { 0 };
            if (!extract_json_string(pRequest, "address", szAddr, sizeof(szAddr)))
            {
                double dAddr = 0.0;
                if (!extract_json_double(pRequest, "address", dAddr))
                {
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing address parameter\"}");
                    return;
                }
                sprintf_s(szAddr, sizeof(szAddr), "0x%llX", static_cast<unsigned long long>(dAddr));
            }

            uintptr_t uAddr = parse_hex_or_dec(szAddr);
            if (!uAddr)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"invalid address 0\"}");
                return;
            }

            double dSize = 64.0;
            extract_json_double(pRequest, "size", dSize);
            if (dSize <= 0.0) dSize = 64.0;
            if (dSize > 2048.0) dSize = 2048.0;
            size_t reqSize = static_cast<size_t>(dSize);

            static uint8_t memBuf[2048];
            SIZE_T bytesRead = 0;
            BOOL bSuccess = ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(uAddr), memBuf, reqSize, &bytesRead);

            if (!bSuccess || bytesRead == 0)
            {
                DWORD err = GetLastError();
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"ReadProcessMemory failed\",\"error_code\":%lu,\"address\":\"0x%llX\"}", err, static_cast<unsigned long long>(uAddr));
                return;
            }

            int written = sprintf_s(pResponse, nMaxLen,
                "{\"status\":\"ok\",\"address\":\"0x%llX\",\"bytes_read\":%zu,\"hex\":\"",
                static_cast<unsigned long long>(uAddr), static_cast<size_t>(bytesRead));
            if (written <= 0) return;
            size_t offset = static_cast<size_t>(written);

            static const char hexChars[] = "0123456789ABCDEF";
            for (size_t i = 0; i < bytesRead && (offset + 4) < (nMaxLen - 512); ++i)
            {
                uint8_t b = memBuf[i];
                pResponse[offset++] = hexChars[(b >> 4) & 0xF];
                pResponse[offset++] = hexChars[b & 0xF];
                pResponse[offset++] = ' ';
            }
            if (offset > 0 && pResponse[offset - 1] == ' ') offset--;

            written = sprintf_s(pResponse + offset, nMaxLen - offset, "\",\"ascii\":\"");
            offset += static_cast<size_t>(written);

            for (size_t i = 0; i < bytesRead && (offset + 4) < (nMaxLen - 64); ++i)
            {
                char c = static_cast<char>(memBuf[i]);
                if (c >= 32 && c <= 126 && c != '\"' && c != '\\')
                    pResponse[offset++] = c;
                else
                    pResponse[offset++] = '.';
            }

            sprintf_s(pResponse + offset, nMaxLen - offset, "\"}");
        }

        static void HandleWriteMemory(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            char szAddr[64] = { 0 };
            if (!extract_json_string(pRequest, "address", szAddr, sizeof(szAddr)))
            {
                double dAddr = 0.0;
                if (!extract_json_double(pRequest, "address", dAddr))
                {
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing address parameter\"}");
                    return;
                }
                sprintf_s(szAddr, sizeof(szAddr), "0x%llX", static_cast<unsigned long long>(dAddr));
            }

            uintptr_t uAddr = parse_hex_or_dec(szAddr);
            if (!uAddr)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"invalid address 0\"}");
                return;
            }

            char hexData[4096] = { 0 };
            if (!extract_json_string(pRequest, "hex", hexData, sizeof(hexData)))
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing hex data parameter\"}");
                return;
            }

            uint8_t writeBuf[2048];
            size_t byteCount = 0;
            const char* p = hexData;

            auto hexNibble = [](char c) -> int
            {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };

            while (*p && byteCount < sizeof(writeBuf))
            {
                while (*p == ' ' || *p == '\t' || *p == ',') p++;
                if (!*p) break;
                if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
                int high = hexNibble(*p++);
                if (high == -1) break;
                if (!*p || *p == ' ' || *p == '\t' || *p == ',')
                {
                    writeBuf[byteCount++] = static_cast<uint8_t>(high);
                }
                else
                {
                    int low = hexNibble(*p++);
                    if (low == -1) break;
                    writeBuf[byteCount++] = static_cast<uint8_t>((high << 4) | low);
                }
            }

            if (byteCount == 0)
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"no valid hex bytes parsed\"}");
                return;
            }

            SIZE_T bytesWritten = 0;
            BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(uAddr), writeBuf, byteCount, &bytesWritten);
            if (!bSuccess || bytesWritten == 0)
            {
                DWORD err = GetLastError();
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"WriteProcessMemory failed\",\"error_code\":%lu,\"address\":\"0x%llX\"}", err, static_cast<unsigned long long>(uAddr));
                return;
            }

            sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"address\":\"0x%llX\",\"bytes_written\":%zu}",
                static_cast<unsigned long long>(uAddr), static_cast<size_t>(bytesWritten));
        }

        static void HandleWorldToScreen(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            double x = 0, y = 0, z = 0;
            if (!extract_json_double(pRequest, "x", x) ||
                !extract_json_double(pRequest, "y", y) ||
                !extract_json_double(pRequest, "z", z))
            {
                sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"missing x, y, z coordinates\"}");
                return;
            }

            float screenW = 1920.0f, screenH = 1080.0f;
            GetScreenDimensions(screenW, screenH);

            Vector3 worldPos = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
            Vector2 screenPos;
            bool inScreen = Camera::WorldToScreen(worldPos, screenPos, screenW, screenH);

            const PlayerData& player = EntityManager::GetLocalPlayer();
            float dist = player.Valid ? player.Position.DistanceTo(worldPos) : 0.0f;

            sprintf_s(pResponse, nMaxLen,
                "{\"status\":\"ok\",\"world_pos\":[%.2f,%.2f,%.2f],\"screen_pos\":[%.1f,%.1f],\"in_screen\":%s,"
                "\"screen_size\":{\"width\":%.0f,\"height\":%.0f},\"distance\":%.1f}",
                worldPos.X, worldPos.Y, worldPos.Z,
                screenPos.X, screenPos.Y,
                inScreen ? "true" : "false",
                screenW, screenH, dist
            );
        }

        static void HandleToggleMenu(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            bool hasOpen = false;
            bool openVal = false;
            if (extract_json_bool(pRequest, "open", openVal))
            {
                WndProcHook::SetMenuOpen(openVal);
            }
            else
            {
                WndProcHook::ToggleMenu();
            }

            sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"menu_open\":%s}",
                WndProcHook::IsMenuOpen() ? "true" : "false");
        }

        static void HandleGetConfig(char* pResponse, size_t nMaxLen)
        {
            sprintf_s(pResponse, nMaxLen,
                "{\"status\":\"ok\",\"config\":{\"snaplines_enabled\":%s,\"quest_waypoints_enabled\":%s,\"menu_open\":%s}}",
                Menu::IsSnaplinesEnabled() ? "true" : "false",
                Menu::IsQuestWaypointsEnabled() ? "true" : "false",
                WndProcHook::IsMenuOpen() ? "true" : "false"
            );
        }

        static void HandleSetConfig(const char* pRequest, char* pResponse, size_t nMaxLen)
        {
            char key[64] = { 0 };
            extract_json_string(pRequest, "key", key, sizeof(key));

            bool bVal = false;
            if (extract_json_bool(pRequest, "value", bVal))
            {
                if (strcmp(key, "snaplines_enabled") == 0)
                {
                    Menu::SetSnaplinesEnabled(bVal);
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"key\":\"%s\",\"value\":%s}", key, bVal ? "true" : "false");
                    return;
                }
                else if (strcmp(key, "quest_waypoints_enabled") == 0)
                {
                    Menu::SetQuestWaypointsEnabled(bVal);
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"key\":\"%s\",\"value\":%s}", key, bVal ? "true" : "false");
                    return;
                }
                else if (strcmp(key, "menu_open") == 0)
                {
                    WndProcHook::SetMenuOpen(bVal);
                    sprintf_s(pResponse, nMaxLen, "{\"status\":\"ok\",\"key\":\"%s\",\"value\":%s}", key, bVal ? "true" : "false");
                    return;
                }
            }

            sprintf_s(pResponse, nMaxLen, "{\"status\":\"error\",\"message\":\"unknown key or invalid value\"}");
        }

        static void HandleGetSkills(char* pResponseJson, size_t nMaxLen)
        {
            const auto& skills = SkillManager::GetSkills();
            size_t offset = sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"skills\":[");

            for (U32 i = 0; i < skills.GetCount(); ++i)
            {
                const auto& s = skills[i];
                char itemBuf[256];
                char safeName[64] = { 0 };
                sanitize_string(s.Name, safeName, sizeof(safeName));

                int itemLen = sprintf_s(itemBuf, sizeof(itemBuf),
                    "%s{\"id\":%u,\"level\":%u,\"name\":\"%s\",\"learned\":%s,\"passive\":%s,\"ready\":%s,\"slot\":%u,\"cooldown\":%.1f,\"duration\":%.1f}",
                    (i > 0) ? "," : "",
                    s.SkillId, s.Level, safeName,
                    s.IsLearned ? "true" : "false",
                    s.IsPassive ? "true" : "false",
                    s.IsReady ? "true" : "false",
                    s.LearnedSlot,
                    s.CooldownRemaining,
                    s.CooldownDuration
                );

                if (itemLen > 0 && (offset + itemLen + 32) < nMaxLen)
                {
                    memcpy(pResponseJson + offset, itemBuf, itemLen);
                    offset += itemLen;
                }
            }
            sprintf_s(pResponseJson + offset, nMaxLen - offset, "]}");
        }

        static void HandleGetInventory(char* pResponseJson, size_t nMaxLen)
        {
            InventoryManager::Update();
            const auto& items = InventoryManager::GetItems();
            size_t offset = sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"item_count\":%u,\"items\":[", items.GetCount());

            for (U32 i = 0; i < items.GetCount(); ++i)
            {
                const auto& item = items[i];
                char itemBuf[256];
                char safeName[64] = { 0 };
                sanitize_string(item.Name, safeName, sizeof(safeName));

                int itemLen = sprintf_s(itemBuf, sizeof(itemBuf),
                    "%s{\"bag\":%u,\"slot\":%u,\"global_slot\":%u,\"type\":%u,\"type_id\":%u,\"count\":%u,\"is_consumable\":%s,\"name\":\"%s\"}",
                    (i > 0) ? "," : "",
                    item.Bag, item.Slot, item.GlobalIndex,
                    item.Type, item.TypeId, item.Count,
                    item.IsConsumable ? "true" : "false",
                    safeName
                );

                if (itemLen > 0 && (offset + itemLen + 32) < nMaxLen)
                {
                    memcpy(pResponseJson + offset, itemBuf, itemLen);
                    offset += itemLen;
                }
            }
            sprintf_s(pResponseJson + offset, nMaxLen - offset, "]}");
        }

        static void HandleGetBuffs(char* pResponseJson, size_t nMaxLen)
        {
            BuffManager::Update();
            const auto& buffs = BuffManager::GetBuffs();
            size_t offset = sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"buff_count\":%u,\"buffs\":[", buffs.GetCount());

            for (U32 i = 0; i < buffs.GetCount(); ++i)
            {
                const auto& b = buffs[i];
                char itemBuf[256];
                char safeName[64] = { 0 };
                sanitize_string(b.Name, safeName, sizeof(safeName));

                int itemLen = sprintf_s(itemBuf, sizeof(itemBuf),
                    "%s{\"id\":%u,\"level\":%u,\"name\":\"%s\",\"duration_sec\":%u,\"total_sec\":%u,\"is_debuff\":%s}",
                    (i > 0) ? "," : "",
                    b.BuffId, b.Level, safeName,
                    b.DurationSeconds, b.TotalDurationSeconds,
                    b.IsDebuff ? "true" : "false"
                );

                if (itemLen > 0 && (offset + itemLen + 32) < nMaxLen)
                {
                    memcpy(pResponseJson + offset, itemBuf, itemLen);
                    offset += itemLen;
                }
            }
            sprintf_s(pResponseJson + offset, nMaxLen - offset, "]}");
        }

        static void HandleSelectTarget(const char* pRequestJson, char* pResponseJson, size_t nMaxLen)
        {
            double d = 0;
            uint32_t targetId = 0;
            if (extract_json_double(pRequestJson, "target_id", d))
                targetId = static_cast<uint32_t>(d);

            if (!Offsets.WorldManager)
            {
                sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"error\",\"message\":\"WorldManager not found\"}");
                return;
            }

            U64 LocalPlayerPtr = 0;
            if (!Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &LocalPlayerPtr) || !LocalPlayerPtr)
            {
                sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"error\",\"message\":\"LocalPlayer not found\"}");
                return;
            }

            *reinterpret_cast<U32*>(LocalPlayerPtr + Offsets.PlayerTargetWorldId) = targetId;
            U64 TargetTypeGlobal = (Offsets.PlayerId - 0xA06AAC) + 0xA0FF5C;
            if (TargetTypeGlobal)
                *reinterpret_cast<U32*>(TargetTypeGlobal) = (targetId != 0 && targetId != 0xFFFFFFFF) ? 2 : 0;

            sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"action\":\"select_target\",\"target_id\":%u}", targetId);
        }

        static void HandleCastSkill(const char* pRequestJson, char* pResponseJson, size_t nMaxLen)
        {
            double dSlot = 0;
            double dTarget = 0;
            double dType = 3;

            int slot = 0;
            if (extract_json_double(pRequestJson, "slot", dSlot))
                slot = static_cast<int>(dSlot);

            U8 targetType = 3;
            if (extract_json_double(pRequestJson, "target_type", dType))
                targetType = static_cast<U8>(dType);

            U32 explicitTarget = 0;
            if (extract_json_double(pRequestJson, "target_id", dTarget))
            {
                explicitTarget = static_cast<U32>(dTarget);
                if (explicitTarget != 0 && Offsets.WorldManager)
                {
                    U64 LocalPlayerPtr = 0;
                    if (Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &LocalPlayerPtr) && LocalPlayerPtr)
                    {
                        *reinterpret_cast<U32*>(LocalPlayerPtr + Offsets.PlayerTargetWorldId) = explicitTarget;
                        U64 TargetTypeGlobal = (Offsets.PlayerId - 0xA06AAC) + 0xA0FF5C;
                        if (TargetTypeGlobal)
                            *reinterpret_cast<U32*>(TargetTypeGlobal) = 2;
                    }
                }
            }

            bool ok = SkillManager::CastSkill(static_cast<U8>(slot), targetType, explicitTarget);
            U32 usedTarget = explicitTarget ? explicitTarget : SkillManager::GetSelectedTargetWorldId();

            sprintf_s(pResponseJson, nMaxLen,
                "{\"status\":\"%s\",\"action\":\"cast_skill\",\"slot\":%d,\"target_id\":%u}",
                ok ? "ok" : "error", slot, usedTarget);
        }

        static void HandleUseQuickslot(const char* pRequestJson, char* pResponseJson, size_t nMaxLen)
        {
            double dSlot = 0;
            int slot = 0;
            if (extract_json_double(pRequestJson, "slot", dSlot))
                slot = static_cast<int>(dSlot);

            HWND Hwnd = nullptr;
            if (Offsets.GameHwnd)
            {
                U64 HwndVal = 0;
                if (Memory::ReadSafe(Offsets.GameHwnd, &HwndVal) && HwndVal)
                    Hwnd = reinterpret_cast<HWND>(HwndVal);
            }
            if (!Hwnd) Hwnd = FindWindowA("SDL_app", nullptr);

            if (Hwnd)
            {
                WPARAM vk = (slot >= 0 && slot <= 8) ? ('1' + slot) : (slot == 9 ? '0' : 0);
                if (vk)
                {
                    UINT sc = MapVirtualKeyA((UINT)vk, 0);
                    PostMessageA(Hwnd, WM_KEYDOWN, vk, 1 | (sc << 16));
                    PostMessageA(Hwnd, WM_KEYUP, vk, 1 | (sc << 16) | (1 << 30) | (1 << 31));
                    sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"action\":\"use_quickslot\",\"slot\":%d,\"key\":\"%c\"}", slot, (char)vk);
                    return;
                }
            }
            sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"error\",\"message\":\"Failed to send quickslot key\"}");
        }

        void ProcessCommand(const char* pRequestJson, char* pResponseJson, size_t nMaxLen)
        {
            char cmd[64] = { 0 };
            if (!extract_json_string(pRequestJson, "cmd", cmd, sizeof(cmd)))
            {
                sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"error\",\"message\":\"missing cmd field\"}");
                return;
            }

            if (strcmp(cmd, "ping") == 0)
            {
                sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"pong\":true}");
            }
            else if (strcmp(cmd, "get_status") == 0)
            {
                HandleGetStatus(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_player") == 0)
            {
                HandleGetPlayer(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_entities") == 0)
            {
                HandleGetEntities(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_nearest_target") == 0)
            {
                HandleGetNearestTarget(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_skills") == 0)
            {
                HandleGetSkills(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_inventory") == 0)
            {
                HandleGetInventory(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_buffs") == 0)
            {
                HandleGetBuffs(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_navigation") == 0)
            {
                HandleGetNavigation(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "walk_to") == 0)
            {
                HandleWalkTo(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "stop_walk") == 0)
            {
                HandleStopWalk(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "select_target") == 0)
            {
                HandleSelectTarget(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "cast_skill") == 0)
            {
                HandleCastSkill(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "use_quickslot") == 0)
            {
                HandleUseQuickslot(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "auto_login") == 0)
            {
                const char* user = nullptr;
                const char* pass = nullptr;
                char userBuf[64] = { 0 };
                char passBuf[64] = { 0 };

                const char* pUser = strstr(pRequestJson, "\"username\":\"");
                if (pUser)
                {
                    pUser += 12;
                    const char* pEnd = strchr(pUser, '"');
                    if (pEnd)
                    {
                        size_t len = (size_t)(pEnd - pUser);
                        if (len < sizeof(userBuf))
                        {
                            memcpy(userBuf, pUser, len);
                            userBuf[len] = '\0';
                            user = userBuf;
                        }
                    }
                }

                const char* pPass = strstr(pRequestJson, "\"password\":\"");
                if (pPass)
                {
                    pPass += 12;
                    const char* pEnd = strchr(pPass, '"');
                    if (pEnd)
                    {
                        size_t len = (size_t)(pEnd - pPass);
                        if (len < sizeof(passBuf))
                        {
                            memcpy(passBuf, pPass, len);
                            passBuf[len] = '\0';
                            pass = passBuf;
                        }
                    }
                }

                bool started = AutoLoginManager::Start(user, pass);
                GameState state = AutoLoginManager::GetCurrentGameState();
                sprintf_s(pResponseJson, nMaxLen,
                    "{\"status\":\"ok\",\"action\":\"auto_login\",\"started\":%s,\"game_state\":%u,\"state_name\":\"%s\",\"message\":\"%s\"}",
                    started ? "true" : "false", static_cast<U8>(state), AutoLoginManager::GetGameStateName(state), AutoLoginManager::GetStatusMessage());
            }
            else if (strcmp(cmd, "unload") == 0 || strcmp(cmd, "unload_overlay") == 0 || strcmp(cmd, "eject") == 0)
            {
                sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"ok\",\"action\":\"unload\",\"message\":\"Unload requested, ejecting DLL...\"}");
                WndProcHook::RequestUnload();
            }
            else if (strcmp(cmd, "check_collision") == 0 || strcmp(cmd, "check_los") == 0)
            {
                HandleCheckCollision(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_ground_height") == 0)
            {
                HandleGetGroundHeight(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "build_path") == 0 || strcmp(cmd, "diagnose_path") == 0)
            {
                HandleDiagnosePath(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "read_memory") == 0)
            {
                HandleReadMemory(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "write_memory") == 0)
            {
                HandleWriteMemory(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "world_to_screen") == 0)
            {
                HandleWorldToScreen(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "toggle_menu") == 0)
            {
                HandleToggleMenu(pRequestJson, pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "get_config") == 0)
            {
                HandleGetConfig(pResponseJson, nMaxLen);
            }
            else if (strcmp(cmd, "set_config") == 0)
            {
                HandleSetConfig(pRequestJson, pResponseJson, nMaxLen);
            }
            else
            {
                sprintf_s(pResponseJson, nMaxLen, "{\"status\":\"error\",\"message\":\"unknown command: %s\"}", cmd);
            }
        }

        static char s_reqBuf[8192] = { 0 };
        static char s_respBuf[65536] = { 0 };

        static DWORD WINAPI MCPPipeThread(LPVOID)
        {
            Logger::Info("[MCP] MCPPipeThread started, listening on \\\\.\\pipe\\ShaiyaOverlay_MCP");

            HANDLE hConnectEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);

            while (g_bServerRunning)
            {
                SECURITY_DESCRIPTOR sd;
                InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
                SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);

                SECURITY_ATTRIBUTES sa;
                sa.nLength = sizeof(sa);
                sa.lpSecurityDescriptor = &sd;
                sa.bInheritHandle = FALSE;

                HANDLE hPipe = CreateNamedPipeA(
                    "\\\\.\\pipe\\ShaiyaOverlay_MCP",
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                    1,
                    65536,
                    8192,
                    0,
                    &sa
                );

                if (hPipe == INVALID_HANDLE_VALUE)
                {
                    DWORD err = GetLastError();
                    Logger::Error("[MCP] Failed to create named pipe, error: %lu", err);
                    Sleep(1000);
                    continue;
                }

                g_hPipe = hPipe;

                OVERLAPPED ov = { 0 };
                ov.hEvent = hConnectEvent;
                ResetEvent(hConnectEvent);

                BOOL connected = ConnectNamedPipe(hPipe, &ov);
                DWORD err = GetLastError();

                if (!connected && err == ERROR_IO_PENDING)
                {
                    HANDLE events[2] = { hConnectEvent, g_hShutdownEvent };
                    DWORD waitRes = WaitForMultipleObjects(2, events, FALSE, INFINITE);
                    if (waitRes == WAIT_OBJECT_0)
                    {
                        connected = TRUE;
                    }
                    else
                    {
                        CancelIo(hPipe);
                        CloseHandle(hPipe);
                        g_hPipe = INVALID_HANDLE_VALUE;
                        break;
                    }
                }
                else if (!connected && err == ERROR_PIPE_CONNECTED)
                {
                    connected = TRUE;
                }

                if (connected && g_bServerRunning)
                {
                    DWORD bytesRead = 0;
                    OVERLAPPED readOv = { 0 };
                    readOv.hEvent = hConnectEvent;
                    ResetEvent(hConnectEvent);

                    BOOL readOk = ReadFile(hPipe, s_reqBuf, sizeof(s_reqBuf) - 1, &bytesRead, &readOv);
                    if (!readOk && GetLastError() == ERROR_IO_PENDING)
                    {
                        HANDLE events[2] = { hConnectEvent, g_hShutdownEvent };
                        DWORD waitRes = WaitForMultipleObjects(2, events, FALSE, 3000);
                        if (waitRes == WAIT_OBJECT_0)
                        {
                            GetOverlappedResult(hPipe, &readOv, &bytesRead, FALSE);
                            readOk = TRUE;
                        }
                        else
                        {
                            CancelIo(hPipe);
                        }
                    }

                    if (readOk && bytesRead > 0)
                    {
                        s_reqBuf[bytesRead] = '\0';
                        ProcessCommand(s_reqBuf, s_respBuf, sizeof(s_respBuf));

                        DWORD bytesWritten = 0;
                        OVERLAPPED writeOv = { 0 };
                        writeOv.hEvent = hConnectEvent;
                        ResetEvent(hConnectEvent);

                        WriteFile(hPipe, s_respBuf, static_cast<DWORD>(strlen(s_respBuf)), &bytesWritten, &writeOv);
                        GetOverlappedResult(hPipe, &writeOv, &bytesWritten, TRUE);
                        FlushFileBuffers(hPipe);
                    }
                    DisconnectNamedPipe(hPipe);
                }

                CloseHandle(hPipe);
                g_hPipe = INVALID_HANDLE_VALUE;
            }

            if (hConnectEvent)
                CloseHandle(hConnectEvent);

            Logger::Info("[MCP] Pipe server thread stopped");
            return 0;
        }

        bool StartServer()
        {
            if (g_bServerRunning)
                return true;

            if (!g_hShutdownEvent)
                g_hShutdownEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
            else
                ResetEvent(g_hShutdownEvent);

            g_bServerRunning = true;
            DWORD dwThreadId = 0;
            g_hPipeThread = CreateThread(nullptr, 0, MCPPipeThread, nullptr, 0, &dwThreadId);
            if (!g_hPipeThread)
            {
                DWORD err = GetLastError();
                Logger::Error("[MCP] CreateThread for MCPPipeThread failed, error: %lu", err);
                g_bServerRunning = false;
                return false;
            }

            Logger::Info("[MCP] MCPPipeThread created successfully, ThreadId: %lu", dwThreadId);
            return true;
        }

        void StopServer()
        {
            if (!g_bServerRunning)
                return;

            g_bServerRunning = false;

            if (g_hShutdownEvent)
                SetEvent(g_hShutdownEvent);

            if (g_hTaskDoneEvent)
            {
                SetEvent(g_hTaskDoneEvent);
                CloseHandle(g_hTaskDoneEvent);
                g_hTaskDoneEvent = nullptr;
            }

            if (g_hPipeThread)
            {
                if (WaitForSingleObject(g_hPipeThread, 500) != WAIT_OBJECT_0)
                {
                    TerminateThread(g_hPipeThread, 0);
                }
                CloseHandle(g_hPipeThread);
                g_hPipeThread = nullptr;
            }

            if (g_hShutdownEvent)
            {
                CloseHandle(g_hShutdownEvent);
                g_hShutdownEvent = nullptr;
            }

            if (g_hPipe != INVALID_HANDLE_VALUE)
            {
                CloseHandle(g_hPipe);
                g_hPipe = INVALID_HANDLE_VALUE;
            }

            Logger::Info("[MCP] StopServer finished");
        }

        bool IsRunning()
        {
            return g_bServerRunning;
        }
    }
}

#endif // MCP_TOOL
