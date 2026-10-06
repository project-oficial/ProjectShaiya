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
#include "Game/Combat/ComboManager.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/Items/InventoryManager.h"
#include "Game/QuickSlots/QuickSlotManager.h"
#include "Game/Quests/QuestManager.h"
#include "Game/Navigation/NavigationManager.h"
#include "Game/Login/AutoLoginManager.h"
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
        InventoryManager::Update();
        QuickSlotManager::Update();
        QuestManager::Update();
        NavigationManager::Update();
        ComboManager::Update();

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        const FixedList<MonsterEntity, 128>& Monsters = EntityManager::GetNearbyMonsters();
        RiskAssessment Risk = RiskCalculator::Evaluate(Player, Monsters);

        // Control ImGui software cursor visibility based on menu state
        ImGui::GetIO().MouseDrawCursor = WndProcHook::IsMenuOpen();

        // Always-on Hardcore Threat Banner (visible whether menu is toggled or not)
        F32 BannerHeight = 75.0f;
        if (NavigationManager::IsNavigating()) BannerHeight += 20.0f;
        if (ComboManager::IsActive()) BannerHeight += 20.0f;

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400.0f, BannerHeight), ImGuiCond_Always);

        ImGuiWindowFlags BannerFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        if (!WndProcHook::IsMenuOpen())
            BannerFlags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

        if (ImGui::Begin("Shaiya Hardcore Overlay", nullptr, BannerFlags))
        {
            ImVec4 ThreatCol = GetThreatColor(Risk.OverallThreat);
            ImGui::TextColored(ThreatCol, "[RISK: %s]", Risk.Summary);
            ImGui::Text("FPS: %.1f | Hostiles: %u | Quests: %u",
                ImGui::GetIO().Framerate, Risk.NearbyHostilesCount, QuestManager::GetQuestCount());

            if (WndProcHook::IsMenuOpen())
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
                if (ImGui::SmallButton("DESCARREGAR MOD"))
                {
                    WndProcHook::RequestUnload();
                }
                ImGui::PopStyleColor(3);
            }

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

            if (ComboManager::IsActive())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.0f, 1.0f), ">> AUTO-COMBO: ATIVO (Tecla: C)");
                if (WndProcHook::IsMenuOpen())
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("STOP COMBO"))
                    {
                        ComboManager::SetActive(false);
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
        RenderInventoryWindow();
        RenderSkillsWindow();
        RenderQuickSlotsWindow();
        RenderQuestsWindow();
        RenderBuffsWindow();
        RenderAutoComboWindow();
    }

    void Menu::RenderOverviewWindow()
    {
        const PlayerData& Player = EntityManager::GetLocalPlayer();
        const FixedList<MonsterEntity, 128>& Monsters = EntityManager::GetNearbyMonsters();
        RiskAssessment Risk = RiskCalculator::Evaluate(Player, Monsters);

        ImGui::SetNextWindowPos(ImVec2(10.0f, 95.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380.0f, 430.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Local Player & Hardcore Risk"))
        {
            if (Player.Valid)
            {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Character ID: %u | Level: %u", Player.Id, Player.Level);
                ImGui::Text("Position: X: %.1f  Y: %.1f  Z: %.1f", Player.Position.X, Player.Position.Y, Player.Position.Z);
                ImGui::Separator();

                // Health (HP)
                F32 HpPct = Player.GetHpPercentage();
                char HpText[64];
                StringUtils::Format(HpText, sizeof(HpText), "%u / %u (%.0f%%)", Player.CurrentHp, Player.MaxHp, HpPct * 100.0f);

                ImVec4 HpBarColor = (HpPct < 0.35f) ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.1f, 0.8f, 0.2f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, HpBarColor);
                ImGui::Text("Vida (HP):");
                ImGui::ProgressBar(HpPct, ImVec2(-1.0f, 0.0f), HpText);
                ImGui::PopStyleColor();

                // Mana (MP)
                F32 MpPct = Player.GetMpPercentage();
                char MpText[64];
                StringUtils::Format(MpText, sizeof(MpText), "%u / %u (%.0f%%)", Player.CurrentMp, Player.MaxMp, MpPct * 100.0f);

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.5f, 0.95f, 1.0f));
                ImGui::Text("Mana (MP):");
                ImGui::ProgressBar(MpPct, ImVec2(-1.0f, 0.0f), MpText);
                ImGui::PopStyleColor();

                // Stamina (SP)
                F32 SpPct = Player.GetSpPercentage();
                char SpText[64];
                StringUtils::Format(SpText, sizeof(SpText), "%u / %u (%.0f%%)", Player.CurrentSp, Player.MaxSp, SpPct * 100.0f);

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
                ImGui::Text("Estamina (SP):");
                ImGui::ProgressBar(SpPct, ImVec2(-1.0f, 0.0f), SpText);
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

            ImGui::Separator();
            GameState State = AutoLoginManager::GetCurrentGameState();
            ImGui::Text("Game State: %s (%u)", AutoLoginManager::GetGameStateName(State), static_cast<U8>(State));
            ImGui::Text("Auto-Login: %s", AutoLoginManager::GetStatusMessage());
            if (AutoLoginManager::IsRunning())
            {
                if (ImGui::Button("Cancelar Auto-Login", ImVec2(-1.0f, 22.0f)))
                {
                    AutoLoginManager::Stop();
                }
            }
            else
            {
                if (ImGui::Button("Executar Auto-Login Manual", ImVec2(-1.0f, 22.0f)))
                {
                    AutoLoginManager::Start();
                }
            }

            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Descarregar / Ejetar Mod (Unload)", ImVec2(-1.0f, 26.0f)))
            {
                WndProcHook::RequestUnload();
            }
            ImGui::PopStyleColor(3);
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
                    if (Mob.IsQuestTarget)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.35f, 1.0f, 1.0f), "[QUEST] %s", Mob.Name);
                    }
                    else
                    {
                        ImGui::Text("%s", Mob.Name);
                    }

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
            AutoLootConfig& LootCfg = GroundItemManager::GetConfig();
            ImGui::Checkbox("Auto-Loot", &LootCfg.Enabled);
            ImGui::SameLine();
            ImGui::Checkbox("Only My Drops", &LootCfg.OnlyMyDrops);
            ImGui::SameLine();
            ImGui::Checkbox("Auto-Walk", &LootCfg.AutoWalkToLoot);
            ImGui::SameLine();
            ImGui::Checkbox("Tracer Lines", &SnaplinesEnabled);
            ImGui::SameLine();
            ImGui::TextDisabled("| Total: %u", ItemCount);

            if (LootCfg.Enabled)
            {
                ImGui::SetNextItemWidth(110.0f);
                ImGui::SliderFloat("Pickup Radius", &LootCfg.PickupRadius, 1.5f, 5.0f, "%.1fm");
                if (LootCfg.AutoWalkToLoot)
                {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(110.0f);
                    ImGui::SliderFloat("Max Walk Dist", &LootCfg.MaxWalkDistance, 5.0f, 40.0f, "%.0fm");
                }
            }
            ImGui::Separator();

            if (ImGui::BeginTable("GroundItemsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Item Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Pick", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Walk", ImGuiTableColumnFlags_WidthFixed, 40.0f);
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
                    ImGui::TableSetColumnIndex(6); ImGui::Text("-");
                }
                else
                {
                    const FixedList<ActiveQuest, 16>& ActiveQuests = QuestManager::GetActiveQuests();
                    const PlayerData& Player = EntityManager::GetLocalPlayer();

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
                        if (Item.OwnerId == 0)
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Free");
                        else if (Player.Valid && Item.OwnerId == Player.Id)
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Mine");
                        else
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%u", Item.OwnerId);

                        ImGui::TableSetColumnIndex(5);
                        char PickBtn[32];
                        StringUtils::Format(PickBtn, sizeof(PickBtn), "Pick##%u", Item.WorldId);
                        if (ImGui::Button(PickBtn, ImVec2(-1.0f, 0.0f)))
                        {
                            GroundItemManager::PickUp(Item.WorldId);
                        }

                        ImGui::TableSetColumnIndex(6);
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

    void Menu::RenderInventoryWindow()
    {
        const FixedList<InventoryItem, 128>& Items = InventoryManager::GetItems();
        U32 ItemCount = Items.GetCount();

        ImGui::SetNextWindowPos(ImVec2(10.0f, 535.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(480.0f, 250.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Player Inventory"))
        {
            ImGui::Text("Itens no Inventário: %u", ItemCount);
            ImGui::Separator();

            if (ImGui::BeginTable("InventoryTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Bolsa", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Nome", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Qtd", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Tipo", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Consumível", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableHeadersRow();

                for (U32 i = 0; i < ItemCount; ++i)
                {
                    const auto& Item = Items[i];
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Bolsa %u", Item.Bag);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%u", Item.Slot);

                    ImGui::TableSetColumnIndex(2);
                    if (Item.IsConsumable)
                    {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", Item.Name);
                    }
                    else
                    {
                        ImGui::Text("%s", Item.Name);
                    }

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%u", Item.Count);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("[%u-%u]", Item.Type, Item.TypeId);

                    ImGui::TableSetColumnIndex(5);
                    if (Item.IsConsumable)
                    {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Sim");
                    }
                    else
                    {
                        ImGui::TextDisabled("Não");
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
                                char SingleObj[96];
                                if (Q.ItemObjectives[O].DroppedByMobName[0] != '\0')
                                {
                                    StringUtils::Format(SingleObj, sizeof(SingleObj), "%s: %u/%u (%s)%s",
                                        Q.ItemObjectives[O].ItemName,
                                        Q.ItemObjectives[O].CurrentCount,
                                        Q.ItemObjectives[O].CountNeeded,
                                        Q.ItemObjectives[O].DroppedByMobName,
                                        O + 1 < Q.ItemObjectiveCount ? ", " : "");
                                }
                                else
                                {
                                    StringUtils::Format(SingleObj, sizeof(SingleObj), "%s: %u/%u%s",
                                        Q.ItemObjectives[O].ItemName,
                                        Q.ItemObjectives[O].CurrentCount,
                                        Q.ItemObjectives[O].CountNeeded,
                                        O + 1 < Q.ItemObjectiveCount ? ", " : "");
                                }
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
                                Vector3 CachedPos;
                                char CachedName[64] = { 0 };
                                if (QuestManager::GetSavedMobPosition(Q.QuestId, TargetMobId, CachedPos, CachedName, sizeof(CachedName)))
                                {
                                    char BtnLabel[32];
                                    StringUtils::Format(BtnLabel, sizeof(BtnLabel), "To Mob*##Q%u", Q.QuestId);
                                    if (ImGui::Button(BtnLabel, ImVec2(-1.0f, 0.0f)))
                                    {
                                        NavigationManager::WalkTo(CachedPos, CachedName[0] ? CachedName : "Quest Mob Area", 5.0f);
                                    }
                                    if (ImGui::IsItemHovered())
                                    {
                                        ImGui::SetTooltip("Saved spawn area: %s (%.0f, %.0f)",
                                            CachedName[0] ? CachedName : "Mob Area", CachedPos.X, CachedPos.Z);
                                    }
                                }
                                else
                                {
                                    ImGui::TextDisabled("No Mob");
                                    if (ImGui::IsItemHovered())
                                    {
                                        ImGui::SetTooltip("Waiting for mob to appear nearby to auto-record position");
                                    }
                                }
                            }
                        }
                        else
                        {
                            bool FoundTurnIn = false;
                            Vector3 TurnInPos;
                            char NpcName[64] = "Quest NPC";

                            // Priority 1: Direct destination from RadarArray for this specific quest
                            if (Q.HasDestination)
                            {
                                TurnInPos = Q.DestinationPos;
                                StringUtils::Copy(NpcName, Q.DestinationName, sizeof(NpcName));
                                FoundTurnIn = true;
                            }

                            // Priority 2: Match from active QuestMarkers
                            const FixedList<QuestMarker, 32>& Markers = QuestManager::GetQuestMarkers();
                            for (U32 K = 0; K < Markers.GetCount(); ++K)
                            {
                                const QuestMarker& M = Markers[K];
                                if (M.IsTurnIn)
                                {
                                    // Match either by QuestId or proximity to resolved DestinationPos
                                    if (M.QuestId == Q.QuestId || (FoundTurnIn && M.Position.DistanceTo(TurnInPos) < 5.0f))
                                    {
                                        TurnInPos = M.Position;
                                        StringUtils::Copy(NpcName, M.NpcName, sizeof(NpcName));
                                        FoundTurnIn = true;
                                        break;
                                    }
                                }
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
        ImGui::SetNextWindowSize(ImVec2(500.0f, 260.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Learned Skills Tracker"))
        {
            ImGui::Checkbox("Somente Aprendidas", &FilterOnlyLearned);
            ImGui::SameLine();
            ImGui::TextDisabled("(%u/%u)", LearnedCount, SkillCount);
            ImGui::SameLine();

            AutoBuffConfig& BuffCfg = BuffManager::GetConfig();
            ImGui::Checkbox("Auto-Buff", &BuffCfg.Enabled);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70.0f);
            int thresh = static_cast<int>(BuffCfg.RecastThresholdSeconds);
            if (ImGui::SliderInt("Recast <= s", &thresh, 1, 30))
            {
                BuffCfg.RecastThresholdSeconds = static_cast<U32>(thresh);
            }

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

            if (ImGui::BeginTable("SkillsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Habilidade", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Nv", ImGuiTableColumnFlags_WidthFixed, 25.0f);
                ImGui::TableSetupColumn("Recarga", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Estado", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Auto", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Ação", ImGuiTableColumnFlags_WidthFixed, 50.0f);
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
                    if (Skill.IsPassive)
                    {
                        ImGui::TextDisabled("Passiva");
                    }
                    else if (Skill.CooldownDuration > 0.0f)
                    {
                        ImGui::Text("%.0fs", Skill.CooldownDuration);
                    }
                    else
                    {
                        ImGui::TextDisabled("0s");
                    }

                    ImGui::TableSetColumnIndex(4);
                    if (!Skill.IsLearned)
                    {
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "ARVORE");
                    }
                    else if (Skill.IsPassive)
                    {
                        ImGui::TextDisabled("PASSIVA");
                    }
                    else if (Skill.IsReady)
                    {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "PRONTA");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.1fs", Skill.CooldownRemaining);
                    }

                    ImGui::TableSetColumnIndex(5);
                    if (Skill.IsLearned && !Skill.IsPassive)
                    {
                        bool isAuto = BuffManager::IsAutoBuff(Skill.SkillId);
                        char CheckId[32];
                        StringUtils::Format(CheckId, sizeof(CheckId), "##AB%u", Skill.SkillId);
                        if (ImGui::Checkbox(CheckId, &isAuto))
                        {
                            BuffManager::SetAutoBuff(Skill.SkillId, isAuto);
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip(isAuto ? "Auto-Buff ATIVO (recast automático ao expirar)" : "Marcar para Auto-Buff");
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("-");
                    }

                    ImGui::TableSetColumnIndex(6);
                    if (!Skill.IsLearned || Skill.IsPassive)
                    {
                        ImGui::TextDisabled("-");
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

    void Menu::RenderAutoComboWindow()
    {
        ComboConfig& Cfg = ComboManager::GetConfig();
        const FixedList<ComboEntry, 16>& Sequence = ComboManager::GetComboSequence();
        U32 SeqCount = Sequence.GetCount();

        ImGui::SetNextWindowPos(ImVec2(520.0f, 335.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(450.0f, 260.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Auto-Combo / Skill Rotation"))
        {
            ImGui::Checkbox("Habilitar", &Cfg.Enabled);
            ImGui::SameLine();
            if (Cfg.Active)
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[ATIVO]");
                ImGui::SameLine();
                if (ImGui::SmallButton("Parar (C)"))
                {
                    ComboManager::SetActive(false);
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[INATIVO]");
                ImGui::SameLine();
                if (ImGui::SmallButton("Iniciar (C)"))
                {
                    ComboManager::SetActive(true);
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Segurar Tecla", &Cfg.HoldKeyMode);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Marcado: combe enquanto segurar a tecla 'C'.\nDesmarcado: aperte 'C' para ligar/desligar.");
            }

            ImGui::Checkbox("Auto-Target Próximo Mob", &Cfg.AutoTargetNext);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Marcado: após matar o monstro, seleciona e ataca automaticamente o próximo.\nDesmarcado (standalone): ataca apenas o alvo selecionado manualmente.");
            }

            if (Cfg.AutoTargetNext)
            {
                ImGui::SameLine();
                int filterMode = static_cast<int>(Cfg.TargetFilter);
                ImGui::RadioButton("Todos os Mobs", &filterMode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Apenas Mobs de Quest", &filterMode, 1);
                Cfg.TargetFilter = static_cast<TargetFilterMode>(filterMode);

                ImGui::SetNextItemWidth(100.0f);
                ImGui::SliderFloat("Raio de Busca", &Cfg.MaxTargetRange, 10.0f, 45.0f, "%.0fm");
                ImGui::SameLine();
            }

            ImGui::SetNextItemWidth(90.0f);
            int delayMs = static_cast<int>(Cfg.CastDelayMs);
            if (ImGui::SliderInt("Delay (ms)", &delayMs, 600, 2500))
            {
                Cfg.CastDelayMs = static_cast<U32>(delayMs);
            }

            U32 TargetWorldId = SkillManager::GetSelectedTargetWorldId();
            if (TargetWorldId != 0)
            {
                const char* TargetName = "Monstro";
                U32 CurHp = 0, MaxHp = 0;
                const auto& Mobs = EntityManager::GetNearbyMonsters();
                for (U32 m = 0; m < Mobs.GetCount(); ++m)
                {
                    if (Mobs[m].WorldId == TargetWorldId)
                    {
                        TargetName = Mobs[m].Name;
                        CurHp = Mobs[m].CurrentHp;
                        MaxHp = Mobs[m].MaxHp;
                        break;
                    }
                }
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Alvo: %s (%u/%u)", TargetName, CurHp, MaxHp);
            }
            else
            {
                ImGui::TextDisabled("Alvo: Nenhum (selecione um monstro vivo para atacar)");
            }

            ImGui::Separator();

            // Skill selector to add to rotation sequence
            const FixedList<SkillInfo, 64>& Skills = SkillManager::GetSkills();
            U32 SkillCount = Skills.GetCount();

            static int SelectedSkillIdx = 0;
            char PreviewText[128] = "Selecione uma habilidade...";
            if (SelectedSkillIdx >= 0 && SelectedSkillIdx < (int)SkillCount)
            {
                StringUtils::Format(PreviewText, sizeof(PreviewText), "%s (ID: %u, Slot: %u)",
                    Skills[SelectedSkillIdx].Name, Skills[SelectedSkillIdx].SkillId, Skills[SelectedSkillIdx].LearnedSlot);
            }

            ImGui::SetNextItemWidth(250.0f);
            if (ImGui::BeginCombo("##AddComboSkill", PreviewText))
            {
                for (U32 i = 0; i < SkillCount; ++i)
                {
                    const SkillInfo& Skill = Skills[i];
                    if (!Skill.IsLearned || Skill.IsPassive)
                        continue;

                    char ItemLabel[128];
                    StringUtils::Format(ItemLabel, sizeof(ItemLabel), "%s (ID: %u, Slot: %u)##Cmb%u",
                        Skill.Name, Skill.SkillId, Skill.LearnedSlot, i);

                    bool isSelected = (SelectedSkillIdx == (int)i);
                    if (ImGui::Selectable(ItemLabel, isSelected))
                    {
                        SelectedSkillIdx = static_cast<int>(i);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("+ Adicionar"))
            {
                if (SelectedSkillIdx >= 0 && SelectedSkillIdx < (int)SkillCount)
                {
                    const SkillInfo& S = Skills[SelectedSkillIdx];
                    if (S.IsLearned && !S.IsPassive)
                    {
                        ComboManager::AddSkillToSequence(S.SkillId, S.Name);
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Limpar"))
            {
                ComboManager::ClearSequence();
            }

            // Sequence Table
            if (ImGui::BeginTable("ComboSeqTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 25.0f);
                ImGui::TableSetupColumn("Habilidade", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Recarga", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Ordem", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Ação", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableHeadersRow();

                if (SeqCount == 0)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Nenhuma habilidade no combo");
                    ImGui::TableSetColumnIndex(2); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(3); ImGui::Text("-");
                    ImGui::TableSetColumnIndex(4); ImGui::Text("-");
                }
                else
                {
                    for (U32 i = 0; i < SeqCount; ++i)
                    {
                        const ComboEntry& Entry = Sequence[i];
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u", i + 1);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", Entry.Name);

                        ImGui::TableSetColumnIndex(2);
                        F32 cdRem = 0.0f;
                        bool ready = true;
                        for (U32 s = 0; s < SkillCount; ++s)
                        {
                            if (Skills[s].SkillId == Entry.SkillId)
                            {
                                cdRem = Skills[s].CooldownRemaining;
                                ready = Skills[s].IsReady;
                                break;
                            }
                        }

                        if (ready)
                        {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "PRONTA");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%.1fs", cdRem);
                        }

                        ImGui::TableSetColumnIndex(3);
                        char UpBtn[32], DownBtn[32];
                        StringUtils::Format(UpBtn, sizeof(UpBtn), "^##U%u", i);
                        StringUtils::Format(DownBtn, sizeof(DownBtn), "v##D%u", i);

                        if (i == 0) ImGui::BeginDisabled();
                        if (ImGui::SmallButton(UpBtn))
                        {
                            ComboManager::MoveSkillUp(i);
                        }
                        if (i == 0) ImGui::EndDisabled();

                        ImGui::SameLine();
                        if (i + 1 >= SeqCount) ImGui::BeginDisabled();
                        if (ImGui::SmallButton(DownBtn))
                        {
                            ComboManager::MoveSkillDown(i);
                        }
                        if (i + 1 >= SeqCount) ImGui::EndDisabled();

                        ImGui::TableSetColumnIndex(4);
                        char DelBtn[32];
                        StringUtils::Format(DelBtn, sizeof(DelBtn), "X##R%u", i);
                        if (ImGui::SmallButton(DelBtn))
                        {
                            ComboManager::RemoveSkillFromSequence(i);
                        }
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
                        if (Buff.DurationSeconds >= 60)
                        {
                            U32 Min = Buff.DurationSeconds / 60;
                            U32 Sec = Buff.DurationSeconds % 60;
                            ImGui::Text("%um %02us", Min, Sec);
                        }
                        else
                        {
                            ImGui::Text("%us", Buff.DurationSeconds);
                        }
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

        // 1b. Draw waypoints for saved quest mob areas when live mobs are out of render range
        for (U32 Q = 0; Q < QuestManager::GetQuestCount(); ++Q)
        {
            const ActiveQuest& Quest = QuestManager::GetActiveQuests()[Q];
            U16 TargetMid = 0;
            const char* MobLabel = "Quest Mob";
            for (U32 O = 0; O < Quest.ObjectiveCount; ++O)
            {
                if (!Quest.Objectives[O].Completed && Quest.Objectives[O].TargetMobId > 0)
                {
                    TargetMid = Quest.Objectives[O].TargetMobId;
                    MobLabel = Quest.Objectives[O].TargetMobName;
                    break;
                }
            }
            if (TargetMid == 0)
            {
                for (U32 I = 0; I < Quest.ItemObjectiveCount; ++I)
                {
                    if (!Quest.ItemObjectives[I].Completed && Quest.ItemObjectives[I].DroppedByMobId > 0)
                    {
                        TargetMid = Quest.ItemObjectives[I].DroppedByMobId;
                        MobLabel = Quest.ItemObjectives[I].DroppedByMobName;
                        break;
                    }
                }
            }

            if (TargetMid > 0)
            {
                bool HasLiveMob = false;
                for (U32 M = 0; M < MobCount; ++M)
                {
                    if (Mobs[M].Alive && Mobs[M].MobId == TargetMid)
                    {
                        HasLiveMob = true;
                        break;
                    }
                }

                if (!HasLiveMob)
                {
                    Vector3 SavedPos;
                    char SavedName[64] = { 0 };
                    if (QuestManager::GetSavedMobPosition(Quest.QuestId, TargetMid, SavedPos, SavedName, sizeof(SavedName)))
                    {
                        Vector2 ScreenPos;
                        if (Camera::WorldToScreen(SavedPos, ScreenPos, ScreenW, ScreenH))
                        {
                            ImVec2 TargetPos(ScreenPos.X, ScreenPos.Y);
                            ImU32 Color = IM_COL32(255, 140, 0, 230); // Orange for saved mob spawn area

                            DrawList->AddLine(ScreenOrigin, TargetPos, Color, 1.8f);
                            DrawList->AddCircle(TargetPos, 12.0f, Color, 16, 2.0f);
                            DrawList->AddCircle(TargetPos, 6.0f, Color, 12, 1.5f);
                            DrawList->AddCircleFilled(TargetPos, 3.0f, Color);

                            const PlayerData& LocalPlayer = EntityManager::GetLocalPlayer();
                            char Label[128];
                            F32 Dist = LocalPlayer.Valid ? SavedPos.DistanceTo(LocalPlayer.Position) : 0.0f;
                            StringUtils::Format(Label, sizeof(Label), "[MOB SPAWN] %s (%.0fm)",
                                SavedName[0] ? SavedName : MobLabel, Dist);
                            DrawList->AddText(ImVec2(TargetPos.x + 14.0f, TargetPos.y - 7.0f), Color, Label);
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
