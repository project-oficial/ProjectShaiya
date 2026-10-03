#include "Menu.h"
#include "Core/StringUtils.h"
#include "Core/Camera.h"
#include "Core/Logger.h"
#include "Hooks/WndProcHook.h"
#include "ThirdParty/ImGui/imgui.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Skills/SkillManager.h"
#include "Game/Buffs/BuffManager.h"
#include "Game/Combat/RiskCalculator.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/QuickSlots/QuickSlotManager.h"
#include "Game/Quests/QuestManager.h"
#include "Game/Navigation/NavigationManager.h"
#include "Core/MCPTool/MCPBridge.h"

namespace ShaiyaOverlay
{
    bool Menu::SnaplinesEnabled = true;
    bool Menu::QuestWaypointsEnabled = true;
    static ImVec4 GetThreatColor(ThreatLevel Threat)
    {
        switch (Threat)
        {
        case ThreatLevel::Fatal:  return ImVec4(1.0f, 0.15f, 0.15f, 1.0f);
        case ThreatLevel::High:   return ImVec4(1.0f, 0.5f, 0.1f, 1.0f);
        case ThreatLevel::Medium: return ImVec4(1.0f, 0.85f, 0.15f, 1.0f);
        case ThreatLevel::Low:    return ImVec4(0.2f, 0.9f, 0.3f, 1.0f);
        default:                  return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        }
    }

    void Menu::Render()
    {
        // Polling fallback only if WndProc hook is not attached
        if (!WndProcHook::IsAttached())
        {
            static bool InsertWasDown = false;
            bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
            if (insertDown && !InsertWasDown)
            {
                WndProcHook::ToggleMenu();
                Logger::Info("Menu: Toggled via INSERT key fallback. State: %s", WndProcHook::IsMenuOpen() ? "OPEN" : "CLOSED");
            }
            InsertWasDown = insertDown;
        }

#ifdef MCP_TOOL
        MCPBridge::OnRenderTick();
#endif

        // Update game states synchronously on render tick
        EntityManager::Update();
        SkillManager::Update();
        BuffManager::Update();
        GroundItemManager::Update();
        QuickSlotManager::Update();
        QuestManager::Update();
        NavigationManager::Update();

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        const FixedList<MonsterEntity, 128>& Monsters = EntityManager::GetNearbyMonsters();
        RiskAssessment Risk = RiskCalculator::Evaluate(Player, Monsters);

        // Control ImGui software cursor visibility based on menu state
        ImGui::GetIO().MouseDrawCursor = WndProcHook::IsMenuOpen();

        // Always-on Hardcore Threat Banner (visible whether menu is toggled or not)
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400.0f, NavigationManager::IsNavigating() ? 95.0f : 75.0f), ImGuiCond_Always);

        ImGuiWindowFlags BannerFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        if (!WndProcHook::IsMenuOpen())
            BannerFlags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

        if (ImGui::Begin("Shaiya Hardcore Overlay", nullptr, BannerFlags))
        {
            ImVec4 ThreatCol = GetThreatColor(Risk.OverallThreat);
            ImGui::TextColored(ThreatCol, "[RISK: %s]", Risk.Summary);
            ImGui::Text("FPS: %.1f | Hostiles: %u | Quests: %u | [INS] %s",
                ImGui::GetIO().Framerate, Risk.NearbyHostilesCount, QuestManager::GetQuestCount(),
                WndProcHook::IsMenuOpen() ? "Close UI" : "Open UI");

            if (NavigationManager::IsNavigating())
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), ">> AUTO-WALK: %s (%.1fm)",
                    NavigationManager::GetTargetName(), NavigationManager::GetRemainingDistance());
                if (WndProcHook::IsMenuOpen())
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("STOP"))
                    {
                        NavigationManager::Stop();
                    }
                }
            }
        }
        ImGui::End();

        // 3D World to Screen snaplines (always visible when enabled)
        RenderGroundItemSnaplines();
        RenderQuestWaypoints();

        // Detailed overlay windows visible when UI is opened via INSERT
        if (!WndProcHook::IsMenuOpen())
            return;

        RenderOverviewWindow();
        RenderEntitiesWindow();
        RenderGroundItemsWindow();
        RenderSkillsWindow();
        RenderQuickSlotsWindow();
        RenderQuestsWindow();
        RenderBuffsWindow();
    }

    void Menu::RenderOverviewWindow()
    {
        const PlayerData& Player = EntityManager::GetLocalPlayer();
        const FixedList<MonsterEntity, 128>& Monsters = EntityManager::GetNearbyMonsters();
        RiskAssessment Risk = RiskCalculator::Evaluate(Player, Monsters);

        ImGui::SetNextWindowPos(ImVec2(10.0f, 95.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380.0f, 230.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Local Player & Hardcore Risk"))
        {
            if (Player.Valid)
            {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Character ID: %u | Level: %u", Player.Id, Player.Level);
                ImGui::Text("Position: X: %.1f  Y: %.1f  Z: %.1f", Player.Position.X, Player.Position.Y, Player.Position.Z);
                ImGui::Separator();

                F32 HpPct = Player.GetHpPercentage();
                char HpText[64];
                StringUtils::Format(HpText, sizeof(HpText), "%u / %u (%.0f%%)", Player.CurrentHp, Player.MaxHp, HpPct * 100.0f);

                ImVec4 HpBarColor = (HpPct < 0.35f) ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.1f, 0.8f, 0.2f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, HpBarColor);
                ImGui::Text("Health:");
                ImGui::ProgressBar(HpPct, ImVec2(-1.0f, 0.0f), HpText);
                ImGui::PopStyleColor();

                ImGui::Separator();
                ImGui::Text("Death Threat Level: ");
                ImGui::SameLine();
                ImGui::TextColored(GetThreatColor(Risk.OverallThreat), "%s", Risk.Summary);

                char ScoreText[32];
                StringUtils::Format(ScoreText, sizeof(ScoreText), "Threat Score: %u / 100", Risk.ThreatScore);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, GetThreatColor(Risk.OverallThreat));
                ImGui::ProgressBar((F32)Risk.ThreatScore / 100.0f, ImVec2(-1.0f, 0.0f), ScoreText);
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Scanning character memory...");
            }
        }
        ImGui::End();
    }

    void Menu::RenderEntitiesWindow()
    {
        const FixedList<MonsterEntity, 128>& Monsters = EntityManager::GetNearbyMonsters();
        U32 MonsterCount = Monsters.GetCount();

        ImGui::SetNextWindowPos(ImVec2(400.0f, 10.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(570.0f, 315.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Nearby Hostiles & Entities"))
        {
            ImGui::Text("Entities in Range: %u", MonsterCount);
            ImGui::Separator();

            if (ImGui::BeginTable("EntitiesTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Mob ID", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Target Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Lvl", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Risk", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Nav", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableHeadersRow();

                for (U32 I = 0; I < MonsterCount; ++I)
                {
                    const MonsterEntity& Mob = Monsters[I];
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%u", Mob.MobId);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", Mob.Name);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%u", Mob.Level);

                    ImGui::TableSetColumnIndex(3);
                    if (Mob.Alive)
                    {
                        ImGui::Text("%u/%u", Mob.CurrentHp, Mob.MaxHp);
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "DEAD");
                    }

                    ImGui::TableSetColumnIndex(4);
                    ImVec4 DistColor = (Mob.Distance < 15.0f) ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) :
                                      (Mob.Distance < 30.0f) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                                                               ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
                    ImGui::TextColored(DistColor, "%.1fm", Mob.Distance);

                    ImGui::TableSetColumnIndex(5);
                    ImVec4 ThreatColor = GetThreatColor(Mob.Threat);
                    const char* ThreatStr = (Mob.Threat == ThreatLevel::Fatal)  ? "FATAL" :
                                            (Mob.Threat == ThreatLevel::High)   ? "HIGH" :
                                            (Mob.Threat == ThreatLevel::Medium) ? "MED" :
                                            (Mob.Threat == ThreatLevel::Low)    ? "LOW" : "NONE";
                    ImGui::TextColored(ThreatColor, "%s", ThreatStr);

                    ImGui::TableSetColumnIndex(6);
                    if (Mob.Alive)
                    {
                        char BtnLabel[32];
                        StringUtils::Format(BtnLabel, sizeof(BtnLabel), "Walk##M%u", Mob.WorldId);
                        if (ImGui::Button(BtnLabel, ImVec2(-1.0f, 0.0f)))
                        {
                            NavigationManager::WalkTo(Mob.Position, Mob.Name, 2.0f);
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("-");
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Menu::RenderGroundItemsWindow()
    {
        const FixedList<GroundItem, 128>& Items = GroundItemManager::GetGroundItems();
        U32 ItemCount = Items.GetCount();

        ImGui::SetNextWindowPos(ImVec2(400.0f, 335.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(570.0f, 220.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Ground Loot & Dropped Items"))
        {
            ImGui::Checkbox("Draw Tracer Lines to Ground Items", &SnaplinesEnabled);
            ImGui::SameLine();
            ImGui::TextDisabled("| Total on Ground: %u", ItemCount);
            ImGui::Separator();

            if (ImGui::BeginTable("GroundItemsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Item Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("World ID", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Nav", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableHeadersRow();

                if (ItemCount == 0)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No dropped items nearby");
                    ImGui::TableSetColumnIndex(1); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(2); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(3); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(4); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(5); ImGui::Text("-");
                }
                else
                {
                    const FixedList<ActiveQuest, 16>& ActiveQuests = QuestManager::GetActiveQuests();

                    for (U32 I = 0; I < ItemCount; ++I)
                    {
                        const GroundItem& Item = Items[I];
                        ImGui::TableNextRow();

                        bool IsQuestDrop = false;
                        U32 QCur = 0;
                        U32 QNeed = 0;

                        for (U32 Q = 0; Q < ActiveQuests.GetCount(); ++Q)
                        {
                            const ActiveQuest& ActQ = ActiveQuests[Q];
                            for (U32 O = 0; O < ActQ.ItemObjectiveCount; ++O)
                            {
                                const QuestItemObjective& Obj = ActQ.ItemObjectives[O];
                                if (!Obj.Completed && Item.Type == Obj.ItemType && Item.TypeId == Obj.ItemTypeId)
                                {
                                    IsQuestDrop = true;
                                    QCur = Obj.CurrentCount;
                                    QNeed = Obj.CountNeeded;
                                    break;
                                }
                            }
                            if (IsQuestDrop) break;
                        }

                        ImGui::TableSetColumnIndex(0);
                        if (IsQuestDrop)
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 1.0f, 1.0f), "[QUEST] %s (%u/%u)", Item.Name, QCur, QNeed);
                        }
                        else if (Item.Type == 44)
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", Item.Name);
                        else if (Item.Distance < 10.0f)
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "%s", Item.Name);
                        else
                            ImGui::Text("%s", Item.Name);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", Item.Category);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%u", Item.Count > 0 ? Item.Count : 1);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%.1fm", Item.Distance);

                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%u", Item.WorldId);

                        ImGui::TableSetColumnIndex(5);
                        char ItemBtn[32];
                        StringUtils::Format(ItemBtn, sizeof(ItemBtn), "Walk##I%u", Item.WorldId);
                        if (ImGui::Button(ItemBtn, ImVec2(-1.0f, 0.0f)))
                        {
                            NavigationManager::WalkTo(Item.Position, Item.Name, 1.5f);
                        }
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Menu::RenderQuestsWindow()
    {
        const FixedList<ActiveQuest, 16>& Quests = QuestManager::GetActiveQuests();
        U32 QuestCount = Quests.GetCount();

        ImGui::SetNextWindowPos(ImVec2(10.0f, 565.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(590.0f, 240.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Active Quests & Waypoints"))
        {
            ImGui::Checkbox("Draw Waypoint Lines to Quest Destinations", &QuestWaypointsEnabled);
            ImGui::SameLine();
            ImGui::TextDisabled("| Active: %u", QuestCount);
            ImGui::Separator();

            if (ImGui::BeginTable("QuestsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Quest Title", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Objective Progress", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("Turn-In Target", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("Distance", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Nav", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableHeadersRow();

                if (QuestCount == 0)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No active quests");
                    ImGui::TableSetColumnIndex(2); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(3); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(4); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(5); ImGui::Text("-");
                }
                else
                {
                    for (U32 I = 0; I < QuestCount; ++I)
                    {
                        const ActiveQuest& Q = Quests[I];
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u", Q.QuestId);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", Q.Title);

                        ImGui::TableSetColumnIndex(2);
                        if (Q.ObjectiveCount == 0 && Q.ItemObjectiveCount == 0)
                        {
                            ImGui::Text("Talk to NPC");
                        }
                        else
                        {
                            char ObjText[160] = { 0 };
                            for (U32 O = 0; O < Q.ObjectiveCount; ++O)
                            {
                                char SingleObj[64];
                                StringUtils::Format(SingleObj, sizeof(SingleObj), "%s: %u/%u%s",
                                    Q.Objectives[O].TargetMobName,
                                    Q.Objectives[O].CurrentCount,
                                    Q.Objectives[O].CountNeeded,
                                    (O + 1 < Q.ObjectiveCount || Q.ItemObjectiveCount > 0) ? ", " : "");
                                StringUtils::Copy(ObjText + StringUtils::Length(ObjText), SingleObj, sizeof(ObjText) - StringUtils::Length(ObjText));
                            }
                            for (U32 O = 0; O < Q.ItemObjectiveCount; ++O)
                            {
                                char SingleObj[64];
                                StringUtils::Format(SingleObj, sizeof(SingleObj), "%s: %u/%u%s",
                                    Q.ItemObjectives[O].ItemName,
                                    Q.ItemObjectives[O].CurrentCount,
                                    Q.ItemObjectives[O].CountNeeded,
                                    O + 1 < Q.ItemObjectiveCount ? ", " : "");
                                StringUtils::Copy(ObjText + StringUtils::Length(ObjText), SingleObj, sizeof(ObjText) - StringUtils::Length(ObjText));
                            }
                            ImGui::Text("%s", ObjText);
                        }

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%s", Q.DestinationName);

                        ImGui::TableSetColumnIndex(4);
                        if (Q.HasDestination)
                            ImGui::Text("%.1fm", Q.Distance);
                        else
                            ImGui::Text("-");

                        ImGui::TableSetColumnIndex(5);
                        bool HasIncompleteHunt = false;
                        U16 IncompleteMobId = 0;
                        for (U32 O = 0; O < Q.ObjectiveCount; ++O)
                        {
                            if (!Q.Objectives[O].Completed)
                            {
                                HasIncompleteHunt = true;
                                IncompleteMobId = Q.Objectives[O].TargetMobId;
                                break;
                            }
                        }

                        bool HasIncompleteItem = false;
                        U8 IncompleteItemType = 0;
                        U8 IncompleteItemTypeId = 0;
                        U16 DropMobId = 0;
                        char DropMobName[64] = { 0 };
                        for (U32 O = 0; O < Q.ItemObjectiveCount; ++O)
                        {
                            if (!Q.ItemObjectives[O].Completed)
                            {
                                HasIncompleteItem = true;
                                IncompleteItemType = Q.ItemObjectives[O].ItemType;
                                IncompleteItemTypeId = Q.ItemObjectives[O].ItemTypeId;
                                DropMobId = Q.ItemObjectives[O].DroppedByMobId;
                                StringUtils::Copy(DropMobName, Q.ItemObjectives[O].DroppedByMobName, sizeof(DropMobName));
                                break;
                            }
                        }

                        // Priority 1: If item is dropped on the ground nearby -> [To Item]
                        bool FoundGroundItem = false;
                        Vector3 GroundItemPos;
                        char GroundItemName[64];
                        if (HasIncompleteItem)
                        {
                            const FixedList<GroundItem, 128>& GroundItems = GroundItemManager::GetGroundItems();
                            F32 BestItemDist = 99999.0f;
                            for (U32 G = 0; G < GroundItems.GetCount(); ++G)
                            {
                                const GroundItem& GI = GroundItems[G];
                                if (GI.Type == IncompleteItemType && GI.TypeId == IncompleteItemTypeId && GI.Distance < BestItemDist)
                                {
                                    BestItemDist = GI.Distance;
                                    GroundItemPos = GI.Position;
                                    StringUtils::Copy(GroundItemName, GI.Name, sizeof(GroundItemName));
                                    FoundGroundItem = true;
                                }
                            }
                        }

                        if (FoundGroundItem)
                        {
                            char BtnLabel[32];
                            StringUtils::Format(BtnLabel, sizeof(BtnLabel), "To Item##Q%u", Q.QuestId);
                            if (ImGui::Button(BtnLabel, ImVec2(-1.0f, 0.0f)))
                            {
                                NavigationManager::WalkTo(GroundItemPos, GroundItemName, 1.5f);
                            }
                        }
                        else if (HasIncompleteHunt || (HasIncompleteItem && (DropMobId > 0 || DropMobName[0] != '\0')))
                        {
                            U16 TargetMobId = HasIncompleteHunt ? IncompleteMobId : DropMobId;
                            const FixedList<MonsterEntity, 128>& Mobs = EntityManager::GetNearbyMonsters();
                            F32 BestDist = 99999.0f;
                            Vector3 BestPos;
                            char BestName[64] = "Quest Mob";
                            bool FoundMob = false;
                            for (U32 M = 0; M < Mobs.GetCount(); ++M)
                            {
                                const MonsterEntity& Mob = Mobs[M];
                                bool Match = (TargetMobId > 0 && Mob.MobId == TargetMobId);
                                if (!Match && DropMobName[0] != '\0')
                                {
                                    Match = StringUtils::ContainsNormalized(Mob.Name, DropMobName);
                                }
                                if (Mob.Alive && Match && Mob.Distance < BestDist)
                                {
                                    BestDist = Mob.Distance;
                                    BestPos = Mob.Position;
                                    StringUtils::Copy(BestName, Mob.Name, sizeof(BestName));
                                    FoundMob = true;
                                }
                            }

                            if (FoundMob)
                            {
                                char BtnLabel[32];
                                StringUtils::Format(BtnLabel, sizeof(BtnLabel), "To Mob##Q%u", Q.QuestId);
                                if (ImGui::Button(BtnLabel, ImVec2(-1.0f, 0.0f)))
                                {
                                    NavigationManager::WalkTo(BestPos, BestName, 2.0f);
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("No Mob");
                            }
                        }
                        else
                        {
                            const FixedList<QuestMarker, 32>& Markers = QuestManager::GetQuestMarkers();
                            bool FoundTurnIn = false;
                            Vector3 TurnInPos;
                            char NpcName[64] = "Quest NPC";
                            for (U32 K = 0; K < Markers.GetCount(); ++K)
                            {
                                if (Markers[K].IsTurnIn)
                                {
                                    TurnInPos = Markers[K].Position;
                                    StringUtils::Copy(NpcName, Markers[K].NpcName, sizeof(NpcName));
                                    FoundTurnIn = true;
                                    break;
                                }
                            }

                            if (!FoundTurnIn && Q.HasDestination)
                            {
                                TurnInPos = Q.DestinationPos;
                                StringUtils::Copy(NpcName, Q.DestinationName, sizeof(NpcName));
                                FoundTurnIn = true;
                            }

                            if (FoundTurnIn)
                            {
                                char BtnLabel[32];
                                StringUtils::Format(BtnLabel, sizeof(BtnLabel), "To NPC##Q%u", Q.QuestId);
                                if (ImGui::Button(BtnLabel, ImVec2(-1.0f, 0.0f)))
                                {
                                    NavigationManager::WalkTo(TurnInPos, NpcName, 2.5f);
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("-");
                            }
                        }
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Menu::RenderSkillsWindow()
    {
        const FixedList<SkillInfo, 64>& Skills = SkillManager::GetSkills();
        U32 SkillCount = Skills.GetCount();

        static bool FilterOnlyLearned = true;

        U32 LearnedCount = 0;
        for (U32 I = 0; I < SkillCount; ++I)
        {
            if (Skills[I].IsLearned) LearnedCount++;
        }

        ImGui::SetNextWindowPos(ImVec2(10.0f, 335.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(440.0f, 240.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Learned Skills Tracker"))
        {
            ImGui::Checkbox("Somente Aprendidas", &FilterOnlyLearned);
            ImGui::SameLine();
            ImGui::TextDisabled("(%u/%u)", LearnedCount, SkillCount);

            U32 TargetWorldId = SkillManager::GetSelectedTargetWorldId();
            const FixedList<MonsterEntity, 128>& Mobs = EntityManager::GetNearbyMonsters();
            const char* TargetName = nullptr;
            U32 TargetCurHp = 0;
            U32 TargetMaxHp = 0;
            if (TargetWorldId != 0)
            {
                for (U32 M = 0; M < Mobs.GetCount(); ++M)
                {
                    if (Mobs[M].WorldId == TargetWorldId)
                    {
                        TargetName = Mobs[M].Name;
                        TargetCurHp = Mobs[M].CurrentHp;
                        TargetMaxHp = Mobs[M].MaxHp;
                        break;
                    }
                }
            }

            if (TargetName)
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Alvo: %s (%u/%u)", TargetName, TargetCurHp, TargetMaxHp);
            else if (TargetWorldId != 0)
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Alvo: ID 0x%08X", TargetWorldId);
            else
                ImGui::TextDisabled("Alvo: Nenhum (auto-alvo no mais próximo)");

            ImGui::Separator();

            if (ImGui::BeginTable("SkillsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Skill", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Lvl", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Ação", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableHeadersRow();

                for (U32 I = 0; I < SkillCount; ++I)
                {
                    const SkillInfo& Skill = Skills[I];
                    if (FilterOnlyLearned && !Skill.IsLearned)
                        continue;

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%u", Skill.SkillId);

                    ImGui::TableSetColumnIndex(1);
                    if (!Skill.IsLearned)
                        ImGui::TextDisabled("%s (Bloqueada)", Skill.Name);
                    else
                        ImGui::Text("%s", Skill.Name);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%u", Skill.Level);

                    ImGui::TableSetColumnIndex(3);
                    if (!Skill.IsLearned)
                    {
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "ARVORE");
                    }
                    else if (Skill.IsReady)
                    {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "PRONTA");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.1fs", Skill.CooldownRemaining);
                    }

                    ImGui::TableSetColumnIndex(4);
                    if (!Skill.IsLearned)
                    {
                        ImGui::TextDisabled("-");
                    }
                    else if (Skill.IsPassive)
                    {
                        ImGui::TextDisabled("PASSIVA");
                    }
                    else
                    {
                        char BtnLabel[32];
                        StringUtils::Format(BtnLabel, sizeof(BtnLabel), "Usar##S%u", Skill.SkillId);
                        if (!Skill.IsReady)
                            ImGui::BeginDisabled();

                        if (ImGui::SmallButton(BtnLabel))
                        {
                            SkillManager::CastSkill(Skill.LearnedSlot, Skill.TargetType);
                        }

                        if (!Skill.IsReady)
                            ImGui::EndDisabled();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Menu::RenderQuickSlotsWindow()
    {
        const FixedList<QuickSlotEntry, 30>& Slots = QuickSlotManager::GetSlots();
        U32 SlotCount = Slots.GetCount();

        ImGui::SetNextWindowPos(ImVec2(10.0f, 565.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520.0f, 240.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Quickslot Configuration"))
        {
            static int SelectedBar = 0;
            ImGui::RadioButton("Bar 1", &SelectedBar, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Bar 2", &SelectedBar, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Bar 3", &SelectedBar, 2);
            ImGui::Separator();

            if (ImGui::BeginTable("QuickSlotsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Slot #", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Target / Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableHeadersRow();

                for (U32 I = 0; I < SlotCount; ++I)
                {
                    const QuickSlotEntry& Slot = Slots[I];
                    if (Slot.BarIndex != (U32)SelectedBar)
                        continue;

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Slot %u", Slot.SlotIndex + 1);

                    ImGui::TableSetColumnIndex(1);
                    if (!Slot.Active)
                    {
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                    }
                    else if (Slot.Type == QuickSlotType::Skill)
                    {
                        ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "SKILL");
                    }
                    else if (Slot.Type == QuickSlotType::Action)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "ACTION");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "ITEM");
                    }

                    ImGui::TableSetColumnIndex(2);
                    if (Slot.Active)
                        ImGui::Text("%s", Slot.Name);
                    else
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[Empty]");

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", Slot.Details);

                    ImGui::TableSetColumnIndex(4);
                    if (Slot.Active)
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "ACTIVE");
                    else
                        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "EMPTY");
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Menu::RenderBuffsWindow()
    {
        const FixedList<BuffInfo, 32>& Buffs = BuffManager::GetBuffs();
        U32 BuffCount = Buffs.GetCount();

        ImGui::SetNextWindowPos(ImVec2(540.0f, 565.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(390.0f, 240.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Buffs & Status Monitor"))
        {
            ImGui::Text("Active Status Effects: %u", BuffCount);
            ImGui::Separator();

            if (ImGui::BeginTable("BuffsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Effect", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Lvl", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableHeadersRow();

                if (BuffCount == 0)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No active status effects");
                    ImGui::TableSetColumnIndex(2); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(3); ImGui::Text("-");
                }
                else
                {
                    for (U32 I = 0; I < BuffCount; ++I)
                    {
                        const BuffInfo& Buff = Buffs[I];
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        if (Buff.IsDebuff)
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "DEBUFF");
                        else
                            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "BUFF");

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", Buff.Name);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%u", Buff.Level);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%us", Buff.DurationSeconds);
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Menu::RenderGroundItemSnaplines()
    {
        if (!SnaplinesEnabled)
            return;

        const FixedList<GroundItem, 128>& Items = GroundItemManager::GetGroundItems();
        U32 ItemCount = Items.GetCount();
        if (ItemCount == 0)
            return;

        ImGuiIO& Io = ImGui::GetIO();
        F32 ScreenW = Io.DisplaySize.x;
        F32 ScreenH = Io.DisplaySize.y;
        if (ScreenW <= 0.0f || ScreenH <= 0.0f)
            return;

        ImVec2 ScreenOrigin(ScreenW * 0.5f, ScreenH);
        ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

        const FixedList<ActiveQuest, 16>& ActiveQuests = QuestManager::GetActiveQuests();

        for (U32 I = 0; I < ItemCount; ++I)
        {
            const GroundItem& Item = Items[I];
            Vector2 ScreenPos;
            if (!Camera::WorldToScreen(Item.Position, ScreenPos, ScreenW, ScreenH))
                continue;

            ImVec2 TargetPos(ScreenPos.X, ScreenPos.Y);

            bool IsQuestItem = false;
            U32 QCur = 0;
            U32 QNeed = 0;

            for (U32 Q = 0; Q < ActiveQuests.GetCount(); ++Q)
            {
                const ActiveQuest& ActQ = ActiveQuests[Q];
                for (U32 O = 0; O < ActQ.ItemObjectiveCount; ++O)
                {
                    const QuestItemObjective& Obj = ActQ.ItemObjectives[O];
                    if (!Obj.Completed && Item.Type == Obj.ItemType && Item.TypeId == Obj.ItemTypeId)
                    {
                        IsQuestItem = true;
                        QCur = Obj.CurrentCount;
                        QNeed = Obj.CountNeeded;
                        break;
                    }
                }
                if (IsQuestItem) break;
            }

            ImU32 LineColor;
            F32 Thickness = 1.5f;

            if (IsQuestItem)
            {
                LineColor = IM_COL32(255, 60, 255, 255); // Glowing Magenta for Quest Drop!
                Thickness = 2.5f;
            }
            else if (Item.Type == 44) // Gold
            {
                LineColor = IM_COL32(255, 215, 0, 220); // Gold yellow
            }
            else if (Item.Distance < 10.0f)
            {
                LineColor = IM_COL32(50, 255, 100, 220); // Bright green
            }
            else if (Item.Distance < 30.0f)
            {
                LineColor = IM_COL32(0, 200, 255, 180); // Cyan
            }
            else
            {
                LineColor = IM_COL32(180, 180, 180, 140); // Silver
            }

            // Tracer line from bottom center of screen to loot world position
            DrawList->AddLine(ScreenOrigin, TargetPos, LineColor, Thickness);

            // Marker dot
            if (IsQuestItem)
            {
                DrawList->AddCircle(TargetPos, 8.0f, LineColor, 14, 2.0f);
                DrawList->AddCircleFilled(TargetPos, 4.0f, LineColor);
            }
            else
            {
                DrawList->AddCircleFilled(TargetPos, 3.5f, LineColor);
            }

            // Item name tag
            char Label[96];
            if (IsQuestItem)
            {
                StringUtils::Format(Label, sizeof(Label), "[QUEST ITEM] %s (%u/%u) (%.1fm)", Item.Name, QCur, QNeed, Item.Distance);
            }
            else
            {
                StringUtils::Format(Label, sizeof(Label), "%s (%.1fm)", Item.Name, Item.Distance);
            }
            DrawList->AddText(ImVec2(TargetPos.x + 6.0f, TargetPos.y - 7.0f), LineColor, Label);
        }
    }

    void Menu::RenderQuestWaypoints()
    {
        if (!QuestWaypointsEnabled)
            return;

        const FixedList<ActiveQuest, 16>& Quests = QuestManager::GetActiveQuests();
        U32 QuestCount = Quests.GetCount();
        if (QuestCount == 0)
            return;

        ImGuiIO& Io = ImGui::GetIO();
        F32 ScreenW = Io.DisplaySize.x;
        F32 ScreenH = Io.DisplaySize.y;
        if (ScreenW <= 0.0f || ScreenH <= 0.0f)
            return;

        ImVec2 ScreenOrigin(ScreenW * 0.5f, ScreenH);
        ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

        const FixedList<MonsterEntity, 128>& Mobs = EntityManager::GetNearbyMonsters();
        U32 MobCount = Mobs.GetCount();

        for (U32 I = 0; I < QuestCount; ++I)
        {
            const ActiveQuest& Q = Quests[I];
            bool HasIncompleteHunt = false;

            // 1. Highlight target quest mobs nearby (direct kill requirement)
            for (U32 O = 0; O < Q.ObjectiveCount; ++O)
            {
                if (Q.Objectives[O].Completed)
                    continue;

                HasIncompleteHunt = true;
                U16 TargetId = Q.Objectives[O].TargetMobId;

                for (U32 M = 0; M < MobCount; ++M)
                {
                    const MonsterEntity& Mob = Mobs[M];
                    if (Mob.Alive && Mob.MobId == TargetId)
                    {
                        Vector2 ScreenPos;
                        if (Camera::WorldToScreen(Mob.Position, ScreenPos, ScreenW, ScreenH))
                        {
                            ImVec2 TargetPos(ScreenPos.X, ScreenPos.Y);
                            ImU32 Color = IM_COL32(255, 140, 0, 240); // Bright Orange for quest mobs

                            DrawList->AddLine(ScreenOrigin, TargetPos, Color, 2.0f);
                            DrawList->AddCircle(TargetPos, 9.0f, Color, 14, 2.0f);
                            DrawList->AddCircleFilled(TargetPos, 4.0f, Color);

                            char Label[96];
                            StringUtils::Format(Label, sizeof(Label), "[QUEST TARGET] %s (%u/%u)",
                                Q.Objectives[O].TargetMobName,
                                Q.Objectives[O].CurrentCount,
                                Q.Objectives[O].CountNeeded);
                            DrawList->AddText(ImVec2(TargetPos.x + 8.0f, TargetPos.y - 8.0f), Color, Label);
                        }
                    }
                }
            }

            // 2. Highlight target mobs that drop required quest items
            for (U32 O = 0; O < Q.ItemObjectiveCount; ++O)
            {
                const QuestItemObjective& Obj = Q.ItemObjectives[O];
                if (Obj.Completed || (Obj.DroppedByMobId == 0 && Obj.DroppedByMobName[0] == '\0'))
                    continue;

                HasIncompleteHunt = true;
                U16 TargetId = Obj.DroppedByMobId;

                for (U32 M = 0; M < MobCount; ++M)
                {
                    const MonsterEntity& Mob = Mobs[M];
                    bool Match = (TargetId > 0 && Mob.MobId == TargetId);
                    if (!Match && Obj.DroppedByMobName[0] != '\0')
                    {
                        Match = StringUtils::ContainsNormalized(Mob.Name, Obj.DroppedByMobName);
                    }

                    if (Mob.Alive && Match)
                    {
                        Vector2 ScreenPos;
                        if (Camera::WorldToScreen(Mob.Position, ScreenPos, ScreenW, ScreenH))
                        {
                            ImVec2 TargetPos(ScreenPos.X, ScreenPos.Y);
                            ImU32 Color = IM_COL32(210, 80, 255, 240); // Bright Purple for item drop mobs

                            DrawList->AddLine(ScreenOrigin, TargetPos, Color, 2.0f);
                            DrawList->AddCircle(TargetPos, 10.0f, Color, 14, 2.0f);
                            DrawList->AddCircleFilled(TargetPos, 4.0f, Color);

                            char Label[128];
                            StringUtils::Format(Label, sizeof(Label), "[QUEST MOB (DROP)] %s (%s %u/%u)",
                                Mob.Name, Obj.ItemName, Obj.CurrentCount, Obj.CountNeeded);
                            DrawList->AddText(ImVec2(TargetPos.x + 8.0f, TargetPos.y - 8.0f), Color, Label);
                        }
                    }
                }
            }
        }

        // 2. Draw destination waypoints for Turn-In NPCs from live game Radar Markers
        const FixedList<QuestMarker, 32>& Markers = QuestManager::GetQuestMarkers();
        U32 MarkerCount = Markers.GetCount();

        for (U32 K = 0; K < MarkerCount; ++K)
        {
            const QuestMarker& Marker = Markers[K];
            if (!Marker.IsTurnIn)
                continue; // Only draw waypoints to turn-in NPCs

            Vector2 ScreenPos;
            if (Camera::WorldToScreen(Marker.Position, ScreenPos, ScreenW, ScreenH))
            {
                ImVec2 TargetPos(ScreenPos.X, ScreenPos.Y);
                ImU32 Color = IM_COL32(255, 215, 0, 240); // Golden yellow for quest turn-in NPC

                DrawList->AddLine(ScreenOrigin, TargetPos, Color, 2.0f);
                DrawList->AddCircle(TargetPos, 12.0f, Color, 16, 2.0f);
                DrawList->AddCircle(TargetPos, 6.0f, Color, 12, 1.5f);
                DrawList->AddCircleFilled(TargetPos, 3.0f, Color);

                char Label[96];
                StringUtils::Format(Label, sizeof(Label), "[QUEST TURN-IN] %s (%.0fm)", Marker.NpcName, Marker.Distance);
                DrawList->AddText(ImVec2(TargetPos.x + 14.0f, TargetPos.y - 7.0f), Color, Label);
            }
        }

        // 3. Draw active auto-walk path if actively navigating
        if (NavigationManager::IsNavigating())
        {
            U32 wpCount = NavigationManager::GetWaypointCount();
            U32 curWpIdx = NavigationManager::GetCurrentWaypointIndex();
            const Vector3* wps = NavigationManager::GetWaypoints();

            ImU32 Color = IM_COL32(0, 255, 255, 255); // Bright cyan for active walk
            ImVec2 lastScreenPt = ScreenOrigin;

            if (wpCount > 0 && curWpIdx < wpCount)
            {
                for (U32 i = curWpIdx; i < wpCount; ++i)
                {
                    Vector2 wpScreen;
                    if (Camera::WorldToScreen(wps[i], wpScreen, ScreenW, ScreenH))
                    {
                        ImVec2 currentPt(wpScreen.X, wpScreen.Y);
                        DrawList->AddLine(lastScreenPt, currentPt, Color, 2.5f);

                        if (i == wpCount - 1)
                        {
                            DrawList->AddCircle(currentPt, 16.0f, Color, 20, 2.5f);
                            DrawList->AddCircleFilled(currentPt, 6.0f, Color);

                            char Label[96];
                            StringUtils::Format(Label, sizeof(Label), ">> [WALKING TO] %s (%.1fm)",
                                NavigationManager::GetTargetName(), NavigationManager::GetRemainingDistance());
                            DrawList->AddText(ImVec2(currentPt.x + 18.0f, currentPt.y - 8.0f), Color, Label);
                        }
                        else
                        {
                            DrawList->AddCircleFilled(currentPt, 4.0f, Color);
                        }

                        lastScreenPt = currentPt;
                    }
                }
            }
            else
            {
                Vector2 ScreenPos;
                if (Camera::WorldToScreen(NavigationManager::GetTargetPosition(), ScreenPos, ScreenW, ScreenH))
                {
                    ImVec2 TargetPos(ScreenPos.X, ScreenPos.Y);
                    DrawList->AddLine(ScreenOrigin, TargetPos, Color, 3.0f);
                    DrawList->AddCircle(TargetPos, 16.0f, Color, 20, 2.5f);
                    DrawList->AddCircleFilled(TargetPos, 6.0f, Color);

                    char Label[96];
                    StringUtils::Format(Label, sizeof(Label), ">> [WALKING TO] %s (%.1fm)",
                        NavigationManager::GetTargetName(), NavigationManager::GetRemainingDistance());
                    DrawList->AddText(ImVec2(TargetPos.x + 18.0f, TargetPos.y - 8.0f), Color, Label);
                }
            }
        }
    }
}
