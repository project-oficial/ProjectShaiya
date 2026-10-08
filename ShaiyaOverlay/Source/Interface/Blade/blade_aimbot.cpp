#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"
#include "Game/Combat/ComboManager.h"
#include "Game/Bot/GrindBot.h"
#include "Game/Skills/SkillManager.h"
#include "Game/Entities/EntityManager.h"
#include <Windows.h>

#include <stdio.h>
#include <math.h>

using namespace Blade;

static float ACardPadding()
{
    return IsWhiteLabel() ? 14.0f : 16.0f;
}

struct ACard
{
    ImDrawList* dl;
    float x, y, w;
    int   rows = 0, i = 0, block = 0;
    float a = 1.0f;

    float Delay() const { return block * 0.05f; }

    void Begin(int n) { rows = n; i = 0; }

    float Row(float h, ImVec2* out_min = nullptr)
    {
        a = IntroT(Delay());
        float dy = (1.0f - a) * 12.0f;
        int corners = 0;
        if (i == 0)        corners |= ImDrawCornerFlags_Top;
        if (i == rows - 1) corners |= ImDrawCornerFlags_Bot;

        ImVec2 rmin(x, y + dy), rmax(x + w, y + h + dy);
        RectFilled(dl, rmin, rmax, Fade(C.row, a), 10.0f, corners);
        if (i > 0)
            dl->AddLine(ImVec2(x + 14.0f, rmin.y), ImVec2(x + w - 14.0f, rmin.y), Fade(C.divider, a), 1.0f);

        if (out_min) *out_min = rmin;
        float cy = rmin.y + h * 0.5f;
        y += h;
        i++; block++;
        return cy;
    }
};

static void RowLabel(ImDrawList* dl, float x, float w, float cy, const char* label, float a,
                     const char* desc = nullptr, float reserve = 176.0f, bool dim = false)
{
    const float card_padding = ACardPadding();
    float max_w = ImMax(w - card_padding - reserve, 24.0f);
    char buf[80];
    const char* shown = FitEllipsis(F_Med, label, max_w, buf, sizeof(buf));
    ImVec2 ts = Measure(F_Med, shown);
    TextAt(dl, F_Med, ImVec2(x + card_padding, cy - ts.y * 0.5f), Fade(dim ? C.text_dim : C.text, a), shown);

    bool truncated = (shown != label);
    if ((desc || truncated) &&
        ImGui::IsMouseHoveringRect(ImVec2(x + 12.0f, cy - 11.0f),
                                   ImVec2(x + 20.0f + ts.x, cy + 11.0f)))
        SetTooltip(label, desc);
}

static void RowSlider(ImDrawList* dl, float x, float w, float cy, const char* id,
                      float* v, float lo, float hi, const char* fmt, float a)
{
    char b[24]; snprintf(b, sizeof(b), fmt, ImLerp(lo, *v, a));
    float sx1 = x + w - ACardPadding();
    float sx0 = sx1 - 104.0f;
    ImVec2 ts = Measure(F_Body, b);
    PushAlpha(a);
    TextAt(dl, F_Body, ImVec2(sx0 - 10.0f - ts.x, cy - ts.y * 0.5f), C.text, b);
    SliderF(id, ImVec2(sx0, cy), ImVec2(sx1, cy), v, lo, hi, a);
    PopAlpha();
}

static void CardSlider(ACard& c, const char* label, const char* id, float* v, float lo, float hi, const char* fmt, const char* desc = nullptr)
{
    float cy = c.Row(46.0f);
    RowLabel(c.dl, c.x, c.w, cy, label, c.a, desc);
    RowSlider(c.dl, c.x, c.w, cy, id, v, lo, hi, fmt, c.a);
}

static void RowToggle(ACard& c, const char* label, const char* id, bool* v,
                      const char* desc = nullptr)
{
    float cy = c.Row(46.0f);
    RowLabel(c.dl, c.x, c.w, cy, label, c.a, desc, 58.0f);
    PushAlpha(c.a);
    Toggle(id, ImVec2(c.x + c.w - ACardPadding(), cy), v, 32.0f, 17.0f);
    PopAlpha();
}

static void RowHotkey(ACard& c, const char* label, const char* id, int* key)
{
    float cy = c.Row(46.0f);
    RowLabel(c.dl, c.x, c.w, cy, label, c.a, nullptr, 120.0f);
    PushAlpha(c.a);
    const float card_padding = ACardPadding();
    HotkeyButton(id, ImVec2(c.x + c.w - card_padding - 96.0f, cy - 14.0f),
                 ImVec2(c.x + c.w - card_padding, cy + 14.0f), key);
    PopAlpha();
}

void Blade::DrawAimbotContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                       float cx0, float cy0, float cw, float pad)
{
    const float view_y0 = cy0 + 28.0f;
    const float view_y1 = max.y - 10.0f;
    const float view_h  = view_y1 - view_y0;
    const bool white_label = IsWhiteLabel();
    const float gap     = white_label ? 14.0f : 12.0f;
    const float col_w   = (cw - pad * 2.0f - gap) * 0.5f;

    static float scroll = 0.0f;
    static float content_h = 0.0f;

    ImVec2 vmin(cx0, view_y0), vmax(max.x - 4.0f, view_y1);
    if (ImGui::IsMouseHoveringRect(vmin, vmax) && ImGui::GetIO().MouseWheel != 0.0f)
        scroll -= ImGui::GetIO().MouseWheel * 42.0f;
    scroll = ImClamp(scroll, 0.0f, ImMax(content_h - view_h, 0.0f));

    dl->PushClipRect(vmin, vmax, true);
    float base_y = view_y0 - scroll;

    const float HDR = 46.0f, ROW = 46.0f;

    int selected_module = S.module_sel[0];

    if (selected_module == 0) // Auto-Combo
    {
        auto& comboCfg = ShaiyaOverlay::ComboManager::GetConfig();
        const auto& seq = ShaiyaOverlay::ComboManager::GetComboSequence();

        // Left Column: Configuration
        {
            ACard L; L.dl = dl; L.x = cx0 + pad; L.y = base_y; L.w = col_w;
            L.Begin(5);
            {
                float cy = L.Row(HDR);
                PushAlpha(L.a);
                if (!white_label)
                    DrawIcon(dl, IC_BOLT, ImVec2(L.x + 26.0f, cy), 17.0f, Accent(1.0f), 1.5f);
                const float text_x = white_label ? L.x + pad : L.x + 42.0f;
                TextAt(dl, F_Title, ImVec2(text_x, cy - Measure(F_Title, pstra("Auto-Combo Engine")).y * 0.5f), C.text, pstra("Auto-Combo Engine"));
                const float checkbox_offset = IsWhiteLabel() ? 25.0f : 26.0f;
                Checkbox(pstra("##combo_en"), ImVec2(L.x + L.w - checkbox_offset, cy), &comboCfg.Enabled, 22.0f);
                PopAlpha();
            }
            RowToggle(L, pstra("Target Auto-Switch"), pstra("##combo_switch"), &comboCfg.AutoTargetNext, pstra("Troca de alvo automatico"));
            {
                bool questOnly = (comboCfg.TargetFilter == ShaiyaOverlay::TargetFilterMode::QuestMonstersOnly);
                bool oldQuest = questOnly;
                RowToggle(L, pstra("Prioritize Quest Mobs"), pstra("##combo_quest"), &questOnly, pstra("Prioriza monstros de missoes"));
                if (questOnly != oldQuest)
                    comboCfg.TargetFilter = questOnly ? ShaiyaOverlay::TargetFilterMode::QuestMonstersOnly : ShaiyaOverlay::TargetFilterMode::AllMonsters;
            }
            CardSlider(L, pstra("Attack Distance"), pstra("##combo_dist"), &comboCfg.MaxTargetRange, 1.0f, 30.0f, pstra("%.1f m"));

            int hk = (int)comboCfg.Hotkey;
            RowHotkey(L, pstra("Combo Hotkey"), pstra("##combo_hk"), &hk);
            comboCfg.Hotkey = (ShaiyaOverlay::U32)hk;

            L.y += gap;
            L.Begin(1);
            float cd = (float)comboCfg.CastDelayMs;
            CardSlider(L, pstra("Cast Delay"), pstra("##combo_delay"), &cd, 200.0f, 3000.0f, pstra("%.0f ms"));
            comboCfg.CastDelayMs = (ShaiyaOverlay::U32)cd;

            content_h = L.y - base_y;
        }

        // Right Column: Skill Sequence
        {
            ACard R; R.dl = dl; R.x = cx0 + pad + col_w + gap; R.y = base_y; R.w = col_w;
            int stepCount = (int)seq.GetCount();
            int totalRows = 2 + (stepCount > 0 ? (stepCount > 6 ? 6 : stepCount) : 1);
            R.Begin(totalRows);
            {
                float cy = R.Row(HDR);
                PushAlpha(R.a);
                if (!white_label)
                    DrawIcon(dl, IC_SWORD, ImVec2(R.x + 26.0f, cy), 17.0f, Accent(1.0f), 1.5f);
                const float text_x = white_label ? R.x + pad : R.x + 42.0f;
                TextAt(dl, F_Title, ImVec2(text_x, cy - Measure(F_Title, pstra("Rotation Sequence")).y * 0.5f), C.text, pstra("Rotation Sequence"));
                PopAlpha();
            }

            if (stepCount == 0)
            {
                float cy = R.Row(ROW);
                TextAt(dl, F_Body, ImVec2(R.x + ACardPadding(), cy - Measure(F_Body, pstra("No skills in sequence.")).y * 0.5f), C.text_mute, pstra("No skills in sequence."));
            }
            else
            {
                for (int s = 0; s < stepCount && s < 6; s++)
                {
                    float cy = R.Row(38.0f);
                    char stepStr[64];
                    snprintf(stepStr, sizeof(stepStr), pstra("#%d %s"), s + 1, seq[s].Name);
                    TextAt(dl, F_Body, ImVec2(R.x + ACardPadding(), cy - Measure(F_Body, stepStr).y * 0.5f), C.text, stepStr);

                    char delayStr[24];
                    snprintf(delayStr, sizeof(delayStr), pstra("ID %u"), seq[s].SkillId);
                    TextRight(dl, F_Small, ImVec2(R.x + R.w - ACardPadding(), cy - Measure(F_Small, delayStr).y * 0.5f), C.text_dim, delayStr);
                }
            }

            // Quick add / clear action row
            {
                ImVec2 rmin;
                float cy = R.Row(ROW, &rmin);
                float btn_w = (R.w - ACardPadding() * 2.0f - 8.0f) * 0.5f;
                ImVec2 b1_min(R.x + ACardPadding(), rmin.y + 6.0f);
                ImVec2 b1_max(b1_min.x + btn_w, rmin.y + ROW - 6.0f);
                if (Button(pstra("##btn_auto_add"), b1_min, b1_max, pstra("Auto Add Skills"), true))
                {
                    const auto& skills = ShaiyaOverlay::SkillManager::GetSkills();
                    for (unsigned int i = 0; i < skills.GetCount(); i++)
                    {
                        if (skills[i].IsLearned && !skills[i].IsPassive && skills[i].SkillId > 0)
                        {
                            ShaiyaOverlay::ComboManager::AddSkillToSequence(skills[i].SkillId, skills[i].Name);
                        }
                    }
                }

                ImVec2 b2_min(b1_max.x + 8.0f, rmin.y + 6.0f);
                ImVec2 b2_max(b2_min.x + btn_w, rmin.y + ROW - 6.0f);
                if (Button(pstra("##btn_clear_seq"), b2_min, b2_max, pstra("Clear Rotation"), false))
                {
                    ShaiyaOverlay::ComboManager::ClearSequence();
                }
            }

            content_h = ImMax(content_h, R.y - base_y);
        }
    }
    else // Grind Bot
    {
        auto& botCfg = ShaiyaOverlay::GrindBot::GetConfig();
        const char* botState = ShaiyaOverlay::GrindBot::GetStateName();

        // Left Column: Bot Engine & Anchor
        {
            ACard L; L.dl = dl; L.x = cx0 + pad; L.y = base_y; L.w = col_w;
            L.Begin(5);
            {
                float cy = L.Row(HDR);
                PushAlpha(L.a);
                if (!white_label)
                    DrawIcon(dl, IC_BOLT, ImVec2(L.x + 26.0f, cy), 17.0f, Accent(1.0f), 1.5f);
                const float text_x = white_label ? L.x + pad : L.x + 42.0f;
                TextAt(dl, F_Title, ImVec2(text_x, cy - Measure(F_Title, pstra("Grind Bot Engine")).y * 0.5f), C.text, pstra("Grind Bot Engine"));
                const float checkbox_offset = IsWhiteLabel() ? 25.0f : 26.0f;
                Checkbox(pstra("##bot_en"), ImVec2(L.x + L.w - checkbox_offset, cy), &botCfg.Enabled, 22.0f);
                PopAlpha();
            }
            {
                float cy = L.Row(ROW);
                RowLabel(dl, L.x, L.w, cy, pstra("Engine State"), L.a, pstra("Estado atual do bot"));
                TextRight(dl, F_Body, ImVec2(L.x + L.w - ACardPadding(), cy - Measure(F_Body, botState).y * 0.5f), Accent(1.0f), botState);
            }
            CardSlider(L, pstra("Leash Radius"), pstra("##bot_roam"), &botCfg.LeashRadius, 5.0f, 150.0f, pstra("%.0f m"));
            CardSlider(L, pstra("Approach Distance"), pstra("##bot_stop"), &botCfg.CombatApproachDistance, 1.0f, 20.0f, pstra("%.1f m"));
            {
                ImVec2 rmin;
                float cy = L.Row(ROW, &rmin);
                ImVec2 bmin(L.x + ACardPadding(), rmin.y + 6.0f);
                ImVec2 bmax(L.x + L.w - ACardPadding(), rmin.y + ROW - 6.0f);
                if (Button(pstra("##btn_set_anchor"), bmin, bmax, botCfg.HasAnchor ? pstra("Anchor Set (Click to Reset)") : pstra("Set Current Pos as Anchor"), true))
                {
                    const auto& player = ShaiyaOverlay::EntityManager::GetLocalPlayer();
                    if (player.Valid)
                    {
                        ShaiyaOverlay::GrindBot::SetAnchor(player.Position);
                        Blade::PushNotification(pstra("Anchor position saved!"), NT_SUCCESS);
                    }
                }
            }

            content_h = L.y - base_y;
        }

        // Right Column: Safety & Rest
        {
            ACard R; R.dl = dl; R.x = cx0 + pad + col_w + gap; R.y = base_y; R.w = col_w;
            R.Begin(4);
            {
                float cy = R.Row(HDR);
                PushAlpha(R.a);
                if (!white_label)
                    DrawIcon(dl, IC_HELMET, ImVec2(R.x + 26.0f, cy), 17.0f, Accent(1.0f), 1.5f);
                const float text_x = white_label ? R.x + pad : R.x + 42.0f;
                TextAt(dl, F_Title, ImVec2(text_x, cy - Measure(F_Title, pstra("Safety & Recovery")).y * 0.5f), C.text, pstra("Safety & Recovery"));
                PopAlpha();
            }
            CardSlider(R, pstra("Rest HP %"), pstra("##bot_rest_hp"), &botCfg.RestHpThresholdPercent, 10.0f, 80.0f, pstra("%.0f%%"));
            RowToggle(R, pstra("Quest Mobs Only"), pstra("##bot_questonly"), &botCfg.QuestMonstersOnly, pstra("Ataca apenas monstros de missoes"));
            {
                float cy = R.Row(ROW);
                char statBuf[64];
                snprintf(statBuf, sizeof(statBuf), pstra("Kills: %u | Looted: %u"), ShaiyaOverlay::GrindBot::GetStats().MonstersKilled, ShaiyaOverlay::GrindBot::GetStats().ItemsLooted);
                RowLabel(dl, R.x, R.w, cy, pstra("Session Stats"), R.a);
                TextRight(dl, F_Small, ImVec2(R.x + R.w - ACardPadding(), cy - Measure(F_Small, statBuf).y * 0.5f), C.text_mute, statBuf);
            }

            content_h = ImMax(content_h, R.y - base_y);
        }
    }

    dl->PopClipRect();

    if (content_h > view_h)
    {
        float track_x = max.x - 7.0f;
        float ratio = view_h / content_h;
        float bar_h = ImMax(view_h * ratio, 24.0f);
        float bar_y = view_y0 + (view_h - bar_h) * (scroll / ImMax(content_h - view_h, 1.0f));
        dl->AddRectFilled(ImVec2(track_x - 1.5f, view_y0), ImVec2(track_x + 1.5f, view_y1), IM_COL32(38, 38, 46, 200), 1.5f);
        dl->AddRectFilled(ImVec2(track_x - 1.5f, bar_y), ImVec2(track_x + 1.5f, bar_y + bar_h), C.scroll, 1.5f);
    }
}
