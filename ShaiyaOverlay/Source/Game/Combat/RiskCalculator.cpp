#include "RiskCalculator.h"
#include "Core/StringUtils.h"

namespace ShaiyaOverlay
{
    RiskAssessment RiskCalculator::Evaluate(const PlayerData& Player, const FixedList<MonsterEntity, 128>& Monsters)
    {
        RiskAssessment Assessment = { ThreatLevel::None, 0, 0, 0, { 0 } };

        if (!Player.Valid)
        {
            StringUtils::Copy(Assessment.Summary, "Waiting for Player Data...", sizeof(Assessment.Summary));
            return Assessment;
        }

        U32 TotalMonsters = Monsters.GetCount();
        U32 CloseHostiles = 0;
        U32 MidHostiles = 0;
        F32 MinDistance = 9999.0f;

        for (U32 I = 0; I < TotalMonsters; ++I)
        {
            const MonsterEntity& Mob = Monsters[I];
            if (!Mob.Alive)
                continue;

            if (Mob.Distance < MinDistance)
                MinDistance = Mob.Distance;

            if (Mob.Distance < 15.0f)
                ++CloseHostiles;
            else if (Mob.Distance < 35.0f)
                ++MidHostiles;
        }

        Assessment.CloseHostilesCount = CloseHostiles;
        Assessment.NearbyHostilesCount = CloseHostiles + MidHostiles;

        F32 HpRatio = Player.GetHpPercentage();

        // Calculate threat score (0 - 100)
        U32 Score = 0;
        Score += CloseHostiles * 30;
        Score += MidHostiles * 10;

        if (HpRatio < 0.25f)
            Score += 50;
        else if (HpRatio < 0.50f)
            Score += 30;
        else if (HpRatio < 0.75f)
            Score += 15;

        if (Score > 100)
            Score = 100;

        Assessment.ThreatScore = Score;

        if (CloseHostiles >= 2 || (CloseHostiles >= 1 && HpRatio < 0.35f) || Score >= 75)
        {
            Assessment.OverallThreat = ThreatLevel::Fatal;
            StringUtils::Format(Assessment.Summary, sizeof(Assessment.Summary),
                "FATAL: %u Mobs Point-Blank! HP: %u%% - RETREAT!", CloseHostiles, (U32)(HpRatio * 100.0f));
        }
        else if (CloseHostiles >= 1 || MidHostiles >= 3 || Score >= 45)
        {
            Assessment.OverallThreat = ThreatLevel::High;
            StringUtils::Format(Assessment.Summary, sizeof(Assessment.Summary),
                "HIGH: %u Hostiles Nearby (Dist: %.1fm)", Assessment.NearbyHostilesCount, MinDistance);
        }
        else if (MidHostiles >= 1 || Score >= 20)
        {
            Assessment.OverallThreat = ThreatLevel::Medium;
            StringUtils::Format(Assessment.Summary, sizeof(Assessment.Summary),
                "CAUTION: %u Hostiles in Perimeter", MidHostiles);
        }
        else
        {
            Assessment.OverallThreat = ThreatLevel::Low;
            StringUtils::Copy(Assessment.Summary, "SAFE: No Hostiles in Immediate Area", sizeof(Assessment.Summary));
        }

        return Assessment;
    }
}
