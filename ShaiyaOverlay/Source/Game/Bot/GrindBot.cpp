#include "GrindBot.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Combat/ComboManager.h"
#include "Game/Combat/HealManager.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/Navigation/NavigationManager.h"
#include "Game/Skills/SkillManager.h"
#include "Core/Logger.h"
#include "Core/StringUtils.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

namespace ShaiyaOverlay
{
    GrindBotConfig GrindBot::Config;
    GrindBotStats GrindBot::Stats;
    GrindBotState GrindBot::State = GrindBotState::Idle;
    bool GrindBot::ConfigLoaded = false;
    U32 GrindBot::CurrentTargetWorldId = 0;
    U32 GrindBot::LastStateTick = 0;

    const char* GrindBot::GetStateName()
    {
        switch (State)
        {
        case GrindBotState::Idle:            return "Idle";
        case GrindBotState::SearchingTarget: return "Searching Target";
        case GrindBotState::Approaching:     return "Approaching Target";
        case GrindBotState::Combat:          return "Combat";
        case GrindBotState::Looting:         return "Looting Drops";
        case GrindBotState::Resting:         return "Resting / Healing";
        case GrindBotState::ReturningHome:   return "Returning to Anchor";
        default:                             return "Unknown";
        }
    }

    void GrindBot::LoadConfig()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\grind_bot.ini");

        Config.Enabled = GetPrivateProfileIntA("GrindBot", "Enabled", 0, IniPath) != 0;
        Config.QuestMonstersOnly = GetPrivateProfileIntA("GrindBot", "QuestMonstersOnly", 0, IniPath) != 0;

        char Buf[64];
        GetPrivateProfileStringA("GrindBot", "LeashRadius", "35.0", Buf, sizeof(Buf), IniPath);
        Config.LeashRadius = static_cast<F32>(atof(Buf));

        GetPrivateProfileStringA("GrindBot", "CombatApproachDistance", "18.0", Buf, sizeof(Buf), IniPath);
        Config.CombatApproachDistance = static_cast<F32>(atof(Buf));

        GetPrivateProfileStringA("GrindBot", "RestHpThresholdPercent", "35.0", Buf, sizeof(Buf), IniPath);
        Config.RestHpThresholdPercent = static_cast<F32>(atof(Buf));

        Config.HasAnchor = GetPrivateProfileIntA("GrindBot", "HasAnchor", 0, IniPath) != 0;
        if (Config.HasAnchor)
        {
            GetPrivateProfileStringA("GrindBot", "AnchorX", "0.0", Buf, sizeof(Buf), IniPath);
            Config.AnchorPosition.X = static_cast<F32>(atof(Buf));
            GetPrivateProfileStringA("GrindBot", "AnchorY", "0.0", Buf, sizeof(Buf), IniPath);
            Config.AnchorPosition.Y = static_cast<F32>(atof(Buf));
            GetPrivateProfileStringA("GrindBot", "AnchorZ", "0.0", Buf, sizeof(Buf), IniPath);
            Config.AnchorPosition.Z = static_cast<F32>(atof(Buf));
        }

        ConfigLoaded = true;
    }

    void GrindBot::SaveConfig()
    {
        char IniPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, IniPath);
        strcat_s(IniPath, "\\grind_bot.ini");

        WritePrivateProfileStringA("GrindBot", "Enabled", Config.Enabled ? "1" : "0", IniPath);
        WritePrivateProfileStringA("GrindBot", "QuestMonstersOnly", Config.QuestMonstersOnly ? "1" : "0", IniPath);

        char Buf[64];
        sprintf_s(Buf, "%.1f", Config.LeashRadius);
        WritePrivateProfileStringA("GrindBot", "LeashRadius", Buf, IniPath);

        sprintf_s(Buf, "%.1f", Config.CombatApproachDistance);
        WritePrivateProfileStringA("GrindBot", "CombatApproachDistance", Buf, IniPath);

        sprintf_s(Buf, "%.1f", Config.RestHpThresholdPercent);
        WritePrivateProfileStringA("GrindBot", "RestHpThresholdPercent", Buf, IniPath);

        WritePrivateProfileStringA("GrindBot", "HasAnchor", Config.HasAnchor ? "1" : "0", IniPath);
        if (Config.HasAnchor)
        {
            sprintf_s(Buf, "%.2f", Config.AnchorPosition.X);
            WritePrivateProfileStringA("GrindBot", "AnchorX", Buf, IniPath);
            sprintf_s(Buf, "%.2f", Config.AnchorPosition.Y);
            WritePrivateProfileStringA("GrindBot", "AnchorY", Buf, IniPath);
            sprintf_s(Buf, "%.2f", Config.AnchorPosition.Z);
            WritePrivateProfileStringA("GrindBot", "AnchorZ", Buf, IniPath);
        }
    }

    void GrindBot::SetAnchor(const Vector3& Pos)
    {
        Config.AnchorPosition = Pos;
        Config.HasAnchor = true;
        SaveConfig();
        Logger::Info("GrindBot: Anchor position set to (%.1f, %.1f, %.1f)", Pos.X, Pos.Y, Pos.Z);
    }

    void GrindBot::ClearAnchor()
    {
        Config.HasAnchor = false;
        SaveConfig();
        Logger::Info("GrindBot: Anchor cleared.");
    }

    void GrindBot::ResetStats()
    {
        Stats.MonstersKilled = 0;
        Stats.ItemsLooted = 0;
        Stats.SessionStartTick = GetTickCount();
    }

    void GrindBot::ToggleActive()
    {
        Config.Enabled = !Config.Enabled;
        if (Config.Enabled)
        {
            const auto& player = EntityManager::GetLocalPlayer();
            if (player.Valid && !Config.HasAnchor)
            {
                SetAnchor(player.Position);
            }
            if (Stats.SessionStartTick == 0)
                Stats.SessionStartTick = GetTickCount();

            State = GrindBotState::SearchingTarget;
            Logger::Info("GrindBot: Bot STARTED.");
        }
        else
        {
            State = GrindBotState::Idle;
            ComboManager::SetActive(false);
            if (NavigationManager::IsNavigating())
                NavigationManager::Stop();
            Logger::Info("GrindBot: Bot STOPPED.");
        }
        SaveConfig();
    }

    bool GrindBot::SelectNextTarget()
    {
        const auto& monsters = EntityManager::GetNearbyMonsters();
        const auto& player = EntityManager::GetLocalPlayer();
        if (!player.Valid) return false;

        Vector3 center = Config.HasAnchor ? Config.AnchorPosition : player.Position;

        F32 bestDist = 1e9f;
        U32 bestWorldId = 0;

        for (U32 i = 0; i < monsters.GetCount(); ++i)
        {
            const auto& m = monsters[i];
            if (!m.Alive || m.CurrentHp == 0) continue;
            if (Config.QuestMonstersOnly && !m.IsQuestTarget) continue;

            F32 distFromCenter = m.Position.DistanceTo(center);
            if (distFromCenter > Config.LeashRadius) continue;

            F32 distFromPlayer = m.Position.DistanceTo(player.Position);
            if (distFromPlayer < bestDist)
            {
                bestDist = distFromPlayer;
                bestWorldId = m.WorldId;
            }
        }

        if (bestWorldId != 0)
        {
            CurrentTargetWorldId = bestWorldId;
            SkillManager::SetTarget(bestWorldId);
            return true;
        }

        return false;
    }

    void GrindBot::Update()
    {
        if (!ConfigLoaded)
            LoadConfig();

        if (!Config.Enabled)
        {
            State = GrindBotState::Idle;
            return;
        }

        const auto& player = EntityManager::GetLocalPlayer();
        if (!player.Valid || player.CurrentHp == 0 || player.MaxHp == 0)
            return;

        U32 now = GetTickCount();

        // 1. Safety Rest Check
        F32 hpPct = (static_cast<F32>(player.CurrentHp) / static_cast<F32>(player.MaxHp)) * 100.0f;
        if (hpPct <= Config.RestHpThresholdPercent)
        {
            if (State != GrindBotState::Resting)
            {
                State = GrindBotState::Resting;
                ComboManager::SetActive(false);
                if (NavigationManager::IsNavigating())
                    NavigationManager::Stop();
                Logger::Info("GrindBot: Health critical (%.1f%% <= %.1f%%). Resting and healing...",
                    hpPct, Config.RestHpThresholdPercent);
            }
            return;
        }
        else if (State == GrindBotState::Resting && hpPct >= 80.0f)
        {
            State = GrindBotState::SearchingTarget;
            Logger::Info("GrindBot: Health recovered (%.1f%%). Resuming grind...", hpPct);
        }

        // 2. Leash Distance Check (Return home if strayed too far from Anchor)
        if (Config.HasAnchor)
        {
            F32 distFromAnchor = player.Position.DistanceTo(Config.AnchorPosition);
            if (distFromAnchor > Config.LeashRadius + 15.0f)
            {
                if (State != GrindBotState::ReturningHome)
                {
                    State = GrindBotState::ReturningHome;
                    ComboManager::SetActive(false);
                    NavigationManager::WalkTo(Config.AnchorPosition, "Home Anchor", 3.0f);
                    Logger::Info("GrindBot: Exceeded leash distance (%.1fm > %.1fm). Returning to Anchor...",
                        distFromAnchor, Config.LeashRadius);
                }
                return;
            }
        }

        // 3. State Machine
        switch (State)
        {
        case GrindBotState::SearchingTarget:
        {
            if (SelectNextTarget())
            {
                State = GrindBotState::Approaching;
                LastStateTick = now;
            }
            break;
        }

        case GrindBotState::Approaching:
        {
            const auto& monsters = EntityManager::GetNearbyMonsters();
            const MonsterEntity* targetMob = nullptr;
            for (U32 i = 0; i < monsters.GetCount(); ++i)
            {
                if (monsters[i].WorldId == CurrentTargetWorldId && monsters[i].Alive)
                {
                    targetMob = &monsters[i];
                    break;
                }
            }

            if (!targetMob)
            {
                // Target lost or despawned
                State = GrindBotState::SearchingTarget;
                break;
            }

            F32 dist = player.Position.DistanceTo(targetMob->Position);
            if (dist <= Config.CombatApproachDistance)
            {
                // Within range: stop moving and engage combat!
                if (NavigationManager::IsNavigating())
                    NavigationManager::Stop();

                SkillManager::SetTarget(CurrentTargetWorldId);
                ComboManager::SetActive(true);
                State = GrindBotState::Combat;
                LastStateTick = now;
            }
            else
            {
                if (!NavigationManager::IsNavigating() && (now - LastStateTick > 1000))
                {
                    NavigationManager::WalkTo(targetMob->Position, targetMob->Name, Config.CombatApproachDistance - 2.0f);
                    LastStateTick = now;
                }
            }
            break;
        }

        case GrindBotState::Combat:
        {
            const auto& monsters = EntityManager::GetNearbyMonsters();
            bool targetAlive = false;
            for (U32 i = 0; i < monsters.GetCount(); ++i)
            {
                if (monsters[i].WorldId == CurrentTargetWorldId && monsters[i].Alive)
                {
                    targetAlive = true;
                    break;
                }
            }

            if (!targetAlive)
            {
                // Monster killed!
                Stats.MonstersKilled++;
                ComboManager::SetActive(false);
                CurrentTargetWorldId = 0;

                // Check for ground loot within pickup distance
                if (GroundItemManager::GetItemCount() > 0)
                {
                    State = GrindBotState::Looting;
                    LastStateTick = now;
                }
                else
                {
                    State = GrindBotState::SearchingTarget;
                }
            }
            break;
        }

        case GrindBotState::Looting:
        {
            // Give auto-loot 2.5 seconds to collect nearby items
            if (now - LastStateTick > 2500 || GroundItemManager::GetItemCount() == 0)
            {
                State = GrindBotState::SearchingTarget;
            }
            break;
        }

        case GrindBotState::ReturningHome:
        {
            if (!NavigationManager::IsNavigating())
            {
                State = GrindBotState::SearchingTarget;
            }
            break;
        }

        default:
            break;
        }
    }
}
