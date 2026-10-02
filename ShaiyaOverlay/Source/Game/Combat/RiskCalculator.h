#pragma once

#include "Core/Types.h"
#include "Game/Entities/Entity.h"
#include "Game/Entities/LocalPlayer.h"

namespace ShaiyaOverlay
{
    struct RiskAssessment
    {
        ThreatLevel OverallThreat;
        U32 ThreatScore; // 0 to 100
        U32 CloseHostilesCount;
        U32 NearbyHostilesCount;
        char Summary[128];
    };

    class RiskCalculator
    {
    public:
        static RiskAssessment Evaluate(const PlayerData& Player, const FixedList<MonsterEntity, 128>& Monsters);
    };
}
