#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"
#include "Core/Types.h"
#include "Game/Items/GroundItemManager.h"
#include "Game/Items/InventoryManager.h"
#include "Game/Combat/HealManager.h"
#include "Game/Buffs/BuffManager.h"
#include "Game/Skills/SkillManager.h"
#include "Game/Login/AutoLoginManager.h"
#include "Game/Entities/EntityManager.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include <vector>
#include <stdlib.h>

using namespace Blade;

static Settings::sInterface& Ui()
{
    return Settings::Get().Interface;
}

static ImU32 ColorU32(const ImColor& color, float alpha = 1.0f)
{
    ImVec4 value = color.Value;
    value.w = alpha;
    return ImGui::GetColorU32(value);
}

struct SearchItem { ProtectedText name; int tab; int module; };
static const SearchItem kSearch[] = {
    { pstra("Auto-Combo"), 0, 0 }, { pstra("Combat range"), 0, 0 }, { pstra("Target switch"), 0, 0 },
    { pstra("Grind Bot"), 0, 1 }, { pstra("Roam radius"), 0, 1 }, { pstra("Anchor position"), 0, 1 },
    { pstra("Monster ESP"), 1, 0 }, { pstra("Entity health"), 1, 0 }, { pstra("NPC ESP"), 1, 0 },
    { pstra("Loot ESP"), 1, 1 }, { pstra("Loot snaplines"), 1, 1 }, { pstra("Quest markers"), 1, 2 },
    { pstra("Auto-Loot"), 2, 0 }, { pstra("Loot radius"), 2, 0 }, { pstra("Filter items"), 2, 0 },
    { pstra("Inventory bags"), 2, 1 }, { pstra("Consumables"), 2, 1 },
    { pstra("Auto-Heal"), 3, 0 }, { pstra("HP threshold"), 3, 0 }, { pstra("MP threshold"), 3, 0 }, { pstra("SP threshold"), 3, 0 },
    { pstra("Auto-Buff"), 3, 1 }, { pstra("Buff rotation"), 3, 1 },
    { pstra("Menu hotkey"), 4, 0 }, { pstra("Panic key"), 4, 0 }, { pstra("UI scale"), 4, 0 }, { pstra("White Label"), 4, 0 },
};
static const int kSearchCount = IM_ARRAYSIZE(kSearch);

static bool SearchMatch(const char* hay, const char* needle)
{
    if (!needle[0]) return false;
    for (const char* h = hay; *h; h++)
    {
        const char* a = h; const char* b = needle;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
        if (!*b) return true;
    }
    return false;
}

static ImVec2 g_search_min(0, 0), g_search_max(0, 0);

static const float MENU_W     = 812.0f;
static const float MENU_H     = 462.0f;
static const float SIDEBAR_W  = 156.0f;
static const float TOPBAR_H   = 46.0f;
static const float ROW_H      = 38.0f;
static const float ROW_GAP    = 2.0f;
static const float SECT_H     = 27.0f;
static const float COL_GAP    = 14.0f;
static const float CONT_PAD   = 14.0f;
static const float WL_MENU_W  = 840.0f;
static const float WL_MENU_H  = 500.0f;

static float CardControlPadding()
{
    return IsWhiteLabel() ? CONT_PAD : 10.0f;
}

static ImU32 OpaquePanelColor(float delta)
{
    const ImVec4 panel = Ui().PanelColor.Value;
    const float r = ImClamp(panel.x + delta, 0.0f, 1.0f);
    const float g = ImClamp(panel.y + delta, 0.0f, 1.0f);
    const float b = ImClamp(panel.z + delta * 1.18f, 0.0f, 1.0f);
    return IM_COL32((int)(r * 255.0f + 0.5f),
                    (int)(g * 255.0f + 0.5f),
                    (int)(b * 255.0f + 0.5f),
                    255);
}

struct MenuTab { Icon ic; ProtectedText label; };
static const MenuTab kTabs[] = {
    { IC_SWORD,   pstra("Combat")   },
    { IC_EYE,     pstra("Visuals")  },
    { IC_CART,    pstra("Loot")     },
    { IC_POTION,  pstra("Support")  },
    { IC_GEAR,    pstra("Settings") },
};
static const int kTabCount = IM_ARRAYSIZE(kTabs);
static const int TAB_COMBAT   = 0;
static const int TAB_VISUALS  = 1;
static const int TAB_LOOT     = 2;
static const int TAB_SUPPORT  = 3;
static const int TAB_SETTINGS = 4;

struct Col
{
    ImDrawList* dl;
    float x, y, w;
    int   group_rows = 0;
    int   group_i = 0;
    int   block = 0;
    float a = 1.0f;

    static const int STAGGER_MS = 45;

    float Delay() const { return block * (STAGGER_MS / 1000.0f); }

    void Section(const char* title, int rows)
    {
        float sa = IntroT(Delay());
        float dy = (1.0f - sa) * 12.0f;

        ImVec2 ts = Measure(F_Small, title);
        TextAt(dl, F_Small, ImVec2(x + 2.0f, y + (SECT_H - ts.y) * 0.5f + 3.0f + dy),
               Fade(C.text_mute, sa), title);

        y += SECT_H;
        group_rows = rows;
        group_i = 0;
        block++;
    }

    float Row(const char* label, bool dim = false, const char* desc = nullptr, float reserve = 46.0f)
    {
        a = IntroT(Delay());
        float dy = (1.0f - a) * 12.0f;

        const bool first = (group_i == 0);
        const bool last  = (group_i == group_rows - 1);

        int corners = 0;
        if (first) corners |= ImDrawCornerFlags_Top;
        if (last)  corners |= ImDrawCornerFlags_Bot;

        ImVec2 rmin(x, y + dy), rmax(x + w, y + ROW_H + dy);
        RectFilled(dl, rmin, rmax, Fade(C.row, a), 10.0f, corners);

        if (!first)
            dl->AddLine(ImVec2(x + 14.0f, rmin.y), ImVec2(x + w - 14.0f, rmin.y),
                        Fade(C.divider, a), 1.0f);

        float max_w = ImMax(w - 14.0f - reserve, 24.0f);
        char buf[80];
        const char* shown = FitEllipsis(F_Body, label, max_w, buf, sizeof(buf));
        ImVec2 ts = Measure(F_Body, shown);
        TextAt(dl, F_Body, ImVec2(x + 14.0f, rmin.y + (ROW_H - ts.y) * 0.5f),
               Fade(dim ? C.text_mute : C.text, a), shown);

        bool truncated = (shown != label);
        if ((desc || truncated) &&
            ImGui::IsMouseHoveringRect(ImVec2(x + 12.0f, rmin.y + 2.0f),
                                       ImVec2(x + 18.0f + ts.x, rmax.y - 2.0f)))
            SetTooltip(label, desc);

        float cy = rmin.y + ROW_H * 0.5f;
        y += ROW_H;
        group_i++;
        block++;
        return cy;
    }
};

static void RowDropdown(Col& c, const char* label, const char* id,
                        int* value, const char* const* opts, int n)
{
    float cy = c.Row(label, false, nullptr, 100.0f);
    float dw = 82.0f, dh = 22.0f;
    const float control_padding = CardControlPadding();
    ImVec2 dmin(c.x + c.w - control_padding - dw, cy - dh * 0.5f);
    ImVec2 dmax(c.x + c.w - control_padding, cy + dh * 0.5f);

    PushAlpha(c.a);
    Dropdown(id, dmin, dmax, opts, n, value);
    PopAlpha();
}

static void RowToggle(Col& c, const char* label, const char* id, bool* v, bool dots)
{
    float cy = c.Row(label, false, nullptr, dots ? 72.0f : 48.0f);
    const float control_padding = CardControlPadding();

    PushAlpha(c.a);
    if (dots)
    {
        char did[64];
        snprintf(did, sizeof(did), pstra("%s_dots"), id);
        IconButton(did, IC_DOTS, ImVec2(c.x + c.w - control_padding - 52.0f, cy), 13.0f, C.text_mute, 10.0f);
    }
    Toggle(id, ImVec2(c.x + c.w - control_padding, cy), v, 32.0f, 17.0f);
    PopAlpha();
}

static void RowRange(Col& c, const char* label, const char* id,
                     float* lo, float* hi, float v_min, float v_max)
{
    float cy = c.Row(label, false, nullptr, c.w * 0.46f);

    char bl[16], bh[16];
    snprintf(bl, sizeof(bl), pstra("%.1f"), *lo);
    snprintf(bh, sizeof(bh), pstra("%.1f"), *hi);

    float sx0 = c.x + c.w * 0.60f;
    float sx1 = c.x + c.w - CardControlPadding();

    PushAlpha(c.a);
    ImVec2 ls = Measure(F_Body, bl);
    TextAt(c.dl, F_Body, ImVec2(c.x + c.w * 0.33f - ls.x * 0.5f, cy - ls.y * 0.5f), C.text, bl);
    DrawIcon(c.dl, IC_ARROWS_LR, ImVec2(c.x + c.w * 0.43f, cy), 12.0f, C.text_mute, 1.2f);
    ImVec2 hs = Measure(F_Body, bh);
    TextAt(c.dl, F_Body, ImVec2(c.x + c.w * 0.52f - hs.x * 0.5f, cy - hs.y * 0.5f), C.text, bh);

    RangeSlider(id, ImVec2(sx0, cy), ImVec2(sx1, cy), lo, hi, v_min, v_max, c.a);
    PopAlpha();
}

static void RowSlider(Col& c, const char* label, const char* id,
                      float* v, float v_min, float v_max, const char* fmt)
{
    float cy = c.Row(label, false, nullptr, c.w * 0.46f);

    char buf[24];
    snprintf(buf, sizeof(buf), fmt, ImLerp(v_min, *v, c.a));

    float sx0 = c.x + c.w * 0.60f;
    float sx1 = c.x + c.w - CardControlPadding();

    PushAlpha(c.a);
    ImVec2 ts = Measure(F_Body, buf);
    TextAt(c.dl, F_Body, ImVec2(sx0 - 12.0f - ts.x, cy - ts.y * 0.5f), C.text, buf);

    SliderF(id, ImVec2(sx0, cy), ImVec2(sx1, cy), v, v_min, v_max, c.a);
    PopAlpha();
}

static void DrawSidebar(ImDrawList* dl, ImVec2 min, float h)
{

    const float logo_area_h = 96.0f;
    if (TexLogoStack && TexLogoStackSz.y > 0.0f)
    {
        float maxw = SIDEBAR_W - 24.0f;
        float maxh = logo_area_h - 16.0f;
        float ar = TexLogoStackSz.x / TexLogoStackSz.y;

        float lw = maxw, lh = lw / ar;
        if (lh > maxh) { lh = maxh; lw = lh * ar; }

        ImVec2 c(min.x + SIDEBAR_W * 0.5f, min.y + logo_area_h * 0.5f);
        dl->AddImage(TexLogoStack,
                     ImVec2(c.x - lw * 0.5f, c.y - lh * 0.5f),
                     ImVec2(c.x + lw * 0.5f, c.y + lh * 0.5f));
    }
    else
    {
        float cy = min.y + logo_area_h * 0.5f;
        DrawIcon(dl, IC_BOLT, ImVec2(min.x + 30.0f, cy), 17.0f, Accent(1.0f), 1.5f);
        TextAt(dl, F_Head, ImVec2(min.x + 44.0f, cy - Measure(F_Head, pstra("BLADE")).y * 0.5f),
               C.text, pstra("BLADE"));
    }

    const float item_h = 23.0f;
    const float gap    = 2.0f;
    const float mod_h  = 20.0f;
    const float top    = min.y + logo_area_h + 6.0f;

    static int   g_sb_tab      = -2;
    static float g_sb_t0       = -100.0f;
    static bool  g_sb_collapse = false;

    const float now   = (float)ImGui::GetTime();
    const float speed = ImMax(Ui().AnimationSpeed, 0.05f);

    if (g_sb_tab == -2) { g_sb_tab = S.menu_tab; g_sb_t0 = now - 5.0f; }

    const float el = (now - g_sb_t0) * speed;

    auto easeOut = [](float t) { float i = 1.0f - t; return 1.0f - i * i * i; };

    float bp;
    if (g_sb_collapse) { float t = ImClamp(el / 0.30f, 0.0f, 1.0f); bp = 1.0f - easeOut(t); }
    else               { float t = ImClamp(el / 0.40f, 0.0f, 1.0f); bp = easeOut(t); }

    const float SB_STEP = 0.06f;
    const float SB_DUR  = 0.22f;
    auto childFade = [&](int m, int count) -> float {
        if (g_sb_collapse) { float d = (count - 1 - m) * SB_STEP; float t = ImClamp((el - d) / SB_DUR, 0.0f, 1.0f); return 1.0f - easeOut(t); }
        float d = m * SB_STEP; float t = ImClamp((el - d) / SB_DUR, 0.0f, 1.0f); return easeOut(t);
    };

    static float g_slide_y[12]   = { 0 };
    static bool  g_slide_rdy[12] = { false };

    const float prof_cy    = min.y + h - 32.0f;
    const float avatar_cy  = prof_cy + 6.0f;
    const float nav_top    = top;
    const float nav_bottom = prof_cy - 30.0f;
    const float nav_h      = nav_bottom - nav_top;

    float content_h = 0.0f;
    for (int i = 0; i < kTabCount; i++)
    {
        content_h += item_h + gap;
        if (g_sb_tab == i)
        {
            int mc = 0; GetModules(i, &mc); if (mc > 16) mc = 16;
            content_h += (mc * mod_h + 5.0f) * bp;
        }
    }
    const float max_scroll = ImMax(0.0f, content_h - nav_h);

    static float g_sb_scroll = 0.0f;
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.MousePos.x >= min.x && io.MousePos.x <= min.x + SIDEBAR_W &&
            io.MousePos.y >= nav_top && io.MousePos.y <= nav_bottom && io.MouseWheel != 0.0f)
            g_sb_scroll -= io.MouseWheel * 34.0f;
    }
    g_sb_scroll = ImClamp(g_sb_scroll, 0.0f, max_scroll);

    dl->PushClipRect(ImVec2(min.x, nav_top - 2.0f),
                     ImVec2(min.x + SIDEBAR_W, nav_bottom + 2.0f), true);

    float y = nav_top - g_sb_scroll;
    for (int i = 0; i < kTabCount; i++)
    {
        ImVec2 rmin(min.x + 10.0f, y), rmax(min.x + SIDEBAR_W - 10.0f, y + item_h);
        bool row_vis = (y + item_h > nav_top - 1.0f && y < nav_bottom + 1.0f);

        char id[40];
        snprintf(id, sizeof(id), pstra("##mtab%d"), i);
        bool hov = false;
        if (row_vis && Hitbox(id, rmin, rmax, &hov))
        {
            if (S.menu_tab == i && g_sb_tab == i && !g_sb_collapse)
            {
                g_sb_collapse = true; g_sb_t0 = now;
            }
            else
            {
                S.menu_tab = i; g_sb_tab = i; g_sb_collapse = false; g_sb_t0 = now;
            }
        }

        bool sel = (S.menu_tab == i);
        if (sel)      RectFilled(dl, rmin, rmax, C.sel, 7.0f);
        else if (hov) RectFilled(dl, rmin, rmax, C.row, 7.0f);

        float cy = y + item_h * 0.5f;
        if (sel)
            RectFilled(dl, ImVec2(min.x + 1.0f, cy - 8.0f), ImVec2(min.x + 3.0f, cy + 8.0f), Accent(1.0f), 1.5f);

        DrawTabIcon(dl, i, kTabs[i].ic, ImVec2(rmin.x + 12.0f, cy), 15.0f, sel ? Accent(1.0f) : C.text_mute);
        ImVec2 ts = Measure(F_Body, kTabs[i].label);
        TextAt(dl, F_Body, ImVec2(rmin.x + 26.0f, cy - ts.y * 0.5f), sel ? C.text : C.text_mute, kTabs[i].label);

        y += item_h + gap;

        if (g_sb_tab == i)
        {
            int mc = 0;
            const Module* mods = GetModules(i, &mc);
            if (mc > 16) mc = 16;

            const float block_top = y;
            const float branch_x  = min.x + 21.0f;
            const float r = 6.0f;
            const float full_h = mc * mod_h + 5.0f;
            const float anim_h = full_h * bp;

            auto LerpCol = [](ImU32 a, ImU32 b, float t) {
                return ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(a),
                                                 ImGui::ColorConvertU32ToFloat4(b), t));
            };

            float first_cy = block_top + mod_h * 0.5f;
            float last_cy  = block_top + (mc - 1) * mod_h + mod_h * 0.5f;

            dl->PushClipRect(ImVec2(min.x, ImMax(block_top - 1.0f, nav_top - 2.0f)),
                             ImVec2(min.x + SIDEBAR_W,
                                    ImMin(block_top + anim_h + 1.0f, nav_bottom + 2.0f)), true);

            dl->AddLine(ImVec2(branch_x, block_top), ImVec2(branch_x, last_cy - r),
                        Fade(C.separator, bp), 1.6f);

            for (int m = 0; m < mc; m++)
            {
                float a   = childFade(m, mc);
                float mcy = block_top + m * mod_h + mod_h * 0.5f;
                float my  = mcy - mod_h * 0.5f;

                bool child_vis = (mcy > nav_top && mcy < nav_bottom);
                if (a > 0.6f && child_vis)
                {
                    char mid[40]; snprintf(mid, sizeof(mid), pstra("##mod%d_%d"), i, m);
                    bool mhov = false;
                    if (Hitbox(mid, ImVec2(min.x + 24.0f, my), ImVec2(min.x + SIDEBAR_W - 10.0f, my + mod_h), &mhov))
                        S.module_sel[i] = m;
                    (void)mhov;
                }

                char sid[48]; snprintf(sid, sizeof(sid), pstra("##selm%d_%d"), i, m);
                float sa = Anim(ImGui::GetCurrentWindow()->GetID(sid), S.module_sel[i] == m, 14.0f);

                dl->PathClear();
                dl->PathArcTo(ImVec2(branch_x + r, mcy - r), r, IM_PI, IM_PI * 0.5f, 8);
                dl->PathLineTo(ImVec2(branch_x + 14.0f, mcy));
                dl->PathStroke(Fade(C.separator, a), false, 1.6f);

                ImVec2 mts = Measure(F_Body, mods[m].name);
                ImU32 tcol = Fade(LerpCol(C.text_mute, C.text, sa), a);
                TextAt(dl, F_Body, ImVec2(min.x + 40.0f + sa * 3.0f, mcy - mts.y * 0.5f), tcol, mods[m].name);
            }

            int selm = S.module_sel[i];
            if (bp > 0.6f && selm >= 0 && selm < mc)
            {
                float target = block_top + selm * mod_h + mod_h * 0.5f;
                if (!g_slide_rdy[i]) { g_slide_y[i] = target; g_slide_rdy[i] = true; }
                else g_slide_y[i] = ImLerp(g_slide_y[i], target,
                                           ImClamp(ImGui::GetIO().DeltaTime * 16.0f * speed, 0.0f, 1.0f));
                float sy = ImClamp(g_slide_y[i], first_cy, last_cy);

                dl->PathClear();
                dl->PathArcTo(ImVec2(branch_x + r, sy - r), r, IM_PI, IM_PI * 0.5f, 8);
                dl->PathLineTo(ImVec2(branch_x + 14.0f, sy));
                dl->PathStroke(Accent(1.0f), false, 2.0f);
            }

            dl->PopClipRect();
            y += anim_h;

            if (g_sb_collapse && bp <= 0.001f) g_sb_tab = -1;
        }
    }
    dl->PopClipRect();

    if (max_scroll > 0.0f)
    {
        float tx = min.x + SIDEBAR_W - 4.0f;
        float th = ImMax(24.0f, nav_h * nav_h / ImMax(content_h, 1.0f));
        float ty = nav_top + (nav_h - th) * (g_sb_scroll / max_scroll);
        RectFilled(dl, ImVec2(tx, nav_top), ImVec2(tx + 2.0f, nav_bottom), Fade(C.separator, 0.4f), 1.0f);
        RectFilled(dl, ImVec2(tx, ty), ImVec2(tx + 2.0f, ty + th), Fade(C.text_mute, 0.7f), 1.0f);
    }

    dl->AddLine(ImVec2(min.x + 12.0f, nav_bottom + 8.0f),
                ImVec2(min.x + SIDEBAR_W - 12.0f, nav_bottom + 8.0f),
                Fade(C.separator, 0.5f), 1.0f);

    char user_buf[64];
    char until_buf[64];
    const char* user = FitEllipsis(F_Body, S.session_user, SIDEBAR_W - 74.0f, user_buf, sizeof(user_buf));
    const char* until = FitEllipsis(F_Tiny, S.session_until, SIDEBAR_W - 74.0f, until_buf, sizeof(until_buf));
    const float profile_gap = 2.0f;
    const ImVec2 user_size = Measure(F_Body, user);
    const ImVec2 until_size = Measure(F_Tiny, until);
    const float profile_text_y = prof_cy - (user_size.y + profile_gap + until_size.y) * 0.5f;
    DrawAvatar(dl, ImVec2(min.x + 32.0f, avatar_cy), 20.0f);
    TextAt(dl, F_Body, ImVec2(min.x + 60.0f, profile_text_y), C.text, user);
    TextAt(dl, F_Tiny, ImVec2(min.x + 60.0f, profile_text_y + user_size.y + profile_gap), C.text_mute, until);
}

static void DrawTopbar(ImDrawList* dl, ImVec2 min, float w)
{
    float cy = min.y + TOPBAR_H * 0.5f;

    float right_w = 72.0f;

    ImVec2 smin(min.x + SIDEBAR_W + CONT_PAD, cy - 13.0f);
    ImVec2 smax(min.x + w - right_w, cy + 13.0f);
    g_search_min = smin; g_search_max = smax;
    bool s_hov = false;
    if (Hitbox(pstra("##bl_search"), smin, smax, &s_hov)) S.search_focus = true;
    RectFilled(dl, smin, smax, (s_hov || S.search_focus) ? C.pill_hover : C.pill, 13.0f);
    if (S.search_focus) RectStroke(dl, smin, smax, Accent(0.55f), 13.0f);
    DrawIcon(dl, IC_SEARCH, ImVec2(smin.x + 15.0f, cy), 13.0f, C.text_mute, 1.4f);

    if (S.search_focus)
    {
        ImGuiIO& io = ImGui::GetIO();
        for (int n = 0; n < io.InputQueueCharacters.Size; n++)
        {
            ImWchar c = io.InputQueueCharacters[n];
            if (c >= 32 && c < 127)
            {
                int len = (int)strlen(S.search);
                if (len < (int)sizeof(S.search) - 1) { S.search[len] = (char)c; S.search[len + 1] = 0; }
            }
        }
        if (ImGui::IsKeyPressed(0x08, true))
        {
            int len = (int)strlen(S.search);
            if (len > 0) S.search[len - 1] = 0;
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape), false))
        {
            S.search[0] = 0; S.search_focus = false;
        }
    }

    const std::string ph(pstra("Search element"));
    bool empty = (S.search[0] == 0);
    const char* txt = empty ? ph.c_str() : S.search;
    TextAt(dl, F_Body, ImVec2(smin.x + 30.0f, cy - Measure(F_Body, txt).y * 0.5f),
           empty ? C.text_mute : C.text, txt);

    if (S.search_focus && fmodf((float)ImGui::GetTime(), 1.0f) < 0.5f)
    {
        float tw = empty ? 0.0f : Measure(F_Body, S.search).x;
        float curx = smin.x + 30.0f + tw + 1.0f;
        dl->AddLine(ImVec2(curx, cy - 7.0f), ImVec2(curx, cy + 7.0f), C.text, 1.0f);
    }

    IconButton(pstra("##bl_folder"), IC_FOLDER, ImVec2(min.x + w - 50.0f, cy), 15.0f, C.text_dim, 12.0f);

    ImVec2 gc(min.x + w - 24.0f, cy);
    ImGuiID gear_id = ImGui::GetCurrentWindow()->GetID(pstra("##bl_gear"));
    bool gear_open = IsPopupOpen(gear_id);

    bool g_hov = false;
    bool g_click = Hitbox(pstra("##bl_gear"), ImVec2(gc.x - 13, gc.y - 13), ImVec2(gc.x + 13, gc.y + 13), &g_hov);
    if (g_hov || gear_open)
        RectFilled(dl, ImVec2(gc.x - 13, gc.y - 13), ImVec2(gc.x + 13, gc.y + 13),
                          IM_COL32(255, 255, 255, gear_open ? 18 : 10), 8.0f);
    DrawIcon(dl, IC_GEAR, gc, 16.0f, (g_hov || gear_open) ? C.text : C.text_dim, 1.4f);

    if (g_click)
    {
        if (gear_open) ClosePopup();
        else OpenPopup(PK_PROFILE, gear_id, ImVec2(gc.x - 13, gc.y - 13), ImVec2(gc.x + 13, gc.y + 13));
    }
}

static void DrawBreadcrumb(ImDrawList* dl, float cx0, float cy0, float cw)
{
    float by = cy0 + 14.0f;
    DrawTabIcon(dl, S.menu_tab, kTabs[S.menu_tab].ic, ImVec2(cx0 + CONT_PAD + 7.0f, by), 15.0f, C.text_dim);

    const char* root = kTabs[S.menu_tab].label;
    ImVec2 rs = Measure(F_Body, root);
    TextAt(dl, F_Body, ImVec2(cx0 + CONT_PAD + 21.0f, by - rs.y * 0.5f), C.text_dim, root);

    float ax = cx0 + CONT_PAD + 21.0f + rs.x + 9.0f;
    dl->AddLine(ImVec2(ax, by), ImVec2(ax + 8.0f, by), C.text_mute, 1.2f);
    dl->AddLine(ImVec2(ax + 5.0f, by - 3.0f), ImVec2(ax + 8.5f, by), C.text_mute, 1.2f);
    dl->AddLine(ImVec2(ax + 5.0f, by + 3.0f), ImVec2(ax + 8.5f, by), C.text_mute, 1.2f);

    int mod_count = 0;
    const Module* mods = GetModules(S.menu_tab, &mod_count);
    int sel = ImClamp(S.module_sel[S.menu_tab], 0, mod_count - 1);
    const char* mod_name = mods[sel].name;

    ImVec2 ms = Measure(F_Body, mod_name);
    ImVec2 bmin(ax + 12.0f, by - 11.0f), bmax(ax + 18.0f + ms.x + 16.0f, by + 11.0f);

    ImGuiID mid = ImGui::GetCurrentWindow()->GetID(pstra("##bc_module"));
    bool open = IsPopupOpen(mid);
    bool hov = false;
    bool click = Hitbox(pstra("##bc_module"), bmin, bmax, &hov);
    if (hov || open)
        RectFilled(dl, bmin, bmax, IM_COL32(255, 255, 255, open ? 16 : 9), 7.0f);

    TextAt(dl, F_Body, ImVec2(ax + 15.0f, by - ms.y * 0.5f), C.text, mod_name);
    DrawIcon(dl, IC_CHEVRON, ImVec2(bmax.x - 9.0f, by), 11.0f, open ? Accent(1.0f) : C.text_mute, 1.3f);

    if (click)
    {
        if (open) ClosePopup();
        else      OpenPopup(PK_MODULES, mid, bmin, bmax);
    }
}

static void RowHotkey(Col& c, const char* label, const char* id, int* key,
                      unsigned int allowed_mouse = HK_MOUSE_ALL)
{
    float cy = c.Row(label, false, nullptr, 116.0f);
    const float control_padding = CardControlPadding();
    PushAlpha(c.a);
    HotkeyButton(id, ImVec2(c.x + c.w - control_padding - 98.0f, cy - 13.0f),
                 ImVec2(c.x + c.w - control_padding, cy + 13.0f), key, allowed_mouse);
    PopAlpha();
}

static void RowColorToggle(Col& c, const char* label, const char* id, bool* enabled, ImVec4* color)
{
    float cy = c.Row(label, false, nullptr, 92.0f);
    const float control_padding = CardControlPadding();
    PushAlpha(c.a);

    char cid[64];
    snprintf(cid, sizeof(cid), pstra("%s_col"), id);
    ImVec2 dot(c.x + c.w - control_padding - 52.0f, cy);
    ColorSwatch(cid, dot, 18.0f, &color->x, label);

    Toggle(id, ImVec2(c.x + c.w - control_padding, cy), enabled, 32.0f, 17.0f);
    PopAlpha();
}

static void RowColorToggle(Col& c, const char* label, const char* id, bool* enabled, ImColor* color)
{
    float cy = c.Row(label, false, nullptr, 92.0f);
    const float control_padding = CardControlPadding();
    PushAlpha(c.a);

    char cid[64];
    snprintf(cid, sizeof(cid), pstra("%s_col"), id);
    ImVec2 dot(c.x + c.w - control_padding - 52.0f, cy);
    ColorSwatch(cid, dot, 18.0f, &color->Value.x, label);

    Toggle(id, ImVec2(c.x + c.w - control_padding, cy), enabled, 32.0f, 17.0f);
    PopAlpha();
}

static void RowColor(Col& c, const char* label, const char* id, ImVec4* color)
{
    float cy = c.Row(label, false, nullptr, 62.0f);
    const float control_padding = CardControlPadding();
    PushAlpha(c.a);
    ColorSwatch(id, ImVec2(c.x + c.w - control_padding - 12.0f, cy), 18.0f, &color->x, label);
    PopAlpha();
}

static void DrawLootContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                            float cx0, float cy0, float cw)
{
    const float view_y0 = cy0 + 28.0f;
    const float view_y1 = max.y - 10.0f;
    const float view_h  = view_y1 - view_y0;
    const float col_w   = (cw - CONT_PAD * 2.0f - COL_GAP) * 0.5f;

    static float scroll = 0.0f;
    static float content_h = 0.0f;

    ImVec2 vmin(cx0, view_y0), vmax(max.x - 4.0f, view_y1);
    if (ImGui::IsMouseHoveringRect(vmin, vmax) && ImGui::GetIO().MouseWheel != 0.0f)
        scroll -= ImGui::GetIO().MouseWheel * 42.0f;
    scroll = ImClamp(scroll, 0.0f, ImMax(content_h - view_h, 0.0f));

    dl->PushClipRect(vmin, vmax, true);
    float base_y = view_y0 - scroll;

    Settings& settings = Settings::Get();

    Col L; L.dl = dl; L.x = cx0 + CONT_PAD; L.y = base_y; L.w = col_w;
    Col R; R.dl = dl; R.x = cx0 + CONT_PAD + col_w + COL_GAP; R.y = base_y; R.w = col_w;

    int selected_module = S.module_sel[2];

    if (selected_module == 0) // Auto-Loot
    {
        auto& lootCfg = ShaiyaOverlay::GroundItemManager::GetConfig();
        L.Section(pstra("Auto-Loot Engine"), 2);
        RowToggle(L, pstra("Enable Auto-Loot"), pstra("##df_loot_en"), &lootCfg.Enabled, false);
        RowSlider(L, pstra("Pickup Radius"), pstra("##df_loot_radius"), &lootCfg.PickupRadius, 1.0f, 30.0f, pstra("%.1f m"));

        const auto& groundItems = ShaiyaOverlay::GroundItemManager::GetGroundItems();
        char bufGround[48];
        snprintf(bufGround, sizeof(bufGround), pstra("Detected Drops: %u"), groundItems.GetCount());

        R.Section(pstra("Ground Loot Status"), 3);
        R.Row(bufGround, false);
        R.Row(pstra("Fast packet pick: Active"), true);
        R.Row(pstra("Prioritizes quest objectives & gold"), true);
    }
    else // Inventory
    {
        L.Section(pstra("Inventory Overview"), 3);
        L.Row(pstra("Capacity: 5 Bags x 48 Slots"), false);
        L.Row(pstra("Total Slots: 240 Available"), true);
        L.Row(pstra("Stride: 0x84 / Offsets centralized"), true);

        const auto& invItems = ShaiyaOverlay::InventoryManager::GetItems();
        int countUsable = 0;
        for (unsigned int i = 0; i < invItems.GetCount(); i++)
            if (invItems[i].IsConsumable && invItems[i].Count > 0) countUsable++;

        R.Section(pstra("Detected Usables"), 4);
        int shown = 0;
        for (unsigned int i = 0; i < invItems.GetCount() && shown < 4; i++)
        {
            if (invItems[i].IsConsumable && invItems[i].Count > 0)
            {
                char cbuf[64];
                snprintf(cbuf, sizeof(cbuf), pstra("B%d S%d: %s (x%d)"), invItems[i].Bag, invItems[i].Slot, invItems[i].Name, invItems[i].Count);
                R.Row(cbuf, false);
                shown++;
            }
        }
        if (shown == 0) R.Row(pstra("No consumables found in bags."), true);
    }

    content_h = ImMax(L.y, R.y) - base_y;
    dl->PopClipRect();

    if (content_h > view_h)
    {
        float track_x = max.x - 7.0f;
        float ratio = view_h / content_h;
        float bar_h = ImMax(view_h * ratio, 24.0f);
        float bar_y = view_y0 + (view_h - bar_h) * (scroll / ImMax(content_h - view_h, 1.0f));
        RectFilled(dl, ImVec2(track_x - 1.5f, view_y0), ImVec2(track_x + 1.5f, view_y1),
                          C.separator, 1.5f);
        RectFilled(dl, ImVec2(track_x - 1.5f, bar_y), ImVec2(track_x + 1.5f, bar_y + bar_h),
                          C.scroll, 1.5f);
    }
    (void)min;
}

static void DrawToolsContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                             float cx0, float cy0, float cw)
{
    const float view_y0 = cy0 + 28.0f;
    const float view_y1 = max.y - 10.0f;
    const float view_h  = view_y1 - view_y0;
    const float col_w   = (cw - CONT_PAD * 2.0f - COL_GAP) * 0.5f;

    static float scroll = 0.0f;
    static float content_h = 0.0f;

    ImVec2 vmin(cx0, view_y0), vmax(max.x - 4.0f, view_y1);
    if (ImGui::IsMouseHoveringRect(vmin, vmax) && ImGui::GetIO().MouseWheel != 0.0f)
        scroll -= ImGui::GetIO().MouseWheel * 42.0f;
    scroll = ImClamp(scroll, 0.0f, ImMax(content_h - view_h, 0.0f));

    dl->PushClipRect(vmin, vmax, true);
    float base_y = view_y0 - scroll;

    Col L; L.dl = dl; L.x = cx0 + CONT_PAD; L.y = base_y; L.w = col_w;
    Col R; R.dl = dl; R.x = cx0 + CONT_PAD + col_w + COL_GAP; R.y = base_y; R.w = col_w;

    int selected_module = S.module_sel[3];

    if (selected_module == 0) // Auto-Heal
    {
        auto& healCfg = ShaiyaOverlay::HealManager::GetConfig();
        L.Section(pstra("Auto-Heal Engine"), 5);
        RowToggle(L, pstra("Enable Auto-Heal"), pstra("##heal_en"), &healCfg.Enabled, false);
        RowSlider(L, pstra("HP Trigger %"), pstra("##heal_hp"), &healCfg.HpThresholdPercent, 10.0f, 95.0f, pstra("%.0f%%"));
        RowSlider(L, pstra("MP Trigger %"), pstra("##heal_mp"), &healCfg.MpThresholdPercent, 10.0f, 95.0f, pstra("%.0f%%"));
        RowSlider(L, pstra("SP Trigger %"), pstra("##heal_sp"), &healCfg.SpThresholdPercent, 10.0f, 95.0f, pstra("%.0f%%"));
        float gcd = (float)healCfg.PotionCooldownMs;
        RowSlider(L, pstra("Potion Delay"), pstra("##heal_gcd"), &gcd, 200.0f, 3000.0f, pstra("%.0f ms"));
        healCfg.PotionCooldownMs = (unsigned int)gcd;

        ShaiyaOverlay::InventoryItem hpItem{}, mpItem{}, spItem{};
        bool hasHp = ShaiyaOverlay::HealManager::FindBestHpItem(&hpItem);
        bool hasMp = ShaiyaOverlay::HealManager::FindBestMpItem(&mpItem);
        bool hasSp = ShaiyaOverlay::HealManager::FindBestSpItem(&spItem);

        char hpBuf[64], mpBuf[64], spBuf[64];
        snprintf(hpBuf, sizeof(hpBuf), pstra("HP: %s (x%d)"), hasHp ? hpItem.Name : pstra("None"), hasHp ? hpItem.Count : 0);
        snprintf(mpBuf, sizeof(mpBuf), pstra("MP: %s (x%d)"), hasMp ? mpItem.Name : pstra("None"), hasMp ? mpItem.Count : 0);
        snprintf(spBuf, sizeof(spBuf), pstra("SP: %s (x%d)"), hasSp ? spItem.Name : pstra("None"), hasSp ? spItem.Count : 0);

        R.Section(pstra("Detected Potions"), 4);
        R.Row(hpBuf, !hasHp);
        R.Row(mpBuf, !hasMp);
        R.Row(spBuf, !hasSp);
        R.Row(pstra("Native opcode: 0x050A (SendUseItem)"), true);
    }
    else // Auto-Buff
    {
        auto& buffCfg = ShaiyaOverlay::BuffManager::GetConfig();
        L.Section(pstra("Auto-Buff Engine"), 3);
        RowToggle(L, pstra("Enable Auto-Buff"), pstra("##buff_en"), &buffCfg.Enabled, false);
        float recastSec = (float)buffCfg.RecastThresholdSeconds;
        RowSlider(L, pstra("Recast Threshold"), pstra("##buff_buffer"), &recastSec, 1.0f, 15.0f, pstra("%.0f s"));
        buffCfg.RecastThresholdSeconds = (unsigned int)recastSec;
        L.Row(pstra("Auto TargetType routing: Active"), true);

        const auto& skills = ShaiyaOverlay::SkillManager::GetSkills();
        int buffCount = 0;
        for (unsigned int i = 0; i < skills.GetCount(); i++)
        {
            if (skills[i].IsLearned && !skills[i].IsPassive && (skills[i].TargetType == 0 || skills[i].TargetType == 2 || skills[i].TargetType == 8))
                buffCount++;
        }

        R.Section(pstra("Self Buff Rotation"), buffCount > 0 ? (buffCount > 4 ? 4 : buffCount) : 1);
        if (buffCount == 0)
        {
            R.Row(pstra("No buff skills learned yet."), true);
        }
        else
        {
            int shown = 0;
            for (unsigned int i = 0; i < skills.GetCount() && shown < 4; i++)
            {
                if (skills[i].IsLearned && !skills[i].IsPassive && (skills[i].TargetType == 0 || skills[i].TargetType == 2 || skills[i].TargetType == 8))
                {
                    bool isBuff = ShaiyaOverlay::BuffManager::IsAutoBuff(skills[i].SkillId);
                    bool oldBuff = isBuff;
                    char bid[32]; snprintf(bid, sizeof(bid), pstra("##buff_tg%u"), i);
                    RowToggle(R, skills[i].Name, bid, &isBuff, false);
                    if (isBuff != oldBuff)
                        ShaiyaOverlay::BuffManager::SetAutoBuff(skills[i].SkillId, isBuff);
                    shown++;
                }
            }
        }
    }

    content_h = ImMax(L.y, R.y) - base_y;
    dl->PopClipRect();

    if (content_h > view_h)
    {
        float track_x = max.x - 7.0f;
        float ratio = view_h / content_h;
        float bar_h = ImMax(view_h * ratio, 24.0f);
        float bar_y = view_y0 + (view_h - bar_h) * (scroll / ImMax(content_h - view_h, 1.0f));
        RectFilled(dl, ImVec2(track_x - 1.5f, view_y0), ImVec2(track_x + 1.5f, view_y1),
                          C.separator, 1.5f);
        RectFilled(dl, ImVec2(track_x - 1.5f, bar_y), ImVec2(track_x + 1.5f, bar_y + bar_h),
                          C.scroll, 1.5f);
    }
    (void)min;
}

static void DrawSettingsContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                                float cx0, float cy0, float cw)
{
    const float view_y0 = cy0 + 28.0f;
    const float view_y1 = max.y - 10.0f;
    const float view_h  = view_y1 - view_y0;
    const float col_w   = (cw - CONT_PAD * 2.0f - COL_GAP) * 0.5f;

    static float scroll = 0.0f;
    static float content_h = 0.0f;

    ImVec2 vmin(cx0, view_y0), vmax(max.x - 4.0f, view_y1);
    if (ImGui::IsMouseHoveringRect(vmin, vmax) && ImGui::GetIO().MouseWheel != 0.0f)
        scroll -= ImGui::GetIO().MouseWheel * 42.0f;
    scroll = ImClamp(scroll, 0.0f, ImMax(content_h - view_h, 0.0f));

    dl->PushClipRect(vmin, vmax, true);
    float base_y = view_y0 - scroll;

    Settings& settings = Settings::Get();

    Col L; L.dl = dl; L.x = cx0 + CONT_PAD; L.y = base_y; L.w = col_w;
    Col R; R.dl = dl; R.x = cx0 + CONT_PAD + col_w + COL_GAP; R.y = base_y; R.w = col_w;

    int selected_module = S.module_sel[4];

    if (selected_module == 0) // Interface
    {
        L.Section(pstra("Menu Controls"), 2);
        RowHotkey(L, pstra("Open / close key"), pstra("##df_menu_key"), &settings.ShowMenuKey,
                  HK_MOUSE_ALL & ~HK_MOUSE1);
        RowHotkey(L, pstra("Panic key"), pstra("##df_panic_key"), &settings.Interface.PanicKey);

        L.y += 10.0f;
        L.Section(pstra("Interface Scaling"), 2);
        RowSlider(L, pstra("UI scale"), pstra("##df_uiscale"), &settings.Interface.UiScale, 0.7f, 1.8f, pstra("%.2fx"));
        RowSlider(L, pstra("Animation speed"), pstra("##df_anim"), &settings.Interface.AnimationSpeed, 0.1f, 3.0f, pstra("%.2fx"));

        R.Section(pstra("HUD Panels"), 4);
        RowToggle(R, pstra("Watermark HUD"), pstra("##df_hud_wm"), &S.hud_watermark, false);
        RowToggle(R, pstra("Target HUD"), pstra("##df_hud_target"), &S.hud_target, false);
        RowToggle(R, pstra("Coordinates HUD"), pstra("##df_hud_coords"), &S.hud_coords, false);
        RowToggle(R, pstra("Notifications"), pstra("##df_hud_notify"), &S.hud_notify, false);

        R.y += 10.0f;
        R.Section(pstra("Styling & Effects"), 3);
        RowToggle(R, pstra("Glass highlight"), pstra("##df_glass"), &settings.Interface.Glass, false);
        RowToggle(R, pstra("Drop shadows"), pstra("##df_shadow"), &settings.Interface.Shadows, false);
        RowToggle(R, pstra("Wallpaper background"), pstra("##df_wallpaper"), &settings.Interface.Wallpaper, false);
    }
    else // White Label & Automation
    {
        L.Section(pstra("White Label Status"), 2);
        L.Row(IsWhiteLabel() ? pstra("White Label: ACTIVE") : pstra("White Label: STANDARD"), false);
        char brandBuf[64];
        snprintf(brandBuf, sizeof(brandBuf), pstra("Brand: %s"), WhiteLabelName());
        L.Row(brandBuf, true);

        L.y += 10.0f;
        L.Section(pstra("Theme Colors"), 4);
        RowColor(L, pstra("Accent Color"), pstra("##th_accent_col"), (ImVec4*)&settings.Interface.AccentColor.Value);
        RowColor(L, pstra("Secondary Color"), pstra("##th_grad_col"), (ImVec4*)&settings.Interface.GradientEnd.Value);
        RowColor(L, pstra("Background Panel"), pstra("##th_panel_col"), (ImVec4*)&settings.Interface.PanelColor.Value);
        RowColor(L, pstra("Text Color"), pstra("##th_text_col"), (ImVec4*)&settings.Interface.TextColor.Value);

        R.Section(pstra("Auto-Login & Account"), 4);
        const auto& loginCfg = ShaiyaOverlay::AutoLoginManager::GetConfig();
        char userBuf[64];
        snprintf(userBuf, sizeof(userBuf), pstra("Account: %s"), loginCfg.Username);
        R.Row(userBuf, false);

        char stateBuf[64];
        snprintf(stateBuf, sizeof(stateBuf), pstra("State: %s"), ShaiyaOverlay::AutoLoginManager::GetGameStateName(ShaiyaOverlay::AutoLoginManager::GetCurrentGameState()));
        R.Row(stateBuf, true);

        char statusBuf[64];
        snprintf(statusBuf, sizeof(statusBuf), pstra("Status: %s"), ShaiyaOverlay::AutoLoginManager::GetStatusMessage());
        R.Row(statusBuf, true);

        R.Row(pstra("RSA Handshake: Confirmed"), true);
    }

    content_h = ImMax(L.y, R.y) - base_y;
    dl->PopClipRect();

    if (content_h > view_h)
    {
        float track_x = max.x - 7.0f;
        float ratio = view_h / content_h;
        float bar_h = ImMax(view_h * ratio, 24.0f);
        float bar_y = view_y0 + (view_h - bar_h) * (scroll / ImMax(content_h - view_h, 1.0f));
        RectFilled(dl, ImVec2(track_x - 1.5f, view_y0), ImVec2(track_x + 1.5f, view_y1),
                          C.separator, 1.5f);
        RectFilled(dl, ImVec2(track_x - 1.5f, bar_y), ImVec2(track_x + 1.5f, bar_y + bar_h),
                          C.scroll, 1.5f);
    }
    (void)min;
}

static void DrawSearchResults(ImDrawList* dl)
{
    if (!S.search_focus) return;

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    bool overBox = (mouse.x >= g_search_min.x && mouse.x <= g_search_max.x &&
                    mouse.y >= g_search_min.y && mouse.y <= g_search_max.y);

    if (!S.search[0])
    {
        if (ImGui::IsMouseClicked(0) && !overBox) S.search_focus = false;
        return;
    }

    int idx[24]; int n = 0;
    for (int i = 0; i < kSearchCount && n < 24; i++)
        if (SearchMatch(kSearch[i].name, S.search)) idx[n++] = i;

    const float w = g_search_max.x - g_search_min.x;
    const float row_h = 30.0f;
    const float x = g_search_min.x;
    const float y = g_search_max.y + 5.0f;

    float h = (n > 0 ? n * row_h : row_h) + 8.0f;
    ImVec2 pmin(x, y), pmax(x + w, y + h);
    Shadow(dl, pmin, pmax, 10.0f, 14.0f);
    RectFilled(dl, pmin, pmax, C.panel_solid, 10.0f);
    RectStroke(dl, pmin, pmax, C.border, 10.0f);

    if (n == 0)
    {
        TextAt(dl, F_Body, ImVec2(x + 16.0f, y + (row_h - Measure(F_Body, pstra("x")).y) * 0.5f + 4.0f),
               C.text_mute, pstra("Nenhum resultado"));
    }
    else for (int k = 0; k < n; k++)
    {
        const SearchItem& it = kSearch[idx[k]];
        ImVec2 rmin(x + 6.0f, y + 4.0f + k * row_h), rmax(x + w - 6.0f, y + 4.0f + (k + 1) * row_h - 2.0f);

        char id[32]; snprintf(id, sizeof(id), pstra("##sr%d"), idx[k]);
        bool hov = false;
        bool click = Hitbox(id, rmin, rmax, &hov);
        if (hov) RectFilled(dl, rmin, rmax, C.row, 7.0f);

        float rcy = (rmin.y + rmax.y) * 0.5f;
        DrawIcon(dl, IC_SEARCH, ImVec2(rmin.x + 14.0f, rcy), 12.0f, C.text_mute, 1.2f);
        TextAt(dl, F_Body, ImVec2(rmin.x + 30.0f, rcy - Measure(F_Body, it.name).y * 0.5f),
               hov ? C.text : C.text_dim, it.name);
        const char* tabname = kTabs[it.tab].label;
        TextRight(dl, F_Small, ImVec2(rmax.x - 12.0f, rcy - Measure(F_Small, tabname).y * 0.5f),
                  C.text_mute, tabname);

        if (click)
        {
            S.menu_tab = it.tab;
            S.module_sel[it.tab] = it.module;
            S.search[0] = 0;
            S.search_focus = false;
        }
    }

    bool overPanel = (mouse.x >= pmin.x && mouse.x <= pmax.x && mouse.y >= pmin.y && mouse.y <= pmax.y);
    if (ImGui::IsMouseClicked(0) && !overBox && !overPanel) S.search_focus = false;
}

static void GlitchLogo(ImDrawList* dl, ImVec2 c, float target_w, float a, float t)
{
    int A = (int)(255.0f * ImClamp(a, 0.0f, 1.0f));
    if (A <= 0) return;

    if (TexLogoStack && TexLogoStackSz.y > 0.0f)
    {
        float ar = TexLogoStackSz.x / TexLogoStackSz.y;
        float lw = target_w, lh = lw / ar;
        float visual_offset_x = lw * 0.072f;
        ImVec2 mn(c.x + visual_offset_x - lw * 0.5f, c.y - lh * 0.5f);
        ImVec2 mx(c.x + visual_offset_x + lw * 0.5f, c.y + lh * 0.5f);

        dl->AddImage(TexLogoStack, mn, mx, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, A));
    }
    else
    {
        ImVec2 ts = Measure(F_Head, pstra("PROJECT"));
        ImVec2 p(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f);
        TextAt(dl, F_Head, p, IM_COL32(255, 255, 255, A), pstra("PROJECT"));
    }

    (void)t;
}

static void DrawSplash(ImDrawList* dl, ImVec2 min, ImVec2 max, float a, float logo_a, float t)
{
    dl->PushClipRect(min, max, true);
    Settings::sInterface& ui = Ui();
    float r = 16.0f * ImClamp(ui.CornerRounding, 0.0f, 1.8f);

    RectFilled(dl, min, max, ColorU32(ui.PanelColor, a), r);

    for (float y = min.y + 2.0f; y < max.y; y += 3.0f)
        dl->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), IM_COL32(0, 0, 0, (int)(26 * a)), 1.0f);
    if (logo_a > 0.001f)
        GlitchLogo(dl, ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f), (max.x - min.x) * 0.40f, logo_a, t);
    dl->PopClipRect();
}

static void SplashHalf(ImDrawList* dl, ImVec2 min, ImVec2 max, float W, float H,
                       float thr, float sign, ImU32 col)
{
    ImVec2 rect[4] = { min, ImVec2(max.x, min.y), max, ImVec2(min.x, max.y) };
    ImVec2 poly[8]; int n = 0;
    for (int i = 0; i < 4; i++)
    {
        ImVec2 a = rect[i], b = rect[(i + 1) & 3];
        float fa = sign * ((a.x - min.x) / W + (a.y - min.y) / H - thr);
        float fb = sign * ((b.x - min.x) / W + (b.y - min.y) / H - thr);
        if (fa >= 0.0f) poly[n++] = a;
        if ((fa >= 0.0f) != (fb >= 0.0f))
        {
            float u = fa / (fa - fb);
            poly[n++] = ImVec2(a.x + u * (b.x - a.x), a.y + u * (b.y - a.y));
        }
    }
    if (n >= 3) dl->AddConvexPolyFilled(poly, n, col);
}

static void DrawSplashWipe(ImDrawList* dl, ImVec2 min, ImVec2 max, float t, float st)
{
    dl->PushClipRect(min, max, true);

    const float W = max.x - min.x, H = max.y - min.y;
    float e = 1.0f - (1.0f - t) * (1.0f - t);
    ImU32 pc = ColorU32(Ui().PanelColor);

    SplashHalf(dl, min, max, W, H, 1.0f - e, -1.0f, pc);
    SplashHalf(dl, min, max, W, H, 1.0f + e, +1.0f, pc);

    (void)st;
    dl->PopClipRect();
}

static float g_splash_t0    = -100.0f;
static bool  g_splash_intro = false;

void Blade::ArmSplash()
{
    g_splash_t0 = (float)ImGui::GetTime();
    g_splash_intro = false;
}

static void DrawWhiteLabelNavigation(ImDrawList* dl, ImVec2 min, float width, float height,
                                     float* content_x, float* content_y, float* content_w)
{
    const float header_h = 50.0f;
    const float pad = 16.0f;
    const float nav_w = 170.0f;
    const float tab_h = 34.0f;
    const float tab_gap = 7.0f;
    const bool has_logo = TexWhiteLabelLogo && TexWhiteLabelLogoSz.x > 0.0f && TexWhiteLabelLogoSz.y > 0.0f;
    const float logo_area_h = has_logo ? 154.0f : 0.0f;
    const char* name = WhiteLabelName();
    ProtectedText fallback(pstra("CONTROL"));
    if (!name || !name[0])
        name = fallback;

    TextAt(dl, F_Head, ImVec2(min.x + pad, min.y + header_h * 0.5f - Measure(F_Head, name).y * 0.5f), C.text, name);
    //TextRight(dl, F_Small, ImVec2(min.x + width - pad, min.y + header_h * 0.5f - Measure(F_Small, pstra("WHITE LABEL")).y * 0.5f),
    //          C.text_mute, pstra("WHITE LABEL"));
    dl->AddLine(ImVec2(min.x + pad, min.y + header_h), ImVec2(min.x + width - pad, min.y + header_h), C.divider, 1.0f);
    dl->AddLine(ImVec2(min.x + nav_w, min.y + header_h + 8.0f), ImVec2(min.x + nav_w, min.y + height - 10.0f), C.divider, 1.0f);

    if (has_logo)
    {
        const float max_w = nav_w - 8.0f;
        const float max_h = logo_area_h - 10.0f;
        const float aspect = TexWhiteLabelLogoSz.x / TexWhiteLabelLogoSz.y;
        float logo_w = max_w;
        float logo_h = logo_w / aspect;
        if (logo_h > max_h)
        {
            logo_h = max_h;
            logo_w = logo_h * aspect;
        }

        const ImVec2 center(min.x + nav_w * 0.5f, min.y + header_h + logo_area_h * 0.5f);
        dl->AddImage(TexWhiteLabelLogo,
                     ImVec2(center.x - logo_w * 0.5f, center.y - logo_h * 0.5f),
                     ImVec2(center.x + logo_w * 0.5f, center.y + logo_h * 0.5f),
                     TexWhiteLabelLogoUV0, TexWhiteLabelLogoUV1);
    }

    const float tab_x = min.x + pad;
    const float tab_y = min.y + header_h + logo_area_h + 14.0f;
    const float tab_w = nav_w - pad * 2.0f;
    for (int index = 0; index < kTabCount; ++index)
    {
        const float y = tab_y + index * (tab_h + tab_gap);
        const ImVec2 tab_min(tab_x, y);
        const ImVec2 tab_max(tab_x + tab_w, y + tab_h);
        bool hovered = false;
        char id[40];
        snprintf(id, sizeof(id), pstra("##wl_nav%d"), index);
        if (Hitbox(id, tab_min, tab_max, &hovered))
            S.menu_tab = index;

        const bool selected = S.menu_tab == index;
        RectFilled(dl, tab_min, tab_max, selected ? Accent(1.0f) : (hovered ? C.row_hover : C.row), 3.0f);
        RectStroke(dl, tab_min, tab_max, selected ? Accent(1.0f) : C.border, 3.0f);
        DrawTabIcon(dl, index, kTabs[index].ic, ImVec2(tab_min.x + 16.0f, (tab_min.y + tab_max.y) * 0.5f),
                    14.0f, selected ? C.panel : C.text_dim);
        TextAt(dl, F_Body, ImVec2(tab_min.x + 28.0f, (tab_min.y + tab_max.y) * 0.5f - Measure(F_Body, kTabs[index].label).y * 0.5f),
               selected ? C.panel : C.text, kTabs[index].label);
    }

    *content_x = min.x + nav_w;
    *content_y = min.y + header_h;
    *content_w = width - nav_w;
}

static float DrawWhiteLabelModules(ImDrawList* dl, float x, float y, int tab)
{
    int module_count = 0;
    const Module* modules = GetModules(tab, &module_count);
    if (module_count <= 1)
        return y;

    const float module_h = 28.0f;
    float module_x = x;
    for (int index = 0; index < module_count; ++index)
    {
        const float label_w = Measure(F_Small, modules[index].name).x;
        const float module_w = label_w + 24.0f;
        const ImVec2 module_min(module_x, y);
        const ImVec2 module_max(module_x + module_w, y + module_h);
        bool hovered = false;
        char id[40];
        snprintf(id, sizeof(id), pstra("##wl_module%d_%d"), tab, index);
        if (Hitbox(id, module_min, module_max, &hovered))
            S.module_sel[tab] = index;

        const bool selected = S.module_sel[tab] == index;
        RectFilled(dl, module_min, module_max, selected ? C.sel : (hovered ? C.row : C.panel), 3.0f);
        RectStroke(dl, module_min, module_max, selected ? Accent(1.0f) : C.border, 3.0f);
        TextCentered(dl, F_Small, ImVec2((module_min.x + module_max.x) * 0.5f, (module_min.y + module_max.y) * 0.5f),
                     selected ? C.text : C.text_dim, modules[index].name);
        module_x += module_w + 7.0f;
    }
    return y + module_h + 8.0f;
}

static void DrawWhiteLabelMenu(ImDrawList* dl, float menu_a)
{
    S.menu_tab = ImClamp(S.menu_tab, 0, kTabCount - 1);
    const int menu_v0 = dl->VtxBuffer.Size;
    const ImVec2 screen = Screen();
    const ImVec2 def(floorf(screen.x * 0.5f - WL_MENU_W * 0.5f), floorf(screen.y * 0.5f - WL_MENU_H * 0.5f));
    ImVec2 min = HudBegin(pstra("blade_menu"), def);
    min.x = floorf(min.x);
    min.y = floorf(min.y);
    const ImVec2 max(min.x + WL_MENU_W, min.y + WL_MENU_H);

    RectFilled(dl, min, max, C.panel, 6.0f);
    RectStroke(dl, min, max, C.border, 6.0f);
    RectFilled(dl, min, ImVec2(max.x, min.y + 4.0f), Accent(1.0f), 6.0f, ImDrawCornerFlags_Top);

    float content_x = min.x;
    float content_y = min.y;
    float content_w = WL_MENU_W;
    DrawWhiteLabelNavigation(dl, min, WL_MENU_W, WL_MENU_H, &content_x, &content_y, &content_w);

    const float cx0 = content_x;
    const float title_y = content_y;
    float cy0 = title_y + 12.0f;
    const char* title = kTabs[S.menu_tab].label;
    TextAt(dl, F_Title, ImVec2(cx0 + CONT_PAD, title_y + 8.0f), C.text, title);
    dl->AddLine(ImVec2(cx0 + CONT_PAD, title_y + 27.0f), ImVec2(max.x - CONT_PAD, title_y + 27.0f), C.divider, 1.0f);

    int module_count = 0;
    GetModules(S.menu_tab, &module_count);
    if (module_count > 1)
    {
        const float modules_y = title_y + 35.0f;
        const float content_y_after_modules = DrawWhiteLabelModules(dl, cx0 + CONT_PAD, modules_y, S.menu_tab);
        cy0 = content_y_after_modules - 28.0f + 8.0f;
    }

    if (S.menu_tab == TAB_COMBAT)       DrawAimbotContent(dl, min, max, cx0, cy0, content_w, CONT_PAD);
    else if (S.menu_tab == TAB_VISUALS) DrawVisualsContent(dl, min, max, cx0, cy0, content_w, CONT_PAD);
    else if (S.menu_tab == TAB_LOOT)    DrawLootContent(dl, min, max, cx0, cy0, content_w);
    else if (S.menu_tab == TAB_SUPPORT) DrawToolsContent(dl, min, max, cx0, cy0, content_w);
    else                                DrawSettingsContent(dl, min, max, cx0, cy0, content_w);

    HudEnd(pstra("blade_menu"), ImVec2(min.x, min.y), ImVec2(max.x, min.y + 50.0f), WL_MENU_H);
    FadeDrawListAlpha(dl, menu_v0, menu_a);
}

void Blade::DrawMainMenu(ImDrawList* dl, float menu_a)
{
    if (menu_a <= 0.01f) return;
    if (IsWhiteLabel())
    {
        DrawWhiteLabelMenu(dl, menu_a);
        return;
    }

    S.menu_tab = ImClamp(S.menu_tab, 0, kTabCount - 1);
    const int menu_v0 = dl->VtxBuffer.Size;

    const float now_t = (float)ImGui::GetTime();

    const float GLITCH_DUR = 1.0f;
    Settings::sInterface& ui = Ui();
    const float SPLIT_DUR  = ImClamp(ui.SplashOpen, 0.3f, 3.0f);
    float st = now_t - g_splash_t0;
    bool  splashing = ui.IntroAnimation && st < (GLITCH_DUR + SPLIT_DUR);

    static int last_tab = -1, last_mod = -1;
    int cur_mod = S.module_sel[S.menu_tab];
    if (S.menu_tab != last_tab || cur_mod != last_mod)
    {
        if (!splashing) TriggerIntro();
        last_tab = S.menu_tab;
        last_mod = cur_mod;
    }

    ImVec2 ds = Screen();

    ImVec2 def(floorf(ds.x * 0.5f - MENU_W * 0.5f - 24.0f), floorf(ds.y * 0.5f - MENU_H * 0.5f - 30.0f));
    ImVec2 min = HudBegin(pstra("blade_menu"), def);
    min.x = floorf(min.x); min.y = floorf(min.y);
    ImVec2 max(min.x + MENU_W, min.y + MENU_H);

    {
        float r = 16.0f * ImClamp(ui.CornerRounding, 0.0f, 1.8f);

        if (ui.Shadows) Shadow(dl, min, max, r, 12.0f);

        dl->AddRectFilled(min, max, OpaquePanelColor(0.000f), r, ImDrawCornerFlags_All);
        dl->AddRectFilled(min, ImVec2(min.x + SIDEBAR_W, max.y), OpaquePanelColor(-0.018f), r,
                          ImDrawCornerFlags_Left);

        RectFilled(dl, min, max, C.panel, r);
        RectFilled(dl, min, ImVec2(min.x + SIDEBAR_W, max.y), C.sidebar, r,
                   ImDrawCornerFlags_Left);

        if (ui.Glass)
            FadeTopRect(dl, min, ImVec2(max.x, min.y + 28.0f), IM_COL32(255, 255, 255, 8), r);

        RectStroke(dl, min, max, C.border, r, ImDrawCornerFlags_All, 1.0f);
    }

    DrawTopbar(dl, min, MENU_W);
    DrawSidebar(dl, min, MENU_H);

    dl->AddLine(ImVec2(min.x + SIDEBAR_W, min.y + 8.0f), ImVec2(min.x + SIDEBAR_W, max.y - 8.0f),
                C.separator, 1.0f);

    const float cx0 = min.x + SIDEBAR_W;
    const float cy0 = min.y + TOPBAR_H;
    const float cw  = MENU_W - SIDEBAR_W;

    DrawBreadcrumb(dl, cx0, cy0, cw);

    if (S.menu_tab == TAB_COMBAT)        DrawAimbotContent(dl, min, max, cx0, cy0, cw, CONT_PAD);
    else if (S.menu_tab == TAB_VISUALS)  DrawVisualsContent(dl, min, max, cx0, cy0, cw, CONT_PAD);
    else if (S.menu_tab == TAB_LOOT)     DrawLootContent(dl, min, max, cx0, cy0, cw);
    else if (S.menu_tab == TAB_SUPPORT)  DrawToolsContent(dl, min, max, cx0, cy0, cw);
    else                                 DrawSettingsContent(dl, min, max, cx0, cy0, cw);

    DrawSearchResults(dl);

    if (splashing)
    {
        if (st >= GLITCH_DUR && !g_splash_intro) { TriggerIntro(); g_splash_intro = true; }
        if (st < GLITCH_DUR)
        {

            float logo_a = ImClamp((GLITCH_DUR - st) / 0.35f, 0.0f, 1.0f);
            DrawSplash(dl, min, max, 1.0f, logo_a, st);
        }
        else
            DrawSplashWipe(dl, min, max, ImClamp((st - GLITCH_DUR) / SPLIT_DUR, 0.0f, 1.0f), st);
    }

    HudEnd(pstra("blade_menu"), ImVec2(min.x, min.y), ImVec2(max.x, min.y + TOPBAR_H), MENU_H);

    FadeDrawListAlpha(dl, menu_v0, menu_a);
}
