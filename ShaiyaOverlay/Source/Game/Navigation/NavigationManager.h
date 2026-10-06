#pragma once

#include "Core/Types.h"
#include <atomic>

namespace ShaiyaOverlay
{
    class NavigationManager
    {
    public:
        static void Update();

        static void WalkTo(const Vector3& Target, const char* TargetName = nullptr, F32 StopDistance = 2.5f);
        static void Stop();
        static void Shutdown();

        // Raycasting / Collision queries
        static bool CheckLineOfSightElevated(const Vector3& Start, const Vector3& End, F32 HeightOffset);
        static bool CheckLineOfSight(const Vector3& Start, const Vector3& End);
        static bool CheckWalkableClearance(const Vector3& Start, const Vector3& End, F32 Radius = 0.70f);
        static bool IsSegmentWalkable(const Vector3& Start, const Vector3& End);
        static F32 GetGroundHeight(F32 X, F32 Z);

        // Path generation
        static U32 BuildPath(const Vector3& Start, const Vector3& Goal, Vector3* OutWaypoints, U32 MaxWaypoints);
        static U32 BuildPathAStar(const Vector3& Start, const Vector3& Goal, Vector3* OutWaypoints, U32 MaxWaypoints);

        static bool IsNavigating() { return Active; }
        static bool IsComputingPath() { return ComputingPath.load(); }
        static const Vector3& GetTargetPosition() { return TargetPos; }
        static const char* GetTargetName() { return DestinationName; }
        static F32 GetRemainingDistance() { return RemainingDistance; }
        static const Vector3* GetWaypoints() { return Waypoints; }
        static U32 GetWaypointCount() { return WaypointCount; }
        static U32 GetCurrentWaypointIndex() { return CurrentWaypointIndex; }

    private:
        static bool Active;
        static Vector3 TargetPos;
        static F32 ArrivalRadius;
        static F32 RemainingDistance;
        static char DestinationName[64];
        static U32 LastPacketTick;

        // Pathfinding Waypoints
        static Vector3 Waypoints[64];
        static U32 WaypointCount;
        static U32 CurrentWaypointIndex;
        static Vector3 PathStartPos;

        // Dynamic stuck detection & re-routing
        static Vector3 LastStuckCheckPos;
        static U32 LastStuckCheckTick;
        static U32 DetourLockTick;

        // Async Background Worker
        static void InitWorker();
        static void RequestAsyncPath(const Vector3& Start, const Vector3& Goal);
        static DWORD WINAPI PathWorkerProc(LPVOID lpParam);

        static std::atomic<bool> WorkerInitialized;
        static std::atomic<bool> ComputingPath;
        static std::atomic<bool> PathReady;
        static std::atomic<bool> CancelRequested;

        static Vector3 RequestStart;
        static Vector3 RequestGoal;
        static Vector3 StagedWaypoints[64];
        static U32 StagedWaypointCount;

        static HANDLE hWorkerThread;
        static HANDLE hWorkEvent;
        static HANDLE hExitEvent;
        static CRITICAL_SECTION PathLock;
    };
}
