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

    Vector3 NavigationManager::Waypoints[64] = { 0 };
    U32 NavigationManager::WaypointCount = 0;
    U32 NavigationManager::CurrentWaypointIndex = 0;
    Vector3 NavigationManager::PathStartPos = { 0 };

    Vector3 NavigationManager::LastStuckCheckPos = { 0 };
    U32 NavigationManager::LastStuckCheckTick = 0;
    U32 NavigationManager::DetourLockTick = 0;

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

    bool NavigationManager::CheckLineOfSight(const Vector3& Start, const Vector3& End)
    {
        if (!Offsets.CheckLineOfSightAddr || !Offsets.WorldManager)
            return true;

        auto Fn = reinterpret_cast<tCheckLineOfSight>(Offsets.CheckLineOfSightAddr);
        float StartBuf[3] = { Start.X, Start.Y, Start.Z };
        float EndBuf[3]   = { End.X,   End.Y,   End.Z };

        return Fn(Offsets.WorldManager, StartBuf, EndBuf);
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
        // 1. Center ray
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

        bool LeftOk = (fabsf(LeftStart.Y - Start.Y) <= 1.5f && fabsf(LeftEnd.Y - End.Y) <= 1.5f && CheckLineOfSight(LeftStart, LeftEnd));

        // 3. Right shoulder ray (check sideways clearance relative to center track)
        Vector3 RightStart = { Start.X - PerpX, Start.Y, Start.Z - PerpZ };
        RightStart.Y = GetGroundHeight(RightStart.X, RightStart.Z);
        Vector3 RightEnd = { End.X - PerpX, End.Y, End.Z - PerpZ };
        RightEnd.Y = GetGroundHeight(RightEnd.X, RightEnd.Z);

        bool RightOk = (fabsf(RightStart.Y - Start.Y) <= 1.5f && fabsf(RightEnd.Y - End.Y) <= 1.5f && CheckLineOfSight(RightStart, RightEnd));

        // BOTH shoulders must be clear to ensure character body width fits without scraping walls/corners
        if (!LeftOk || !RightOk)
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

        // Overall slope check: reject slopes steeper than 0.28 (~15.6 degrees uphill) or 0.40 (downhill)
        F32 StartY = GetGroundHeight(Start.X, Start.Z);
        if (StartY == 0.0f) StartY = Start.Y;
        F32 EndY = GetGroundHeight(End.X, End.Z);
        if (EndY == 0.0f) EndY = End.Y;

        F32 deltaY = EndY - StartY;
        if (deltaY > 0.0f && (deltaY / Dist) > 0.28f)
            return false;
        if (deltaY < 0.0f && (-deltaY / Dist) > 0.40f)
            return false;

        // Width clearance corridor check (0.45m radius = 0.90m clear corridor)
        if (!CheckWalkableClearance(Start, End, 0.45f))
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

            // Slope between consecutive samples (prevents crossing steep steps > 15.6 deg uphill, > 22 deg downhill)
            F32 stepSlope = (actualY - prevY) / sampleStep;
            if (stepSlope > 0.28f || stepSlope < -0.40f)
                return false;

            F32 expectedY = Start.Y + (End.Y - Start.Y) * t;

            // Reject terrain crests that bulge >0.8m above line
            if ((actualY - expectedY) > 0.8f)
                return false;

            // Reject ditches/drops that sink >1.5m below line
            if ((expectedY - actualY) > 1.5f)
                return false;

            prevY = actualY;
        }

        return true;
    }

    struct AStarCell
    {
        F32 gCost;
        F32 fCost;
        I16 parentX;
        I16 parentZ;
        U8 state; // 0 = unvisited, 1 = open, 2 = closed, 3 = blocked
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

        // Grid setup: 112x112 grid covers full distance with high resolution
        const int GridDim = 112;
        F32 DesiredSpan = TotalDist + 50.0f;
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

        static AStarCell Grid[112][112];
        memset(Grid, 0, sizeof(Grid));

        static HeapNode OpenHeap[8192];
        int HeapSize = 0;

        auto PushHeap = [&](I16 x, I16 z, F32 f) {
            if (HeapSize >= 8180) return;
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
        const int maxIterations = 14000;

        while (HeapSize > 0 && iterations < maxIterations)
        {
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
            F32 curWY = (cx == startGX && cz == startGZ) ? AdjustedStart.Y : GetGroundHeight(curWX, curWZ);
            Vector3 curPos = { curWX, curWY, curWZ };

            if ((cx != startGX || cz != startGZ) && (IsGoalNode(cx, cz) || (h < 60.0f && IsSegmentWalkable(curPos, AdjustedGoal))))
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

                F32 nWX = GridToWorldX(nx);
                F32 nWZ = GridToWorldZ(nz);
                F32 nWY = GetGroundHeight(nWX, nWZ);
                if (nWY == 0.0f)
                {
                    Grid[nx][nz].state = 3;
                    continue;
                }

                // HARD LIMIT: strictly block slopes where player slides or cliffs (uphill > 0.28, downhill > 0.40)
                F32 stepDist = costs[i];
                F32 deltaY = nWY - curWY;

                if (deltaY > 0.0f)
                {
                    if ((deltaY / stepDist) > 0.28f)
                    {
                        continue;
                    }
                }
                else
                {
                    if ((-deltaY / stepDist) > 0.40f)
                    {
                        continue;
                    }
                }

                // Width clearance corridor check (0.45m radius)
                Vector3 nPos = { nWX, nWY, nWZ };
                if (!CheckWalkableClearance(curPos, nPos, 0.45f))
                {
                    continue;
                }

                // ELEVATION PENALTY: quadratic uphill penalty heavily penalizes climbing hills
                F32 slopeGrad = deltaY / stepDist;
                F32 slopePenalty = (slopeGrad > 0.0f) ? (slopeGrad * slopeGrad * 120.0f * stepDist) : (fabsf(slopeGrad) * 5.0f * stepDist);
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

        Vector3 RawPath[256];
        int RawCount = 0;

        F32 bestWX = GridToWorldX(bestX);
        F32 bestWZ = GridToWorldZ(bestZ);
        F32 bestWY = GetGroundHeight(bestWX, bestWZ);
        Vector3 bestPos = { bestWX, bestWY, bestWZ };

        if (found || IsGoalNode(bestX, bestZ) || IsSegmentWalkable(bestPos, AdjustedGoal))
        {
            RawPath[RawCount++] = AdjustedGoal;
        }

        while (tx != -1 && tz != -1 && RawCount < 250)
        {
            F32 wx = GridToWorldX(tx);
            F32 wz = GridToWorldZ(tz);
            F32 wy = GetGroundHeight(wx, wz);
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
        Vector3 Smoothed[64];
        U32 SmoothCount = 0;

        Smoothed[SmoothCount++] = AdjustedStart;

        int curIdx = 0;
        while (curIdx < RawCount - 1 && SmoothCount < MaxWaypoints - 1)
        {
            int farthest = curIdx + 1;
            int maxLookahead = curIdx + 14;
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
        if (found || IsGoalNode(bestX, bestZ) || IsSegmentWalkable(bestPos, AdjustedGoal) || CheckLineOfSight(Smoothed[SmoothCount - 1], AdjustedGoal))
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
        TargetPos = Target;
        F32 GroundY = GetGroundHeight(TargetPos.X, TargetPos.Z);
        if (GroundY != 0.0f)
            TargetPos.Y = GroundY;

        ArrivalRadius = StopDistance > 1.0f ? StopDistance : 1.5f;
        Active = true;
        LastPacketTick = 0;

        if (TargetName && TargetName[0] != '\0')
            StringUtils::Copy(DestinationName, TargetName, sizeof(DestinationName));
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

        WaypointCount = BuildPath(StartPos, TargetPos, Waypoints, 64);
        CurrentWaypointIndex = 0;
        PathStartPos = StartPos;
        LastStuckCheckPos = StartPos;
        LastStuckCheckTick = GetTickCount();

        Logger::Info("Navigation: Path generated with %u waypoints.", WaypointCount);

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
    }

    void NavigationManager::Stop()
    {
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

        HWND Hwnd = GetGameHwnd();

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
        U32 Now = GetTickCount();
        if (Now - LastStuckCheckTick > 2500)
        {
            F32 MovedDx = CurPos.X - LastStuckCheckPos.X;
            F32 MovedDz = CurPos.Z - LastStuckCheckPos.Z;
            F32 MovedDist = Vector3::Sqrt(MovedDx * MovedDx + MovedDz * MovedDz);

            if (MovedDist < 0.20f)
            {
                Logger::Info("Navigation: Stuck/sliding detected (moved %.2fm in 2.5s, rem=%.1fm). Re-routing...", MovedDist, RemainingDistance);

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

                WaypointCount = BuildPath(CurPos, TargetPos, Waypoints, 64);
                CurrentWaypointIndex = 0;
                PathStartPos = CurPos;
                DetourLockTick = Now;

                if (WaypointCount == 0)
                {
                    Logger::Info("Navigation: Re-route found no path. Stopping.");
                    Stop();
                    return;
                }

                // Give detour at least 4.0s before another stuck check can trigger
                LastStuckCheckPos = CurPos;
                LastStuckCheckTick = Now + 1500;
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
                if (DistToWaypoint <= 1.8f || (DotPast > 0.0f && DistToWaypoint <= 3.5f))
                {
                    canAdvance = true;
                }
            }
            else
            {
                // Next waypoint is BLOCKED by a wall/corner OR in active detour recovery:
                // MUST clear the waypoint plane (DotPast > 0) OR reach within 0.35m!
                if (DotPast > 0.0f || DistToWaypoint <= 0.35f)
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
            *reinterpret_cast<F32*>(Offsets.CameraEye + 8) = EyeZ;

            // Secondary CameraEye (immediately follows ProjMatrix 4x4)
            if (Offsets.ProjMatrix)
            {
                U64 CamEye2 = Offsets.ProjMatrix + 0x40;
                *reinterpret_cast<F32*>(CamEye2) = EyeX;
                *reinterpret_cast<F32*>(CamEye2 + 8) = EyeZ;
            }
        }

        // Set direction vectors in LocalPlayer
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x60) = DirX;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x64) = 0.0f;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x68) = DirZ;

        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x338) = DirX;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x33C) = 0.0f;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x340) = DirZ;

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
