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

        static bool IsNavigating() { return Active; }
        static const Vector3& GetTargetPosition() { return TargetPos; }
        static const char* GetTargetName() { return DestinationName; }
        static F32 GetRemainingDistance() { return RemainingDistance; }

    private:
        static bool Active;
        static Vector3 TargetPos;
        static F32 ArrivalRadius;
        static F32 RemainingDistance;
        static char DestinationName[64];
        static U32 LastPacketTick;
    };
}
