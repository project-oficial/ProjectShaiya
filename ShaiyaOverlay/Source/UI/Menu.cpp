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
#include "Game/Combat/HealManager.h"
#include "Game/Bot/GrindBot.h"
#include "Game/Visuals/SkinChanger.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/Items/InventoryManager.h"
#include "Game/QuickSlots/QuickSlotManager.h"
#include "Game/Quests/QuestManager.h"
#include "Game/Navigation/NavigationManager.h"
#include "Game/Navigation/WaypointManager.h"
#include "Game/Login/AutoLoginManager.h"
#include "Core/MCPTool/MCPBridge.h"
#include "Interface/Blade/blade_ui.hpp"
#include "Interface/Blade/BladeBridge.h"
#include "Config/Settings.h"

namespace ShaiyaOverlay
{
    bool Menu::SnaplinesEnabled = true;
    bool Menu::QuestWaypointsEnabled = true;

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
        WaypointManager::Update();
        ComboManager::Update();
        HealManager::Update();
        GrindBot::Update();
        SkinChanger::Update();

        // Control ImGui software cursor visibility based on menu state
        Blade::S.menu_open = WndProcHook::IsMenuOpen();
        ImGui::GetIO().MouseDrawCursor = Blade::S.menu_open;

        // Update session user for Blade UI
        const auto& localPlayer = EntityManager::GetLocalPlayer();
        if (localPlayer.Valid)
        {
            char sessionUser[64];
            snprintf(sessionUser, sizeof(sessionUser), "%s (Lv.%u)", "Player", localPlayer.Level);
            BladeBridge::SetSession(sessionUser, "Premium VIP");
        }

        // Render faithful Blade interface & HUDs
        BladeBridge::Render();

        // 3D World to Screen snaplines & ESP
        if (Settings::Get().Visuals.Loot && Settings::Get().Visuals.LootSnaplines)
            RenderGroundItemSnaplines();

        if (Settings::Get().Visuals.QuestWaypoints)
            RenderQuestWaypoints();

        if (Settings::Get().Visuals.MonsterEsp)
            RenderMonsterEsp();
    }

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

    void Menu::RenderMainWindow()
    {
        ImGui::SetNextWindowPos(ImVec2(20.0f, 90.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(740.0f, 560.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Shaiya Assistant - Control Panel", nullptr, ImGuiWindowFlags_None))
        {
            if (ImGui::BeginTabBar("MainControlTabBar", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Status"))
                {
                    RenderStatusTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Auto-Combo"))
                {
                    RenderAutoComboTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Auto-Heal"))
                {
                    RenderAutoHealTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Auto-Loot"))
                {
                    RenderAutoLootTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Auto-Buff"))
                {
                    RenderAutoBuffTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Grind Bot"))
                {
                    RenderGrindBotTab();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void Menu::RenderStatusTab()
    {
        const PlayerData& Player = EntityManager::GetLocalPlayer();
        const FixedList<MonsterEntity, 128>& Monsters = EntityManager::GetNearbyMonsters();
        RiskAssessment Risk = RiskCalculator::Evaluate(Player, Monsters);

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Player Status");
        ImGui::Separator();

        if (Player.Valid)
        {
            ImGui::Text("Character ID: %u | Level: %u", Player.Id, Player.Level);
            ImGui::Text("Position: X: %.1f | Y: %.1f | Z: %.1f", Player.Position.X, Player.Position.Y, Player.Position.Z);
            ImGui::Spacing();

            // HP Bar
            F32 HpPct = Player.GetHpPercentage();
            char HpText[64];
            StringUtils::Format(HpText, sizeof(HpText), "%u / %u (%.0f%%)", Player.CurrentHp, Player.MaxHp, HpPct * 100.0f);
            ImVec4 HpBarColor = (HpPct < 0.35f) ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.1f, 0.8f, 0.2f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, HpBarColor);
            ImGui::Text("Health (HP):");
            ImGui::ProgressBar(HpPct, ImVec2(-1.0f, 0.0f), HpText);
            ImGui::PopStyleColor();

            // MP Bar
            F32 MpPct = Player.GetMpPercentage();
            char MpText[64];
            StringUtils::Format(MpText, sizeof(MpText), "%u / %u (%.0f%%)", Player.CurrentMp, Player.MaxMp, MpPct * 100.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.5f, 0.95f, 1.0f));
            ImGui::Text("Mana (MP):");
            ImGui::ProgressBar(MpPct, ImVec2(-1.0f, 0.0f), MpText);
            ImGui::PopStyleColor();

            // SP Bar
            F32 SpPct = Player.GetSpPercentage();
            char SpText[64];
            StringUtils::Format(SpText, sizeof(SpText), "%u / %u (%.0f%%)", Player.CurrentSp, Player.MaxSp, SpPct * 100.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
            ImGui::Text("Stamina (SP):");
            ImGui::ProgressBar(SpPct, ImVec2(-1.0f, 0.0f), SpText);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Waiting for character data in memory...");
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Hardcore Threat & Environment");
        ImGui::Text("Threat Assessment: ");
        ImGui::SameLine();
        ImGui::TextColored(GetThreatColor(Risk.OverallThreat), "%s", Risk.Summary);

        char ScoreText[32];
        StringUtils::Format(ScoreText, sizeof(ScoreText), "Threat Score: %u / 100", Risk.ThreatScore);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, GetThreatColor(Risk.OverallThreat));
        ImGui::ProgressBar((F32)Risk.ThreatScore / 100.0f, ImVec2(-1.0f, 0.0f), ScoreText);
        ImGui::PopStyleColor();

        ImGui::Text("Hostiles Nearby: %u | Hostiles Close (<12m): %u", Risk.NearbyHostilesCount, Risk.CloseHostilesCount);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Visual Overlays (ESP)");
        ImGui::Checkbox("Draw Ground Item Snaplines", &SnaplinesEnabled);
        ImGui::SameLine();
        ImGui::Checkbox("Draw Quest Waypoint Lines", &QuestWaypointsEnabled);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "System & Login Actions");
        GameState State = AutoLoginManager::GetCurrentGameState();
        ImGui::Text("Game State: %s (%u) | Auto-Login Status: %s",
            AutoLoginManager::GetGameStateName(State), static_cast<U8>(State), AutoLoginManager::GetStatusMessage());

#ifdef TEST_MODE
        if (AutoLoginManager::IsRunning())
        {
            if (ImGui::Button("Cancel Auto-Login"))
                AutoLoginManager::Stop();
        }
        else
        {
            if (ImGui::Button("Run Manual Auto-Login"))
                AutoLoginManager::Start();
        }
#endif

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Unload Mod DLL (END)"))
        {
            WndProcHook::RequestUnload();
        }
        ImGui::PopStyleColor(3);
    }

    void Menu::RenderAutoComboTab()
    {
        ComboConfig& Cfg = ComboManager::GetConfig();
        const FixedList<ComboEntry, 16>& Sequence = ComboManager::GetComboSequence();
        const FixedList<SkillInfo, 64>& Skills = SkillManager::GetSkills();
        U32 SequenceCount = Sequence.GetCount();

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Auto-Combo Execution Controls");
        ImGui::Separator();

        if (ImGui::Checkbox("Enable Auto-Combo Rotation", &Cfg.Enabled))
            ComboManager::SaveConfig();

        ImGui::SameLine();
        bool active = Cfg.Active;
        char activeLabel[64];
        StringUtils::Format(activeLabel, sizeof(activeLabel), "Active (Hotkey: [%c])", static_cast<char>(Cfg.Hotkey));
        if (ImGui::Checkbox(activeLabel, &active))
        {
            ComboManager::SetActive(active);
        }

        if (ImGui::Checkbox("Hold Key Mode (Executes only while holding hotkey)", &Cfg.HoldKeyMode))
            ComboManager::SaveConfig();

        int delay = static_cast<int>(Cfg.CastDelayMs);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::SliderInt("Cast Delay (ms)", &delay, 400, 2500))
        {
            Cfg.CastDelayMs = static_cast<U32>(delay);
            ComboManager::SaveConfig();
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Target Switching Options");

        if (ImGui::Checkbox("Auto-Target Next Monster on Kill", &Cfg.AutoTargetNext))
            ComboManager::SaveConfig();

        const char* filterNames[] = { "All Monsters", "Quest Monsters Only" };
        int currentFilter = static_cast<int>(Cfg.TargetFilter);
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::Combo("Target Filter", &currentFilter, filterNames, 2))
        {
            Cfg.TargetFilter = static_cast<TargetFilterMode>(currentFilter);
            ComboManager::SaveConfig();
        }

        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::SliderFloat("Search Range (m)", &Cfg.MaxTargetRange, 10.0f, 45.0f, "%.1fm"))
            ComboManager::SaveConfig();

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Combo Rotation Sequence (%u/16 skills)", SequenceCount);

        if (ImGui::Button("Clear Sequence"))
            ComboManager::ClearSequence();

        if (ImGui::BeginTable("ComboSequenceTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 140.0f)))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 35.0f);
            ImGui::TableSetupColumn("Skill Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableHeadersRow();

            for (U32 i = 0; i < SequenceCount; ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", i + 1);

                ImGui::TableSetColumnIndex(1);
                const char* displayName = Sequence[i].Name;
                if (displayName[0] == '\0' || strncmp(displayName, "Skill #", 7) == 0)
                {
                    for (U32 s = 0; s < Skills.GetCount(); ++s)
                    {
                        if (Skills[s].SkillId == Sequence[i].SkillId && Skills[s].Name[0] != '\0')
                        {
                            displayName = Skills[s].Name;
                            break;
                        }
                    }
                }
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", displayName);

                ImGui::TableSetColumnIndex(2);
                char btnUp[32], btnDown[32], btnDel[32];
                StringUtils::Format(btnUp, sizeof(btnUp), "Up##%u", i);
                StringUtils::Format(btnDown, sizeof(btnDown), "Down##%u", i);
                StringUtils::Format(btnDel, sizeof(btnDel), "Del##%u", i);

                if (i > 0)
                {
                    if (ImGui::SmallButton(btnUp)) ComboManager::MoveSkillUp(i);
                    ImGui::SameLine();
                }
                if (i + 1 < SequenceCount)
                {
                    if (ImGui::SmallButton(btnDown)) ComboManager::MoveSkillDown(i);
                    ImGui::SameLine();
                }
                if (ImGui::SmallButton(btnDel))
                {
                    ComboManager::RemoveSkillFromSequence(i);
                }
            }
            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Learned Offensive Skills (Click to Add)");

        if (ImGui::BeginTable("AvailableSkillsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 140.0f)))
        {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (U32 i = 0; i < Skills.GetCount(); ++i)
            {
                const auto& S = Skills[i];
                if (!S.IsLearned || S.IsPassive || S.TargetType != 3) continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", S.SkillId);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", S.Name);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Lv.%u", S.Level);

                ImGui::TableSetColumnIndex(3);
                char btnAdd[32];
                StringUtils::Format(btnAdd, sizeof(btnAdd), "Add##%u", S.SkillId);
                if (ImGui::SmallButton(btnAdd))
                {
                    ComboManager::AddSkillToSequence(S.SkillId, S.Name);
                }
            }
            ImGui::EndTable();
        }
    }

    void Menu::RenderAutoHealTab()
    {
        AutoHealConfig& Cfg = HealManager::GetConfig();

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Auto-Heal & Consumable Settings");
        ImGui::Separator();

        if (ImGui::Checkbox("Enable Auto-Heal System", &Cfg.Enabled))
            HealManager::SaveConfig();

        if (HealManager::GetLastHealAction()[0] != '\0')
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), ">> %s", HealManager::GetLastHealAction());
        }

        int cd = static_cast<int>(Cfg.PotionCooldownMs);
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderInt("Potion Cooldown (ms)", &cd, 300, 3000))
        {
            Cfg.PotionCooldownMs = static_cast<U32>(cd);
            HealManager::SaveConfig();
        }

        ImGui::Separator();
        // 1. Auto-HP
        if (ImGui::Checkbox("Auto-HP (Health)", &Cfg.AutoHpEnabled))
            HealManager::SaveConfig();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::SliderFloat("Use HP <= %##hp", &Cfg.HpThresholdPercent, 10.0f, 95.0f, "%.0f%%"))
            HealManager::SaveConfig();

        FixedList<InventoryItem, 16> hpItems;
        HealManager::GetAvailableHpItems(hpItems);

        char hpPreview[64] = "Automatic (Best Item)";
        if (strcmp(Cfg.SelectedHpItem, "Auto") != 0 && Cfg.SelectedHpItem[0] != '\0')
            StringUtils::Copy(hpPreview, Cfg.SelectedHpItem, sizeof(hpPreview));

        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::BeginCombo("HP Potion Item", hpPreview))
        {
            bool isAuto = (strcmp(Cfg.SelectedHpItem, "Auto") == 0);
            if (ImGui::Selectable("Automatic (Best Item)", isAuto))
            {
                StringUtils::Copy(Cfg.SelectedHpItem, "Auto", sizeof(Cfg.SelectedHpItem));
                HealManager::SaveConfig();
            }
            for (U32 i = 0; i < hpItems.GetCount(); ++i)
            {
                char label[96];
                StringUtils::Format(label, sizeof(label), "%s (+%u HP) [%ux]##%u",
                    hpItems[i].Name, hpItems[i].HpRecovery, hpItems[i].Count, i);
                bool sel = (strcmp(Cfg.SelectedHpItem, hpItems[i].Name) == 0);
                if (ImGui::Selectable(label, sel))
                {
                    StringUtils::Copy(Cfg.SelectedHpItem, hpItems[i].Name, sizeof(Cfg.SelectedHpItem));
                    HealManager::SaveConfig();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Test HP Now"))
            HealManager::TestHealNow("HP");

        InventoryItem bestHp = { 0 };
        if (HealManager::FindBestHpItem(&bestHp))
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "  -> Selected: %s (%ux in stock) [Bag %u, Slot %u] (+%u HP)",
                bestHp.Name, bestHp.Count, bestHp.Bag, bestHp.Slot, bestHp.HpRecovery);
        else
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "  -> No HP recovery item found in inventory!");

        ImGui::Separator();
        // 2. Auto-MP
        if (ImGui::Checkbox("Auto-MP (Mana)", &Cfg.AutoMpEnabled))
            HealManager::SaveConfig();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::SliderFloat("Use MP <= %##mp", &Cfg.MpThresholdPercent, 10.0f, 95.0f, "%.0f%%"))
            HealManager::SaveConfig();

        FixedList<InventoryItem, 16> mpItems;
        HealManager::GetAvailableMpItems(mpItems);

        char mpPreview[64] = "Automatic (Best Item)";
        if (strcmp(Cfg.SelectedMpItem, "Auto") != 0 && Cfg.SelectedMpItem[0] != '\0')
            StringUtils::Copy(mpPreview, Cfg.SelectedMpItem, sizeof(mpPreview));

        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::BeginCombo("MP Potion Item", mpPreview))
        {
            bool isAuto = (strcmp(Cfg.SelectedMpItem, "Auto") == 0);
            if (ImGui::Selectable("Automatic (Best Item)", isAuto))
            {
                StringUtils::Copy(Cfg.SelectedMpItem, "Auto", sizeof(Cfg.SelectedMpItem));
                HealManager::SaveConfig();
            }
            for (U32 i = 0; i < mpItems.GetCount(); ++i)
            {
                char label[96];
                StringUtils::Format(label, sizeof(label), "%s (+%u MP) [%ux]##%u",
                    mpItems[i].Name, mpItems[i].MpRecovery, mpItems[i].Count, i);
                bool sel = (strcmp(Cfg.SelectedMpItem, mpItems[i].Name) == 0);
                if (ImGui::Selectable(label, sel))
                {
                    StringUtils::Copy(Cfg.SelectedMpItem, mpItems[i].Name, sizeof(Cfg.SelectedMpItem));
                    HealManager::SaveConfig();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Test MP Now"))
            HealManager::TestHealNow("MP");

        InventoryItem bestMp = { 0 };
        if (HealManager::FindBestMpItem(&bestMp))
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "  -> Selected: %s (%ux in stock) [Bag %u, Slot %u] (+%u MP)",
                bestMp.Name, bestMp.Count, bestMp.Bag, bestMp.Slot, bestMp.MpRecovery);
        else
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "  -> No MP recovery item found in inventory!");

        ImGui::Separator();
        // 3. Auto-SP
        if (ImGui::Checkbox("Auto-SP (Stamina)", &Cfg.AutoSpEnabled))
            HealManager::SaveConfig();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::SliderFloat("Use SP <= %##sp", &Cfg.SpThresholdPercent, 10.0f, 95.0f, "%.0f%%"))
            HealManager::SaveConfig();

        FixedList<InventoryItem, 16> spItems;
        HealManager::GetAvailableSpItems(spItems);

        char spPreview[64] = "Automatic (Best Item)";
        if (strcmp(Cfg.SelectedSpItem, "Auto") != 0 && Cfg.SelectedSpItem[0] != '\0')
            StringUtils::Copy(spPreview, Cfg.SelectedSpItem, sizeof(spPreview));

        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::BeginCombo("SP Potion Item", spPreview))
        {
            bool isAuto = (strcmp(Cfg.SelectedSpItem, "Auto") == 0);
            if (ImGui::Selectable("Automatic (Best Item)", isAuto))
            {
                StringUtils::Copy(Cfg.SelectedSpItem, "Auto", sizeof(Cfg.SelectedSpItem));
                HealManager::SaveConfig();
            }
            for (U32 i = 0; i < spItems.GetCount(); ++i)
            {
                char label[96];
                StringUtils::Format(label, sizeof(label), "%s (+%u SP) [%ux]##%u",
                    spItems[i].Name, spItems[i].SpRecovery, spItems[i].Count, i);
                bool sel = (strcmp(Cfg.SelectedSpItem, spItems[i].Name) == 0);
                if (ImGui::Selectable(label, sel))
                {
                    StringUtils::Copy(Cfg.SelectedSpItem, spItems[i].Name, sizeof(Cfg.SelectedSpItem));
                    HealManager::SaveConfig();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Test SP Now"))
            HealManager::TestHealNow("SP");

        InventoryItem bestSp = { 0 };
        if (HealManager::FindBestSpItem(&bestSp))
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "  -> Selected: %s (%ux in stock) [Bag %u, Slot %u] (+%u SP)",
                bestSp.Name, bestSp.Count, bestSp.Bag, bestSp.Slot, bestSp.SpRecovery);
        else
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "  -> No SP recovery item found in inventory!");
    }

    void Menu::RenderAutoLootTab()
    {
        AutoLootConfig& Cfg = GroundItemManager::GetConfig();
        const FixedList<GroundItem, 128>& Items = GroundItemManager::GetGroundItems();
        U32 ItemCount = Items.GetCount();

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Auto-Loot Configuration");
        ImGui::Separator();

        ImGui::Checkbox("Enable Auto-Loot", &Cfg.Enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Only My Drops (Free or Player Owned)", &Cfg.OnlyMyDrops);

        ImGui::Checkbox("Auto-Walk to Drops", &Cfg.AutoWalkToLoot);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::SliderFloat("Pickup Radius (m)", &Cfg.PickupRadius, 1.0f, 35.0f, "%.1fm");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Nearby Ground Items (%u)", ItemCount);

        if (ImGui::BeginTable("GroundItemsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 320.0f)))
        {
            ImGui::TableSetupColumn("Item Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("Distance", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableHeadersRow();

            for (U32 i = 0; i < ItemCount; ++i)
            {
                const auto& item = Items[i];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", item.Name);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", item.Count);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.1fm", item.Distance);

                ImGui::TableSetColumnIndex(3);
                if (item.OwnerId == 0)
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Free");
                else if (item.OwnerId == EntityManager::GetLocalPlayer().Id)
                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Mine");
                else
                    ImGui::TextDisabled("Other (%u)", item.OwnerId);

                ImGui::TableSetColumnIndex(4);
                char btnPick[32], btnWalk[32];
                StringUtils::Format(btnPick, sizeof(btnPick), "Pick##%u", item.WorldId);
                StringUtils::Format(btnWalk, sizeof(btnWalk), "Walk##%u", item.WorldId);

                if (ImGui::SmallButton(btnPick))
                {
                    GroundItemManager::PickUp(item.WorldId);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(btnWalk))
                {
                    NavigationManager::WalkTo(item.Position, item.Name, 1.5f);
                }
            }
            ImGui::EndTable();
        }
    }

    void Menu::RenderAutoBuffTab()
    {
        AutoBuffConfig& Cfg = BuffManager::GetConfig();
        const FixedList<SkillInfo, 64>& Skills = SkillManager::GetSkills();

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Auto-Buff Configuration");
        ImGui::Separator();

        ImGui::Checkbox("Enable Auto-Buff System", &Cfg.Enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        int threshold = static_cast<int>(Cfg.RecastThresholdSeconds);
        if (ImGui::SliderInt("Recast Threshold (Sec)", &threshold, 1, 30))
        {
            Cfg.RecastThresholdSeconds = static_cast<U32>(threshold);
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Learned Self-Buffs (Auto-Recast List)");

        if (ImGui::BeginTable("BuffsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 320.0f)))
        {
            ImGui::TableSetupColumn("Skill Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("Auto-Recast", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Active Status", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (U32 i = 0; i < Skills.GetCount(); ++i)
            {
                const auto& S = Skills[i];
                if (!S.IsLearned || S.IsPassive || (S.TargetType != 0 && S.TargetType != 2 && S.TargetType != 8)) continue;

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", S.Name);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lv.%u", S.Level);

                ImGui::TableSetColumnIndex(2);
                bool isAuto = BuffManager::IsAutoBuff(S.SkillId);
                char chkId[32];
                StringUtils::Format(chkId, sizeof(chkId), "##autobuff_%u", S.SkillId);
                if (ImGui::Checkbox(chkId, &isAuto))
                {
                    BuffManager::SetAutoBuff(S.SkillId, isAuto);
                }

                ImGui::TableSetColumnIndex(3);
                U32 remSeconds = 0;
                if (BuffManager::HasBuff(S.SkillId, &remSeconds))
                {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "Active (%us)", remSeconds);
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Expired");
                }

                ImGui::TableSetColumnIndex(4);
                char btnCast[32];
                StringUtils::Format(btnCast, sizeof(btnCast), "Cast##%u", S.SkillId);
                if (ImGui::SmallButton(btnCast))
                {
                    BuffManager::CastBuff(S.LearnedSlot);
                }
            }
            ImGui::EndTable();
        }
    }

    void Menu::RenderGrindBotTab()
    {
        GrindBotConfig& Cfg = GrindBot::GetConfig();
        const GrindBotStats& Stats = GrindBot::GetStats();

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Autonomous Grind Bot Controller");
        ImGui::Separator();

        bool botActive = Cfg.Enabled;
        if (ImGui::Checkbox("Enable Grind Bot", &botActive))
        {
            GrindBot::ToggleActive();
        }

        ImGui::SameLine();
        ImGui::Text("Current State: ");
        ImGui::SameLine();
        ImVec4 stateColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        if (GrindBot::GetState() == GrindBotState::Combat) stateColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        else if (GrindBot::GetState() == GrindBotState::Looting) stateColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
        else if (GrindBot::GetState() == GrindBotState::Approaching) stateColor = ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
        else if (GrindBot::GetState() == GrindBotState::Resting) stateColor = ImVec4(0.9f, 0.4f, 0.9f, 1.0f);
        ImGui::TextColored(stateColor, "[%s]", GrindBot::GetStateName());

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Patrol & Anchor Area");

        if (Cfg.HasAnchor)
        {
            ImGui::Text("Anchor Location: X: %.1f | Y: %.1f | Z: %.1f",
                Cfg.AnchorPosition.X, Cfg.AnchorPosition.Y, Cfg.AnchorPosition.Z);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No Anchor Set (Uses current player position when started)");
        }

        const auto& player = EntityManager::GetLocalPlayer();
        if (player.Valid)
        {
            if (ImGui::Button("Set Current Position as Anchor"))
            {
                GrindBot::SetAnchor(player.Position);
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Clear Anchor"))
        {
            GrindBot::ClearAnchor();
        }

        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat("Leash Radius (m)", &Cfg.LeashRadius, 15.0f, 100.0f, "%.1fm");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat("Combat Approach Range (m)", &Cfg.CombatApproachDistance, 5.0f, 30.0f, "%.1fm");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Safety & Filters");

        ImGui::Checkbox("Hunt Quest Monsters Only", &Cfg.QuestMonstersOnly);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::SliderFloat("Rest & Heal if HP <= %", &Cfg.RestHpThresholdPercent, 15.0f, 70.0f, "%.0f%%");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Session Statistics");

        ImGui::Text("Monsters Killed: %u", Stats.MonstersKilled);

        U32 elapsedSec = (Stats.SessionStartTick > 0) ? (GetTickCount() - Stats.SessionStartTick) / 1000 : 0;
        U32 hours = elapsedSec / 3600;
        U32 minutes = (elapsedSec % 3600) / 60;
        U32 seconds = elapsedSec % 60;
        ImGui::Text("Session Time: %02u:%02u:%02u", hours, minutes, seconds);

        if (ImGui::Button("Reset Session Stats"))
        {
            GrindBot::ResetStats();
        }
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
        const FixedList<QuestMarker, 64>& Markers = QuestManager::GetQuestMarkers();
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

    void Menu::RenderMonsterEsp()
    {
        const auto& cfg = Settings::Get().Visuals;
        if (!cfg.MonsterEsp) return;

        ImGuiIO& Io = ImGui::GetIO();
        F32 ScreenW = Io.DisplaySize.x;
        F32 ScreenH = Io.DisplaySize.y;
        if (ScreenW <= 0.0f || ScreenH <= 0.0f) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        const auto& monsters = EntityManager::GetNearbyMonsters();

        for (U32 i = 0; i < monsters.GetCount(); i++)
        {
            const auto& m = monsters[i];
            if (!m.Alive || m.CurrentHp == 0) continue;

            Vector2 screenPos;
            if (!Camera::WorldToScreen(m.Position, screenPos, ScreenW, ScreenH))
                continue;

            Vector3 headPos = m.Position;
            headPos.Y += 1.8f;
            Vector2 headScreen;
            if (!Camera::WorldToScreen(headPos, headScreen, ScreenW, ScreenH))
                continue;

            ImU32 col = ImGui::ColorConvertFloat4ToU32(cfg.MonsterColor);

            if (cfg.MonsterDist)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "[%.0fm]", m.Distance);
                ImVec2 tsz = ImGui::CalcTextSize(buf);
                dl->AddText(ImVec2(screenPos.X - tsz.x * 0.5f, headScreen.Y - tsz.y - 2.0f), col, buf);
            }
        }
    }
}
