#include "NavigationManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include <cmath>

namespace ShaiyaOverlay
{
    bool NavigationManager::Active = false;
    Vector3 NavigationManager::TargetPos;
    F32 NavigationManager::ArrivalRadius = 2.5f;
    F32 NavigationManager::RemainingDistance = 0.0f;
    char NavigationManager::DestinationName[64] = { 0 };
    U32 NavigationManager::LastPacketTick = 0;

    Vector3 NavigationManager::Waypoints[128] = { 0 };
    U32 NavigationManager::WaypointCount = 0;
    U32 NavigationManager::CurrentWaypointIndex = 0;
    Vector3 NavigationManager::PathStartPos = { 0 };

    Vector3 NavigationManager::LastStuckCheckPos = { 0 };
    U32 NavigationManager::LastStuckCheckTick = 0;
    U32 NavigationManager::DetourLockTick = 0;

    std::atomic<bool> NavigationManager::WorkerInitialized{ false };
    std::atomic<bool> NavigationManager::ComputingPath{ false };
    std::atomic<bool> NavigationManager::PathReady{ false };
    std::atomic<bool> NavigationManager::CancelRequested{ false };

    Vector3 NavigationManager::RequestStart = { 0.0f, 0.0f, 0.0f };
    Vector3 NavigationManager::RequestGoal = { 0.0f, 0.0f, 0.0f };
    Vector3 NavigationManager::StagedWaypoints[128] = {};
    U32 NavigationManager::StagedWaypointCount = 0;

    HANDLE NavigationManager::hWorkerThread = nullptr;
    HANDLE NavigationManager::hWorkEvent = nullptr;
    HANDLE NavigationManager::hExitEvent = nullptr;
    CRITICAL_SECTION NavigationManager::PathLock = {};

    static bool KeyIsDown = false;

    typedef bool (__fastcall* tCheckLineOfSight)(U64 WorldMgr, const float* StartPos, const float* EndPos);
    typedef float (__fastcall* tGetGroundHeight)(U64 WorldMgr, float X, float Z);

    static HWND GetGameHwnd()
    {
        if (Offsets.GameHwnd)
        {
            U64 HwndVal = 0;
            if (Memory::ReadSafe(Offsets.GameHwnd, &HwndVal) && HwndVal)
                return reinterpret_cast<HWND>(HwndVal);
        }
        return FindWindowA("SDL_app", nullptr);
    }

    void NavigationManager::InitWorker()
    {
        if (WorkerInitialized.load())
            return;

        InitializeCriticalSection(&PathLock);
        hWorkEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
        hExitEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);

        hWorkerThread = CreateThread(
            nullptr,
            0,
            PathWorkerProc,
            nullptr,
            0,
            nullptr
        );

        WorkerInitialized.store(true);
        Logger::Info("Navigation: Async pathfinder worker thread initialized.");
    }

    void NavigationManager::Shutdown()
    {
        if (!WorkerInitialized.load())
            return;

        Stop();

        if (hExitEvent)
            SetEvent(hExitEvent);

        if (hWorkerThread)
        {
            WaitForSingleObject(hWorkerThread, 1500);
            CloseHandle(hWorkerThread);
            hWorkerThread = nullptr;
        }

        if (hWorkEvent)
        {
            CloseHandle(hWorkEvent);
            hWorkEvent = nullptr;
        }

        if (hExitEvent)
        {
            CloseHandle(hExitEvent);
            hExitEvent = nullptr;
        }

        DeleteCriticalSection(&PathLock);
        WorkerInitialized.store(false);
        Logger::Info("Navigation: Async pathfinder worker thread shut down.");
    }

    DWORD WINAPI NavigationManager::PathWorkerProc(LPVOID lpParam)
    {
        HANDLE waitHandles[2] = { hExitEvent, hWorkEvent };

        while (true)
        {
            DWORD res = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
            if (res == WAIT_OBJECT_0) // hExitEvent
            {
                break;
            }
            else if (res == WAIT_OBJECT_0 + 1) // hWorkEvent
            {
                Vector3 start, goal;
                EnterCriticalSection(&PathLock);
                start = RequestStart;
                goal = RequestGoal;
                LeaveCriticalSection(&PathLock);

                if (CancelRequested.load())
                {
                    ComputingPath.store(false);
                    continue;
                }

                Vector3 localWps[128];
                U32 count = BuildPath(start, goal, localWps, 128);

                if (CancelRequested.load())
                {
                    ComputingPath.store(false);
                    continue;
                }

                EnterCriticalSection(&PathLock);
                StagedWaypointCount = count;
                for (U32 i = 0; i < count; ++i)
                    StagedWaypoints[i] = localWps[i];
                LeaveCriticalSection(&PathLock);

                PathReady.store(true);
                ComputingPath.store(false);
            }
        }

        return 0;
    }

    void NavigationManager::RequestAsyncPath(const Vector3& Start, const Vector3& Goal)
    {
        InitWorker();

        CancelRequested.store(true); // Signal any active calculation to cancel

        EnterCriticalSection(&PathLock);
        RequestStart = Start;
        RequestGoal = Goal;
        CancelRequested.store(false);
        PathReady.store(false);
        ComputingPath.store(true);
        LeaveCriticalSection(&PathLock);

        SetEvent(hWorkEvent);
    }

    bool NavigationManager::CheckLineOfSightElevated(const Vector3& Start, const Vector3& End, F32 HeightOffset)
    {
        if (!Offsets.CheckLineOfSightAddr || !Offsets.WorldManager)
            return true;

        auto Fn = reinterpret_cast<tCheckLineOfSight>(Offsets.CheckLineOfSightAddr);
        float StartBuf[3] = { Start.X, Start.Y + HeightOffset, Start.Z };
        float EndBuf[3]   = { End.X,   End.Y + HeightOffset,   End.Z };

        return Fn(Offsets.WorldManager, StartBuf, EndBuf);
    }

    bool NavigationManager::CheckLineOfSight(const Vector3& Start, const Vector3& End)
    {
        // 1. Torso/Waist level (+0.95m): main body check
        if (!CheckLineOfSightElevated(Start, End, 0.95f))
            return false;

        // 2. Head level (+1.75m): doorframes, archways, low ceilings, awnings, tree branches
        if (!CheckLineOfSightElevated(Start, End, 1.75f))
            return false;

        // 3. Knee level (+0.35m): low obstacles, fences, curbs, rubble, gravestones
        if (!CheckLineOfSightElevated(Start, End, 0.35f))
            return false;

        return true;
    }

    F32 NavigationManager::GetGroundHeight(F32 X, F32 Z)
    {
        if (!Offsets.GetGroundHeightAddr || !Offsets.WorldManager)
            return 0.0f;

        auto Fn = reinterpret_cast<tGetGroundHeight>(Offsets.GetGroundHeightAddr);
        return Fn(Offsets.WorldManager, X, Z);
    }

    bool NavigationManager::CheckWalkableClearance(const Vector3& Start, const Vector3& End, F32 Radius)
    {
        // 1. Center capsule ray (all 3 vertical height levels)
        if (!CheckLineOfSight(Start, End))
            return false;

        if (Radius <= 0.1f)
            return true;

        F32 Dx = End.X - Start.X;
        F32 Dz = End.Z - Start.Z;
        F32 Len = Vector3::Sqrt(Dx * Dx + Dz * Dz);
        if (Len < 0.1f)
            return true;

        // Perpendicular horizontal unit vector for corridor clearance
        F32 InvLen = 1.0f / Len;
        F32 PerpX = -Dz * InvLen * Radius;
        F32 PerpZ =  Dx * InvLen * Radius;

        // 2. Left shoulder ray (check sideways clearance relative to center track)
        Vector3 LeftStart = { Start.X + PerpX, Start.Y, Start.Z + PerpZ };
        LeftStart.Y = GetGroundHeight(LeftStart.X, LeftStart.Z);
        Vector3 LeftEnd = { End.X + PerpX, End.Y, End.Z + PerpZ };
        LeftEnd.Y = GetGroundHeight(LeftEnd.X, LeftEnd.Z);

        if (LeftStart.Y == 0.0f || LeftEnd.Y == 0.0f)
            return false; // Ground is missing or inside solid building boundary

        if (fabsf(LeftStart.Y - Start.Y) > 0.85f || fabsf(LeftEnd.Y - End.Y) > 0.85f)
            return false; // Steep sideways drop or cliff wall at shoulder

        // Left shoulder Torso (+0.95m) and Head (+1.75m)
        if (!CheckLineOfSightElevated(LeftStart, LeftEnd, 0.95f) ||
            !CheckLineOfSightElevated(LeftStart, LeftEnd, 1.75f))
            return false;

        // 3. Right shoulder ray (check sideways clearance relative to center track)
        Vector3 RightStart = { Start.X - PerpX, Start.Y, Start.Z - PerpZ };
        RightStart.Y = GetGroundHeight(RightStart.X, RightStart.Z);
        Vector3 RightEnd = { End.X - PerpX, End.Y, End.Z - PerpZ };
        RightEnd.Y = GetGroundHeight(RightEnd.X, RightEnd.Z);

        if (RightStart.Y == 0.0f || RightEnd.Y == 0.0f)
            return false; // Ground is missing or inside solid building boundary

        if (fabsf(RightStart.Y - Start.Y) > 0.85f || fabsf(RightEnd.Y - End.Y) > 0.85f)
            return false; // Steep sideways drop or cliff wall at shoulder

        // Right shoulder Torso (+0.95m) and Head (+1.75m)
        if (!CheckLineOfSightElevated(RightStart, RightEnd, 0.95f) ||
            !CheckLineOfSightElevated(RightStart, RightEnd, 1.75f))
            return false;

        return true;
    }

    bool NavigationManager::IsSegmentWalkable(const Vector3& Start, const Vector3& End)
    {
        F32 Dx = End.X - Start.X;
        F32 Dz = End.Z - Start.Z;
        F32 Dist = Vector3::Sqrt(Dx * Dx + Dz * Dz);
        if (Dist < 0.1f)
            return true;

        // Overall slope check: reject slopes steeper than 0.70 (~35 degrees uphill) or 0.85 (downhill)
        F32 StartY = GetGroundHeight(Start.X, Start.Z);
        if (StartY == 0.0f) StartY = Start.Y;
        F32 EndY = GetGroundHeight(End.X, End.Z);
        if (EndY == 0.0f) EndY = End.Y;

        F32 deltaY = EndY - StartY;
        if (deltaY > 0.0f && (deltaY / Dist) > 0.70f)
            return false;
        if (deltaY < 0.0f && (-deltaY / Dist) > 0.85f)
            return false;

        // Width clearance corridor check (0.70m radius = 1.40m clear corridor with full height clearance)
        if (!CheckWalkableClearance(Start, End, 0.70f))
            return false;

        // Sample intermediate terrain elevation every 2.0 meters along the line
        int numSamples = static_cast<int>(Dist / 2.0f);
        if (numSamples < 2 && Dist > 2.0f) numSamples = 2;
        if (numSamples > 48) numSamples = 48;

        F32 prevY = Start.Y;
        F32 sampleStep = Dist / static_cast<F32>(numSamples + 1);

        for (int s = 1; s <= numSamples; ++s)
        {
            F32 t = static_cast<F32>(s) / static_cast<F32>(numSamples + 1);
            F32 sx = Start.X + Dx * t;
            F32 sz = Start.Z + Dz * t;
            F32 actualY = GetGroundHeight(sx, sz);
            if (actualY == 0.0f)
                return false;

            // Slope between consecutive samples (prevents crossing steep steps > 35 deg uphill, > 40 deg downhill)
            F32 stepSlope = (actualY - prevY) / sampleStep;
            if (stepSlope > 0.70f || stepSlope < -0.85f)
                return false;

            F32 expectedY = Start.Y + (End.Y - Start.Y) * t;

            // Reject terrain crests that bulge >2.0m above line
            if ((actualY - expectedY) > 2.0f)
                return false;

            // Reject ditches/drops that sink >3.0m below line
            if ((expectedY - actualY) > 3.0f)
                return false;

            prevY = actualY;
        }

        return true;
    }

    struct AStarCell
    {
        F32 gCost;
        F32 fCost;
        F32 groundY;
        I16 parentX;
        I16 parentZ;
        U8 state; // 0 = unvisited, 1 = open, 2 = closed, 3 = blocked
        U8 heightCached; // 0 = not computed, 1 = computed
    };

    struct HeapNode
    {
        I16 x, z;
        F32 fCost;
    };

    U32 NavigationManager::BuildPath(const Vector3& Start, const Vector3& Goal, Vector3* OutWaypoints, U32 MaxWaypoints)
    {
        if (!OutWaypoints || MaxWaypoints == 0)
            return 0;

        CancelRequested.store(false);

        // East Gate Corridor is ONLY needed when crossing between the Eastern Wilderness (X > 430, Y < 100)
        // and the Keolloseu town plateau (X <= 430, Y > 105) to navigate the sheer eastern cliff.
        const bool StartInEastWilderness = (Start.X > 430.0f && Start.Y < 100.0f);
        const bool GoalInEastWilderness  = (Goal.X  > 430.0f && Goal.Y  < 100.0f);
        const bool StartOnTownPlateau    = (Start.X <= 430.0f && Start.Y >= 104.0f);
        const bool GoalOnTownPlateau     = (Goal.X  <= 430.0f && Goal.Y  >= 104.0f);

        const Vector3 GateOuter = { 424.0f, 107.14f, 141.0f };
        const Vector3 GateInner = { 405.0f, 107.30f, 141.0f };

        // 1. Entering town plateau from the Eastern Wilderness
        if (StartInEastWilderness && GoalOnTownPlateau)
        {
            U32 wp1 = BuildPathAStar(Start, GateOuter, OutWaypoints, MaxWaypoints > 48 ? 48 : MaxWaypoints - 8);
            if (wp1 > 0 && wp1 < MaxWaypoints - 6)
            {
                OutWaypoints[wp1++] = GateInner;
                U32 remSlots = MaxWaypoints - wp1;
                U32 wp2 = BuildPathAStar(GateInner, Goal, OutWaypoints + wp1, remSlots);
                return wp1 + wp2;
            }
        }

        // 2. Exiting town plateau to the Eastern Wilderness
        if (StartOnTownPlateau && GoalInEastWilderness)
        {
            U32 wp1 = BuildPathAStar(Start, GateInner, OutWaypoints, MaxWaypoints > 16 ? 16 : MaxWaypoints - 8);
            if (wp1 > 0 && wp1 < MaxWaypoints - 6)
            {
                OutWaypoints[wp1++] = GateOuter;
                U32 remSlots = MaxWaypoints - wp1;
                U32 wp2 = BuildPathAStar(GateOuter, Goal, OutWaypoints + wp1, remSlots);
                return wp1 + wp2;
            }
        }

        return BuildPathAStar(Start, Goal, OutWaypoints, MaxWaypoints);
    }

    U32 NavigationManager::BuildPathAStar(const Vector3& Start, const Vector3& Goal, Vector3* OutWaypoints, U32 MaxWaypoints)
    {
        if (!OutWaypoints || MaxWaypoints == 0)
            return 0;

        Vector3 AdjustedGoal = Goal;
        F32 GoalGroundY = GetGroundHeight(AdjustedGoal.X, AdjustedGoal.Z);
        if (GoalGroundY != 0.0f)
            AdjustedGoal.Y = GoalGroundY;

        Vector3 AdjustedStart = Start;
        F32 StartGroundY = GetGroundHeight(AdjustedStart.X, AdjustedStart.Z);
        if (StartGroundY != 0.0f)
            AdjustedStart.Y = StartGroundY;

        F32 Dx = AdjustedGoal.X - AdjustedStart.X;
        F32 Dz = AdjustedGoal.Z - AdjustedStart.Z;
        F32 TotalDist = Vector3::Sqrt(Dx * Dx + Dz * Dz);
        if (TotalDist < 0.5f)
        {
            OutWaypoints[0] = AdjustedGoal;
            return 1;
        }

        // Direct line check: if the path to goal is already clear and flat, go directly
        if (IsSegmentWalkable(AdjustedStart, AdjustedGoal))
        {
            OutWaypoints[0] = AdjustedGoal;
            return 1;
        }

        // Grid setup: 256x256 grid covers full distance with adaptive detour margin
        const int GridDim = 256;
        F32 DetourMargin = (TotalDist > 60.0f) ? (TotalDist * 1.35f + 160.0f) : (TotalDist + 40.0f);
        F32 DesiredSpan = DetourMargin;
        F32 CellSize = DesiredSpan / static_cast<F32>(GridDim - 4);
        if (CellSize < 0.75f) CellSize = 0.75f;

        Vector3 Center = { (AdjustedStart.X + AdjustedGoal.X) * 0.5f,
                           (AdjustedStart.Y + AdjustedGoal.Y) * 0.5f,
                           (AdjustedStart.Z + AdjustedGoal.Z) * 0.5f };

        auto WorldToGridX = [&](F32 wx) -> int {
            int gx = GridDim / 2 + static_cast<int>(floorf((wx - Center.X) / CellSize + 0.5f));
            return gx < 0 ? 0 : (gx >= GridDim ? GridDim - 1 : gx);
        };
        auto WorldToGridZ = [&](F32 wz) -> int {
            int gz = GridDim / 2 + static_cast<int>(floorf((wz - Center.Z) / CellSize + 0.5f));
            return gz < 0 ? 0 : (gz >= GridDim ? GridDim - 1 : gz);
        };
        auto GridToWorldX = [&](int gx) -> F32 {
            return Center.X + (gx - GridDim / 2) * CellSize;
        };
        auto GridToWorldZ = [&](int gz) -> F32 {
            return Center.Z + (gz - GridDim / 2) * CellSize;
        };

        int startGX = WorldToGridX(AdjustedStart.X);
        int startGZ = WorldToGridZ(AdjustedStart.Z);
        int goalGX = WorldToGridX(AdjustedGoal.X);
        int goalGZ = WorldToGridZ(AdjustedGoal.Z);

        static AStarCell Grid[256][256];
        memset(Grid, 0, sizeof(Grid));

        auto GetOrComputeHeight = [&](int gx, int gz) -> F32 {
            if (Grid[gx][gz].heightCached)
                return Grid[gx][gz].groundY;
            F32 wx = GridToWorldX(gx);
            F32 wz = GridToWorldZ(gz);
            F32 y = GetGroundHeight(wx, wz);
            Grid[gx][gz].groundY = y;
            Grid[gx][gz].heightCached = 1;
            return y;
        };

        static HeapNode OpenHeap[65536];
        int HeapSize = 0;

        auto PushHeap = [&](I16 x, I16 z, F32 f) {
            if (HeapSize >= 65530) return;
            int i = HeapSize++;
            while (i > 0)
            {
                int p = (i - 1) / 2;
                if (OpenHeap[p].fCost <= f) break;
                OpenHeap[i] = OpenHeap[p];
                i = p;
            }
            OpenHeap[i] = { x, z, f };
        };

        auto PopHeap = [&]() -> HeapNode {
            HeapNode top = OpenHeap[0];
            HeapNode last = OpenHeap[--HeapSize];
            if (HeapSize > 0)
            {
                int i = 0;
                while (i * 2 + 1 < HeapSize)
                {
                    int l = i * 2 + 1;
                    int r = l + 1;
                    int s = (r < HeapSize && OpenHeap[r].fCost < OpenHeap[l].fCost) ? r : l;
                    if (last.fCost <= OpenHeap[s].fCost) break;
                    OpenHeap[i] = OpenHeap[s];
                    i = s;
                }
                OpenHeap[i] = last;
            }
            return top;
        };

        auto Heuristic = [&](int x1, int z1, int x2, int z2) -> F32 {
            F32 hx = static_cast<F32>(x1 - x2);
            F32 hz = static_cast<F32>(z1 - z2);
            return Vector3::Sqrt(hx * hx + hz * hz) * CellSize;
        };

        // Goal reached if within 1 cell of goalGX, goalGZ or arrival radius of actual world goal
        auto IsGoalNode = [&](int gx, int gz) -> bool {
            if (gx == goalGX && gz == goalGZ) return true;
            F32 wx = GridToWorldX(gx);
            F32 wz = GridToWorldZ(gz);
            F32 ddx = wx - AdjustedGoal.X;
            F32 ddz = wz - AdjustedGoal.Z;
            return (ddx * ddx + ddz * ddz) <= (ArrivalRadius * ArrivalRadius);
        };

        Grid[startGX][startGZ].gCost = 0.0f;
        Grid[startGX][startGZ].fCost = Heuristic(startGX, startGZ, goalGX, goalGZ);
        Grid[startGX][startGZ].groundY = AdjustedStart.Y;
        Grid[startGX][startGZ].heightCached = 1;
        Grid[startGX][startGZ].parentX = -1;
        Grid[startGX][startGZ].parentZ = -1;
        Grid[startGX][startGZ].state = 1;
        PushHeap(static_cast<I16>(startGX), static_cast<I16>(startGZ), Grid[startGX][startGZ].fCost);

        int bestX = startGX;
        int bestZ = startGZ;
        F32 bestH = 1e9f;
        bool found = false;

        const int dx[8] = { 0, 1, 0, -1, 1, -1, 1, -1 };
        const int dz[8] = { 1, 0, -1, 0, 1, 1, -1, -1 };
        const F32 costs[8] = {
            CellSize, CellSize, CellSize, CellSize,
            1.414f * CellSize, 1.414f * CellSize, 1.414f * CellSize, 1.414f * CellSize
        };

        int iterations = 0;
        const int maxIterations = 20000;

        while (HeapSize > 0 && iterations < maxIterations)
        {
            if (CancelRequested.load())
                return 0;

            HeapNode cur = PopHeap();
            int cx = cur.x;
            int cz = cur.z;

            if (Grid[cx][cz].state == 2) continue;
            Grid[cx][cz].state = 2;
            ++iterations;

            F32 h = Heuristic(cx, cz, goalGX, goalGZ);
            if (h < bestH)
            {
                bestH = h;
                bestX = cx;
                bestZ = cz;
            }

            F32 curWX = (cx == startGX && cz == startGZ) ? AdjustedStart.X : GridToWorldX(cx);
            F32 curWZ = (cx == startGX && cz == startGZ) ? AdjustedStart.Z : GridToWorldZ(cz);
            F32 curWY = (cx == startGX && cz == startGZ) ? AdjustedStart.Y : GetOrComputeHeight(cx, cz);
            Vector3 curPos = { curWX, curWY, curWZ };

            if ((cx != startGX || cz != startGZ) && (IsGoalNode(cx, cz) || (h < 12.0f && (iterations % 8 == 0) && CheckLineOfSight(curPos, AdjustedGoal))))
            {
                bestX = cx;
                bestZ = cz;
                found = true;
                break;
            }

            for (int i = 0; i < 8; ++i)
            {
                int nx = cx + dx[i];
                int nz = cz + dz[i];

                if (nx < 0 || nx >= GridDim || nz < 0 || nz >= GridDim) continue;
                if (Grid[nx][nz].state == 2 || Grid[nx][nz].state == 3) continue;

                F32 nWY = GetOrComputeHeight(nx, nz);
                if (nWY == 0.0f)
                {
                    Grid[nx][nz].state = 3;
                    continue;
                }

                // Slopes: uphill <= 0.70, downhill <= 0.85
                F32 stepDist = costs[i];
                F32 deltaY = nWY - curWY;

                if (deltaY > 0.0f)
                {
                    if ((deltaY / stepDist) > 0.70f)
                    {
                        continue;
                    }
                }
                else
                {
                    if ((-deltaY / stepDist) > 0.85f)
                    {
                        continue;
                    }
                }

                // Fast torso-level raycast check between adjacent grid cells
                F32 nWX = GridToWorldX(nx);
                F32 nWZ = GridToWorldZ(nz);
                Vector3 nPos = { nWX, nWY, nWZ };
                if (!CheckLineOfSightElevated(curPos, nPos, 0.95f))
                {
                    continue;
                }

                // Moderate uphill penalty so A* prefers gentler slopes but easily climbs natural hills
                F32 slopeGrad = deltaY / stepDist;
                F32 slopePenalty = (slopeGrad > 0.0f) ? (slopeGrad * slopeGrad * 4.0f * stepDist) : (fabsf(slopeGrad) * 1.0f * stepDist);
                F32 stepCost = stepDist + slopePenalty;

                F32 newG = Grid[cx][cz].gCost + stepCost;
                if (Grid[nx][nz].state == 0 || newG < Grid[nx][nz].gCost)
                {
                    Grid[nx][nz].gCost = newG;
                    Grid[nx][nz].fCost = newG + Heuristic(nx, nz, goalGX, goalGZ);
                    Grid[nx][nz].parentX = static_cast<I16>(cx);
                    Grid[nx][nz].parentZ = static_cast<I16>(cz);
                    Grid[nx][nz].state = 1;
                    PushHeap(static_cast<I16>(nx), static_cast<I16>(nz), Grid[nx][nz].fCost);
                }
            }
        }

        // Traceback path
        int tx = bestX;
        int tz = bestZ;

        Vector3 RawPath[512];
        int RawCount = 0;

        F32 bestWX = GridToWorldX(bestX);
        F32 bestWZ = GridToWorldZ(bestZ);
        F32 bestWY = GetOrComputeHeight(bestX, bestZ);
        Vector3 bestPos = { bestWX, bestWY, bestWZ };

        if (found || IsGoalNode(bestX, bestZ) || CheckLineOfSight(bestPos, AdjustedGoal))
        {
            RawPath[RawCount++] = AdjustedGoal;
        }

        while (tx != -1 && tz != -1 && RawCount < 500)
        {
            F32 wx = GridToWorldX(tx);
            F32 wz = GridToWorldZ(tz);
            F32 wy = GetOrComputeHeight(tx, tz);
            RawPath[RawCount++] = { wx, wy, wz };

            int px = Grid[tx][tz].parentX;
            int pz = Grid[tx][tz].parentZ;
            tx = px;
            tz = pz;
        }

        if (RawCount <= 0)
        {
            OutWaypoints[0] = AdjustedGoal;
            return 1;
        }

        // Reverse raw path so it starts at AdjustedStart and ends at AdjustedGoal
        for (int i = 0; i < RawCount / 2; ++i)
        {
            Vector3 tmp = RawPath[i];
            RawPath[i] = RawPath[RawCount - 1 - i];
            RawPath[RawCount - 1 - i] = tmp;
        }

        // Path smoothing (string pulling) with clearance and slope validation
        Vector3 Smoothed[128];
        U32 SmoothCount = 0;

        Smoothed[SmoothCount++] = AdjustedStart;

        int curIdx = 0;
        while (curIdx < RawCount - 1 && SmoothCount < MaxWaypoints - 1)
        {
            int farthest = curIdx + 1;
            int maxLookahead = curIdx + 20;
            if (maxLookahead >= RawCount) maxLookahead = RawCount - 1;

            for (int test = maxLookahead; test > curIdx + 1; --test)
            {
                if (IsSegmentWalkable(Smoothed[SmoothCount - 1], RawPath[test]))
                {
                    farthest = test;
                    break;
                }
            }
            Smoothed[SmoothCount++] = RawPath[farthest];
            curIdx = farthest;
        }

        // Ensure AdjustedGoal is ALWAYS connected as the destination if goal reached
        if (found || IsGoalNode(bestX, bestZ) || CheckLineOfSight(bestPos, AdjustedGoal) || CheckLineOfSight(Smoothed[SmoothCount - 1], AdjustedGoal))
        {
            if (SmoothCount >= MaxWaypoints)
                SmoothCount = MaxWaypoints - 1;

            F32 FinalDistToGoal = Smoothed[SmoothCount - 1].DistanceTo(AdjustedGoal);
            if (FinalDistToGoal <= 2.5f || CheckLineOfSight(Smoothed[SmoothCount - 1], AdjustedGoal))
            {
                Smoothed[SmoothCount - 1] = AdjustedGoal;
            }
            else
            {
                Smoothed[SmoothCount++] = AdjustedGoal;
            }
        }

        // Copy out (skipping index 0 which is Start position)
        U32 FinalCount = 0;
        for (U32 i = 1; i < SmoothCount && FinalCount < MaxWaypoints; ++i)
        {
            F32 gy = GetGroundHeight(Smoothed[i].X, Smoothed[i].Z);
            if (gy != 0.0f)
                Smoothed[i].Y = gy;
            OutWaypoints[FinalCount++] = Smoothed[i];
        }

        if (FinalCount == 0)
        {
            OutWaypoints[0] = AdjustedGoal;
            FinalCount = 1;
        }

        return FinalCount;
    }

    void NavigationManager::WalkTo(const Vector3& Target, const char* TargetName, F32 StopDistance)
    {
        InitWorker();
        CancelRequested.store(false);

        TargetPos = Target;
        F32 GroundY = GetGroundHeight(TargetPos.X, TargetPos.Z);
        if (GroundY != 0.0f)
            TargetPos.Y = GroundY;

        ArrivalRadius = StopDistance > 1.0f ? StopDistance : 1.5f;
        Active = true;
        LastPacketTick = 0;

        if (TargetName && TargetName[0] != '\0')
        {
            StringUtils::Copy(DestinationName, TargetName, sizeof(DestinationName));
            StringUtils::NormalizeAccents(DestinationName, sizeof(DestinationName), false);
        }
        else
            StringUtils::Format(DestinationName, sizeof(DestinationName), "Pos (%.0f, %.0f)", TargetPos.X, TargetPos.Z);

        Logger::Info("Navigation: Auto-walk started to %s at (%.1f, %.1f, %.1f)",
            DestinationName, TargetPos.X, TargetPos.Y, TargetPos.Z);

        // Read player start position
        Vector3 StartPos = TargetPos;
        U64 LocalPlayerPtr = 0;
        if (Offsets.WorldManager && Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &LocalPlayerPtr) && LocalPlayerPtr)
        {
            Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosX, &StartPos.X);
            Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosY, &StartPos.Y);
            Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosZ, &StartPos.Z);
        }

        PathStartPos = StartPos;
        LastStuckCheckPos = StartPos;
        LastStuckCheckTick = GetTickCount();

        // Fast path: if direct line has direct line-of-sight and walkable slope, start immediately with 0 latency
        if (CheckLineOfSight(StartPos, TargetPos) && IsSegmentWalkable(StartPos, TargetPos))
        {
            WaypointCount = 1;
            Waypoints[0] = TargetPos;
            CurrentWaypointIndex = 0;
            ComputingPath.store(false);
            PathReady.store(false);

            Logger::Info("Navigation: Direct clear path with 1 waypoint.");

            HWND Hwnd = GetGameHwnd();
            if (Hwnd)
            {
                PostMessageA(Hwnd, WM_KEYDOWN, 'W', 1 | (0x11 << 16));
                KeyIsDown = true;
            }

            if (Offsets.KeyBuffer)
            {
                UINT scanCode = MapVirtualKeyA('W', MAPVK_VK_TO_VSC);
                *reinterpret_cast<U8*>(Offsets.KeyBuffer + scanCode) = 0x80;
                KeyIsDown = true;
            }
            return;
        }

        // Direct path blocked by obstacle: calculate path asynchronously in background thread!
        RequestAsyncPath(StartPos, TargetPos);
    }

    void NavigationManager::Stop()
    {
        CancelRequested.store(true);
        ComputingPath.store(false);
        PathReady.store(false);

        if (!Active && !KeyIsDown)
            return;

        Active = false;
        RemainingDistance = 0.0f;
        WaypointCount = 0;
        CurrentWaypointIndex = 0;

        HWND Hwnd = GetGameHwnd();
        if (Hwnd && KeyIsDown)
        {
            PostMessageA(Hwnd, WM_KEYUP, 'W', 1 | (0x11 << 16) | (1 << 30) | (1 << 31));
        }

        if (Offsets.KeyBuffer)
        {
            UINT scanCode = MapVirtualKeyA('W', MAPVK_VK_TO_VSC);
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + scanCode) = 0x00;
        }

        KeyIsDown = false;

        Logger::Info("Navigation: Auto-walk stopped.");
    }

    void NavigationManager::Update()
    {
        if (!Active)
            return;

        if (!Offsets.WorldManager)
        {
            Stop();
            return;
        }

        U64 LocalPlayerPtr = 0;
        if (!Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &LocalPlayerPtr) || !LocalPlayerPtr)
        {
            Stop();
            return;
        }

        Vector3 CurPos;
        if (!Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosX, &CurPos.X) ||
            !Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosY, &CurPos.Y) ||
            !Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosZ, &CurPos.Z))
        {
            Stop();
            return;
        }

        HWND Hwnd = GetGameHwnd();
        U32 Now = GetTickCount();

        // Cancel on manual user movement or escape ONLY when game is foreground
        if (Hwnd && GetForegroundWindow() == Hwnd)
        {
            if ((GetAsyncKeyState('W') & 0x8000) ||
                (GetAsyncKeyState('A') & 0x8000) ||
                (GetAsyncKeyState('S') & 0x8000) ||
                (GetAsyncKeyState('D') & 0x8000) ||
                (GetAsyncKeyState(VK_ESCAPE) & 0x8000))
            {
                Logger::Info("Navigation: Manual cancel detected.");
                Stop();
                return;
            }
        }

        // If path is currently being computed asynchronously:
        if (ComputingPath.load())
        {
            // Keep game rendering smoothly without blocking
            return;
        }

        // If async path calculation just finished:
        if (PathReady.load())
        {
            PathReady.store(false);

            EnterCriticalSection(&PathLock);
            WaypointCount = StagedWaypointCount;
            for (U32 i = 0; i < WaypointCount; ++i)
                Waypoints[i] = StagedWaypoints[i];
            LeaveCriticalSection(&PathLock);

            CurrentWaypointIndex = 0;
            PathStartPos = CurPos;
            LastStuckCheckPos = CurPos;
            LastStuckCheckTick = Now;
            DetourLockTick = Now;
            Logger::Info("Navigation: Async path ready with %u waypoints.", WaypointCount);

            if (WaypointCount == 0)
            {
                Logger::Info("Navigation: No path found to target.");
                Stop();
                return;
            }

            if (Hwnd)
            {
                PostMessageA(Hwnd, WM_KEYDOWN, 'W', 1 | (0x11 << 16));
                KeyIsDown = true;
            }

            if (Offsets.KeyBuffer)
            {
                UINT scanCode = MapVirtualKeyA('W', MAPVK_VK_TO_VSC);
                *reinterpret_cast<U8*>(Offsets.KeyBuffer + scanCode) = 0x80;
                KeyIsDown = true;
            }
        }

        // Remaining distance to ultimate goal
        F32 TotalDx = TargetPos.X - CurPos.X;
        F32 TotalDz = TargetPos.Z - CurPos.Z;
        RemainingDistance = Vector3::Sqrt(TotalDx * TotalDx + TotalDz * TotalDz);

        // Check if arrived at final destination
        if (RemainingDistance <= ArrivalRadius)
        {
            Logger::Info("Navigation: Arrived at %s! Distance: %.1fm", DestinationName, RemainingDistance);
            Stop();
            return;
        }

        // Dynamic stuck/sliding recovery (only if player is completely immobile for 2.5s)
        Now = GetTickCount();
        if (Now - LastStuckCheckTick > 2500)
        {
            F32 MovedDx = CurPos.X - LastStuckCheckPos.X;
            F32 MovedDz = CurPos.Z - LastStuckCheckPos.Z;
            F32 MovedDist = Vector3::Sqrt(MovedDx * MovedDx + MovedDz * MovedDz);

            if (MovedDist < 0.20f)
            {
                Logger::Info("Navigation: Stuck/sliding detected (moved %.2fm in 2.5s, rem=%.1fm).", MovedDist, RemainingDistance);

                // If close to current waypoint, skip to next waypoint first
                if (CurrentWaypointIndex < WaypointCount - 1)
                {
                    Vector3 CurWp = Waypoints[CurrentWaypointIndex];
                    F32 cdx = CurWp.X - CurPos.X;
                    F32 cdz = CurWp.Z - CurPos.Z;
                    F32 distWp = Vector3::Sqrt(cdx * cdx + cdz * cdz);
                    if (distWp <= 3.5f)
                    {
                        Logger::Info("Navigation: Skipping close waypoint %u/%u (dist=%.1fm) to unstick.", CurrentWaypointIndex, WaypointCount, distWp);
                        CurrentWaypointIndex++;
                        LastStuckCheckPos = CurPos;
                        LastStuckCheckTick = Now;
                        DetourLockTick = Now;
                        return;
                    }
                }

                Logger::Info("Navigation: Re-routing asynchronously from stuck position...");

                // Release key briefly to stop momentum/sliding
                if (Offsets.KeyBuffer)
                {
                    UINT scanCode = MapVirtualKeyA('W', MAPVK_VK_TO_VSC);
                    *reinterpret_cast<U8*>(Offsets.KeyBuffer + scanCode) = 0x00;
                }
                if (Hwnd && KeyIsDown)
                {
                    PostMessageA(Hwnd, WM_KEYUP, 'W', 1 | (0x11 << 16) | (1 << 30) | (1 << 31));
                    KeyIsDown = false;
                }

                PathStartPos = CurPos;
                DetourLockTick = Now;
                LastStuckCheckPos = CurPos;
                LastStuckCheckTick = Now + 1500;

                RequestAsyncPath(CurPos, TargetPos);
                return;
            }

            LastStuckCheckPos = CurPos;
            LastStuckCheckTick = Now;
        }

        // Ensure valid waypoint target: stop when last waypoint reached (no re-pathfind)
        if (WaypointCount == 0 || CurrentWaypointIndex >= WaypointCount)
        {
            Stop();
            return;
        }

        Vector3 CurrentTarget = Waypoints[CurrentWaypointIndex];
        bool isFinalWaypoint = (CurrentWaypointIndex == WaypointCount - 1);

        F32 Dx = CurrentTarget.X - CurPos.X;
        F32 Dz = CurrentTarget.Z - CurPos.Z;
        F32 DistToWaypoint = Vector3::Sqrt(Dx * Dx + Dz * Dz);

        if (isFinalWaypoint)
        {
            if (DistToWaypoint <= ArrivalRadius)
            {
                Logger::Info("Navigation: Final waypoint reached. Stopping.");
                Stop();
                return;
            }
        }
        else
        {
            // Advance to next waypoint if physically passed, very close, or within corridor with clear line of sight
            Vector3 PrevPt = (CurrentWaypointIndex == 0) ? PathStartPos : Waypoints[CurrentWaypointIndex - 1];
            F32 SegX = CurrentTarget.X - PrevPt.X;
            F32 SegZ = CurrentTarget.Z - PrevPt.Z;
            F32 DotPast = (CurPos.X - CurrentTarget.X) * SegX + (CurPos.Z - CurrentTarget.Z) * SegZ;

            Vector3 NextWp = Waypoints[CurrentWaypointIndex + 1];
            bool hasLosToNext = CheckLineOfSight(CurPos, NextWp);

            // During detour lock (first 3.0s after stuck recovery), do not cut corners early!
            bool isDetourLocked = (Now - DetourLockTick < 3000);

            bool canAdvance = false;
            if (!isDetourLocked && hasLosToNext)
            {
                // Next waypoint is unobstructed: can smoothly round or cut the corner
                if (DistToWaypoint <= 2.2f || (DotPast > 0.0f && DistToWaypoint <= 4.0f))
                {
                    canAdvance = true;
                }
            }
            else
            {
                // Blocked or detour locked: advance if past plane OR within 1.2m of waypoint
                if (DotPast > 0.0f || DistToWaypoint <= 1.2f)
                {
                    canAdvance = true;
                }
            }

            if (canAdvance)
            {
                CurrentWaypointIndex++;
                if (CurrentWaypointIndex >= WaypointCount)
                {
                    Stop();
                    return;
                }
                CurrentTarget = Waypoints[CurrentWaypointIndex];
                Dx = CurrentTarget.X - CurPos.X;
                Dz = CurrentTarget.Z - CurPos.Z;
                DistToWaypoint = Vector3::Sqrt(Dx * Dx + Dz * Dz);
            }
        }

        if (DistToWaypoint < 0.01f)
            DistToWaypoint = 0.01f;

        F32 InvLen = 1.0f / DistToWaypoint;
        F32 DirX = Dx * InvLen;
        F32 DirZ = Dz * InvLen;

        // Orient camera behind the character looking towards current waypoint
        if (Offsets.CameraEye)
        {
            F32 LookX = 0.0f;
            F32 LookY = 0.0f;
            F32 LookZ = 0.0f;
            Memory::ReadSafe(Offsets.CameraEye + 0x0C, &LookX);
            Memory::ReadSafe(Offsets.CameraEye + 0x10, &LookY);
            Memory::ReadSafe(Offsets.CameraEye + 0x14, &LookZ);

            F32 CamDist = 5.0f;
            F32 EyeX = LookX - CamDist * DirX;
            F32 EyeZ = LookZ - CamDist * DirZ;

            // Primary CameraEye
            *reinterpret_cast<F32*>(Offsets.CameraEye) = EyeX;
            *reinterpret_cast<F32*>(Offsets.CameraEye + 0x08) = EyeZ;

            // Secondary CameraEye (immediately follows ProjMatrix 4x4)
            if (Offsets.ProjMatrix)
            {
                U64 CamEye2 = Offsets.ProjMatrix + Offsets.CameraSecondaryEyeOffset;
                *reinterpret_cast<F32*>(CamEye2) = EyeX;
                *reinterpret_cast<F32*>(CamEye2 + 0x08) = EyeZ;
            }
        }

        // Set direction vectors in LocalPlayer
        *reinterpret_cast<F32*>(LocalPlayerPtr + Offsets.PlayerDirX) = DirX;
        *reinterpret_cast<F32*>(LocalPlayerPtr + Offsets.PlayerDirY) = 0.0f;
        *reinterpret_cast<F32*>(LocalPlayerPtr + Offsets.PlayerDirZ) = DirZ;

        *reinterpret_cast<F32*>(LocalPlayerPtr + Offsets.PlayerMoveDirX) = DirX;
        *reinterpret_cast<F32*>(LocalPlayerPtr + Offsets.PlayerMoveDirY) = 0.0f;
        *reinterpret_cast<F32*>(LocalPlayerPtr + Offsets.PlayerMoveDirZ) = DirZ;

        // Ensure keydown is maintained directly in memory buffer and via PostMessage fallback
        if (Offsets.KeyBuffer)
        {
            UINT scanCode = MapVirtualKeyA('W', MAPVK_VK_TO_VSC);
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + scanCode) = 0x80;
        }

        if (Hwnd && !KeyIsDown)
        {
            PostMessageA(Hwnd, WM_KEYDOWN, 'W', 1 | (0x11 << 16));
            KeyIsDown = true;
        }
    }
}
