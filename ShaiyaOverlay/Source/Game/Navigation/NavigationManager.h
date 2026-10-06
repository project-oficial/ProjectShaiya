#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class NavigationManager
    {
    public:
        static void Update();

        static void WalkTo(const Vector3& Target, const char* TargetName = nullptr, F32 StopDistance = 2.5f);
        static void Stop();

        // Raycasting / Collision queries
        static bool CheckLineOfSight(const Vector3& Start, const Vector3& End);
        static bool CheckWalkableClearance(const Vector3& Start, const Vector3& End, F32 Radius = 0.75f);
        static bool IsSegmentWalkable(const Vector3& Start, const Vector3& End);
        static F32 GetGroundHeight(F32 X, F32 Z);

        // Path generation
        static U32 BuildPath(const Vector3& Start, const Vector3& Goal, Vector3* OutWaypoints, U32 MaxWaypoints);
        static U32 BuildPathAStar(const Vector3& Start, const Vector3& Goal, Vector3* OutWaypoints, U32 MaxWaypoints);

        static bool IsNavigating() { return Active; }
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
    };
}
