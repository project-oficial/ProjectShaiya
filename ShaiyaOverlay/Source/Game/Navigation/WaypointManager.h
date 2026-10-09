#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct CustomWaypoint
    {
        char Name[64];
        Vector3 Position;
        F32 Distance;
    };

    struct UsefulNpc
    {
        char Name[64];
        char Role[32]; // "Gatekeeper", "Merchant", "Quest", "Blacksmith", etc.
        Vector3 Position;
        F32 Distance;
        U8 Type;
        U16 Id;
    };

    class WaypointManager
    {
    public:
        static void Initialize();
        static void Update();

        // Custom Waypoints
        static const FixedList<CustomWaypoint, 32>& GetCustomWaypoints() { return CustomWaypoints; }
        static bool AddCustomWaypoint(const char* Name, const Vector3& Pos);
        static bool RemoveCustomWaypoint(U32 Index);
        static void SaveCustomWaypoints();
        static void LoadCustomWaypoints();

        // Useful NPCs
        static const FixedList<UsefulNpc, 256>& GetUsefulNpcs() { return UsefulNpcs; }
        static void RefreshUsefulNpcs();
        static bool ResolveNpcName(U8 NpcType, U16 NpcId, char* OutName, U32 MaxLen);

    private:
        static FixedList<CustomWaypoint, 32> CustomWaypoints;
        static FixedList<UsefulNpc, 256> UsefulNpcs;
        static U32 LastScanTick;
        static bool Initialized;
    };
}
