#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

using namespace Blade;

static Settings::sInterface& Ui()
{
    return Settings::Get().Interface;
}

static float* ColorPtr(ImColor& color)
{
    return &color.Value.x;
}

static ImU32 ColorU32(const ImColor& color, float alpha = 1.0f)
{
    ImVec4 value = color.Value;
    value.w = alpha;
    return ImGui::GetColorU32(value);
}

const Blade::ItemDef Blade::kItems[] = {
    { pstra("Weapon"),         IM_COL32(232, 228,  90, 255) },
    { pstra("Ammo"),           IM_COL32( 96, 198, 192, 255) },
    { pstra("Medical"),        IM_COL32(200, 154,  86, 255) },
    { pstra("Armor"),          IM_COL32(242, 203,  70, 255) },
    { pstra("Backpack"),       IM_COL32( 52,  86, 200, 255) },
    { pstra("Attachment"),     IM_COL32( 47, 212, 107, 255) },
};
const int Blade::kItemCount = IM_ARRAYSIZE(kItems);

static const Module kCombat[] = {
    { pstra("Auto-Combo"), nullptr, 0 },
    { pstra("Grind Bot"), nullptr, 0 },
};
static const Module kVisuals[] = {
    { pstra("Entity ESP"), nullptr, 0 },
    { pstra("Skin Changer"), nullptr, 0 },
    { pstra("Loot Snaplines"), nullptr, 0 },
    { pstra("Quest Waypoints"), nullptr, 0 },
};
static const Module kLoot[] = {
    { pstra("Auto-Loot"), nullptr, 0 },
    { pstra("Inventory"), nullptr, 0 },
};
static const Module kSupport[] = {
    { pstra("Auto-Heal"), nullptr, 0 },
    { pstra("Auto-Buff"), nullptr, 0 },
};
static const Module kSettings[] = {
    { pstra("Interface"), nullptr, 0 },
    { pstra("White Label"), nullptr, 0 },
};

const Blade::Module* Blade::GetModules(int tab, int* out_count)
{
    switch (tab)
    {
    case 0: *out_count = IM_ARRAYSIZE(kCombat);   return kCombat;
    case 1: *out_count = IM_ARRAYSIZE(kVisuals);  return kVisuals;
    case 2: *out_count = IM_ARRAYSIZE(kLoot);     return kLoot;
    case 3: *out_count = IM_ARRAYSIZE(kSupport);  return kSupport;
    default: *out_count = IM_ARRAYSIZE(kSettings); return kSettings;
    }
}

static void PopupPanel(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding = 12.0f)
{
    Settings::sInterface& ui = Ui();
    const bool white_label = IsWhiteLabel();
    float r = white_label ? 4.0f : rounding * ImClamp(ui.CornerRounding, 0.0f, 1.8f);

    ImVec4 panel = ui.PanelColor.Value;
    ImVec4 pn(ImMin(panel.x + 0.02f, 1.0f),
              ImMin(panel.y + 0.02f, 1.0f),
              ImMin(panel.z + 0.024f, 1.0f), 1.0f);

    if (ui.Shadows && !white_label)
        Shadow(dl, min, max, r, 18.0f);

    RectFilled(dl, min, max, ImGui::GetColorU32(pn), r);

    if (ui.Glass && !white_label)
        FadeTopRect(dl, min, ImVec2(max.x, min.y + 26.0f), IM_COL32(255, 255, 255, 10), r);

    RectStroke(dl, min, max, C.border, r, ImDrawCornerFlags_All, 1.0f);
}

static bool ListRow(ImDrawList* dl, const char* id, ImVec2 min, ImVec2 max,
                    const char* label, bool selected, ImFont* font = nullptr)
{
    bool hov = false;
    bool clicked = Hitbox(id, min, max, &hov);

    const float rounding = IsWhiteLabel() ? 3.0f : 8.0f;
    if (selected)      RectFilled(dl, min, max, IsWhiteLabel() ? Accent(1.0f) : C.sel, rounding);
    else if (hov)      RectFilled(dl, min, max, C.row, rounding);
    if (selected)
    {
        if (IsWhiteLabel())
            DrawIcon(dl, IC_CHECK, ImVec2(max.x - 14.0f, (min.y + max.y) * 0.5f), 12.0f, C.panel, 1.5f);
        else
            RectFilled(dl, ImVec2(min.x - 7.0f, min.y + 4.0f), ImVec2(min.x - 3.5f, max.y - 4.0f),
                       Accent(1.0f), 2.0f);
    }

    ImFont* f = font ? font : F_Med;
    ImVec2 ts = Measure(f, label);
    TextAt(dl, f, ImVec2(min.x + 12.0f, (min.y + max.y) * 0.5f - ts.y * 0.5f),
           selected && IsWhiteLabel() ? C.panel : (selected ? C.text : (hov ? C.text : C.text_dim)), label);

    return clicked;
}

static void PopupDropdown(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    const float item_h = 30.0f;
    const float pad = 5.0f;
    float w = ImMax(P.amax.x - P.amin.x, 112.0f);
    float h = P.count * item_h + pad * 2.0f;

    ImVec2 ds = Screen();
    float x = P.amin.x;
    float y = P.amax.y + 5.0f;
    if (y + h > ds.y - 8.0f) y = P.amin.y - 5.0f - h;
    if (x + w > ds.x - 8.0f) x = ds.x - 8.0f - w;

    float a = IsWhiteLabel() ? 1.0f : P.anim;
    y -= (1.0f - a) * 6.0f;

    ImVec2 min(x, y), max(x + w, y + h);
    PopupPanel(dl, min, max, 10.0f);

    int* value = (int*)P.data;
    for (int i = 0; i < P.count; i++)
    {
        ImVec2 rmin(min.x + pad + 4.0f, min.y + pad + i * item_h);
        ImVec2 rmax(max.x - pad, rmin.y + item_h - 1.0f);

        char id[48];
        snprintf(id, sizeof(id), pstra("##dd_%d"), i);
        if (ListRow(dl, id, rmin, rmax, P.opts[i], value && *value == i, F_Body))
        {
            if (value) *value = i;
            ClosePopup();
        }
    }

    *out_min = min; *out_max = max;
}

static void CheckerBoard(ImDrawList* dl, ImVec2 min, ImVec2 max, float cell)
{
    dl->PushClipRect(min, max, true);
    int nx = (int)((max.x - min.x) / cell) + 1;
    int ny = (int)((max.y - min.y) / cell) + 1;
    for (int y = 0; y < ny; y++)
        for (int x = 0; x < nx; x++)
        {
            ImU32 c = ((x + y) & 1) ? IM_COL32(120, 120, 128, 255) : IM_COL32(200, 200, 208, 255);
            RectFilled(dl, ImVec2(min.x + x * cell, min.y + y * cell),
                              ImVec2(min.x + (x + 1) * cell, min.y + (y + 1) * cell), c);
        }
    dl->PopClipRect();
}

static void DrawDropper(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
{
    float k = s * 0.5f;
    ImVec2 tip(c.x - k, c.y + k);
    ImVec2 neck(c.x + k * 0.15f, c.y - k * 0.15f);
    dl->AddLine(tip, neck, col, 2.2f);

    dl->AddTriangleFilled(tip, ImVec2(tip.x + 5.0f, tip.y - 1.5f),
                          ImVec2(tip.x + 1.5f, tip.y - 5.0f), col);

    dl->AddCircleFilled(ImVec2(c.x + k * 0.6f, c.y - k * 0.6f), 3.4f, col, 12);
}

static void PopupColorWhiteLabel(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    float* col = CP.col;
    if (!col) { CloseColorPicker(); return; }

    static ImGuiID cached_for = 0;
    static float hue = 0.0f;
    static float saturation = 1.0f;
    static float value = 1.0f;
    if (cached_for != CP.id)
    {
        ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], hue, saturation, value);
        cached_for = CP.id;
    }

    const float width = 252.0f;
    const float pad = 12.0f;
    const float header_h = 36.0f;
    const float color_h = 142.0f;
    const float hue_w = 14.0f;
    const float alpha_h = 10.0f;
    const float swatch_h = 24.0f;
    const float height = header_h + color_h + alpha_h + swatch_h + pad * 4.0f;

    ImVec2 ds = Screen();
    if (!CP.placed)
    {
        CP.pos = ImVec2(
            ImClamp(CP.anchor_min.x - width * 0.5f + (CP.anchor_max.x - CP.anchor_min.x) * 0.5f,
                    12.0f, ds.x - width - 12.0f),
            ImClamp(CP.anchor_max.y + 8.0f, 12.0f, ds.y - height - 12.0f));
        CP.placed = true;
    }
    CP.pos.x = ImClamp(CP.pos.x, 8.0f, ImMax(ds.x - width - 8.0f, 8.0f));
    CP.pos.y = ImClamp(CP.pos.y, 8.0f, ImMax(ds.y - height - 8.0f, 8.0f));

    const ImVec2 min(CP.pos.x, CP.pos.y);
    const ImVec2 max(min.x + width, min.y + height);
    PopupPanel(dl, min, max, 4.0f);

    const ImU32 current = ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], 1.0f));
    RectFilled(dl, ImVec2(min.x + pad, min.y + 10.0f), ImVec2(min.x + pad + 16.0f, min.y + 26.0f), current, 2.0f);
    TextAt(dl, F_Med, ImVec2(min.x + pad + 26.0f, min.y + header_h * 0.5f - Measure(F_Med, CP.label).y * 0.5f), C.text, CP.label);
    if (IconButton(pstra("##wl_cp_close"), IC_CLOSE, ImVec2(max.x - 18.0f, min.y + header_h * 0.5f), 12.0f, C.text_dim, 10.0f))
    {
        CloseColorPicker();
        return;
    }
    dl->AddLine(ImVec2(min.x + pad, min.y + header_h), ImVec2(max.x - pad, min.y + header_h), C.divider, 1.0f);

    const ImVec2 sv_min(min.x + pad, min.y + header_h + pad);
    const ImVec2 sv_max(max.x - pad - hue_w - 8.0f, sv_min.y + color_h);
    float hue_r, hue_g, hue_b;
    ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, hue_r, hue_g, hue_b);
    const ImU32 hue_color = ImGui::GetColorU32(ImVec4(hue_r, hue_g, hue_b, 1.0f));
    dl->AddRectFilledMultiColor(sv_min, sv_max, IM_COL32_WHITE, hue_color, hue_color, IM_COL32_WHITE);
    dl->AddRectFilledMultiColor(sv_min, sv_max, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
                                IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 255));
    RectStroke(dl, sv_min, sv_max, C.border, 2.0f);

    Hitbox(pstra("##wl_cp_sv"), sv_min, sv_max);
    const ImGuiID sv_id = ImGui::GetCurrentWindow()->GetID(pstra("##wl_cp_sv"));
    if (GImGui->ActiveId == sv_id && ImGui::IsMouseDown(0))
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        saturation = ImClamp((mouse.x - sv_min.x) / ImMax(sv_max.x - sv_min.x, 1.0f), 0.0f, 1.0f);
        value = 1.0f - ImClamp((mouse.y - sv_min.y) / ImMax(sv_max.y - sv_min.y, 1.0f), 0.0f, 1.0f);
    }

    const ImVec2 cursor(sv_min.x + saturation * (sv_max.x - sv_min.x), sv_min.y + (1.0f - value) * (sv_max.y - sv_min.y));
    dl->AddRect(cursor - ImVec2(4.0f, 4.0f), cursor + ImVec2(4.0f, 4.0f), IM_COL32(255, 255, 255, 255), 1.0f);

    const ImVec2 hue_min(sv_max.x + 8.0f, sv_min.y);
    const ImVec2 hue_max(hue_min.x + hue_w, sv_max.y);
    constexpr int hue_steps = 48;
    for (int index = 0; index < hue_steps; ++index)
    {
        const float t0 = static_cast<float>(index) / static_cast<float>(hue_steps);
        const float t1 = static_cast<float>(index + 1) / static_cast<float>(hue_steps);
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(t0, 1.0f, 1.0f, r, g, b);
        RectFilled(dl, ImVec2(hue_min.x, ImLerp(hue_min.y, hue_max.y, t0)),
                   ImVec2(hue_max.x, ImLerp(hue_min.y, hue_max.y, t1) + 1.0f), ImGui::GetColorU32(ImVec4(r, g, b, 1.0f)));
    }
    RectStroke(dl, hue_min, hue_max, C.border, 2.0f);
    Hitbox(pstra("##wl_cp_hue"), hue_min, hue_max);
    const ImGuiID hue_id = ImGui::GetCurrentWindow()->GetID(pstra("##wl_cp_hue"));
    if (GImGui->ActiveId == hue_id && ImGui::IsMouseDown(0))
        hue = ImClamp((ImGui::GetIO().MousePos.y - hue_min.y) / ImMax(hue_max.y - hue_min.y, 1.0f), 0.0f, 0.9999f);

    const float hue_y = ImLerp(hue_min.y, hue_max.y, hue);
    dl->AddLine(ImVec2(hue_min.x - 2.0f, hue_y), ImVec2(hue_max.x + 2.0f, hue_y), IM_COL32(255, 255, 255, 255), 2.0f);

    ImGui::ColorConvertHSVtoRGB(hue, saturation, value, col[0], col[1], col[2]);

    const ImVec2 alpha_min(min.x + pad, sv_max.y + pad);
    const ImVec2 alpha_max(max.x - pad, alpha_min.y + alpha_h);
    CheckerBoard(dl, alpha_min, alpha_max, 5.0f);
    dl->AddRectFilledMultiColor(alpha_min, alpha_max, IM_COL32((int)(col[0] * 255.0f), (int)(col[1] * 255.0f), (int)(col[2] * 255.0f), 0),
                                current, current, IM_COL32((int)(col[0] * 255.0f), (int)(col[1] * 255.0f), (int)(col[2] * 255.0f), 0));
    RectStroke(dl, alpha_min, alpha_max, C.border, 2.0f);
    Hitbox(pstra("##wl_cp_alpha"), alpha_min, alpha_max);
    const ImGuiID alpha_id = ImGui::GetCurrentWindow()->GetID(pstra("##wl_cp_alpha"));
    if (GImGui->ActiveId == alpha_id && ImGui::IsMouseDown(0))
        col[3] = ImClamp((ImGui::GetIO().MousePos.x - alpha_min.x) / ImMax(alpha_max.x - alpha_min.x, 1.0f), 0.0f, 1.0f);
    const float alpha_x = ImLerp(alpha_min.x, alpha_max.x, col[3]);
    dl->AddLine(ImVec2(alpha_x, alpha_min.y - 2.0f), ImVec2(alpha_x, alpha_max.y + 2.0f), C.text, 2.0f);

    const ImColor& primary = Settings::Get().WhiteLabelTheme.PrimaryColor;
    const ImColor& secondary = Settings::Get().WhiteLabelTheme.SecondaryColor;
    const ImColor& text = Settings::Get().WhiteLabelTheme.TextColor;
    const ImColor& background = Settings::Get().WhiteLabelTheme.BackgroundColor;
    const ImColor swatches[] = { primary, secondary, text, background };
    const float swatch_y = alpha_max.y + pad;
    const float swatch_w = (alpha_max.x - alpha_min.x - 18.0f) / 4.0f;
    for (int index = 0; index < 4; ++index)
    {
        const ImVec2 sw_min(alpha_min.x + index * (swatch_w + 6.0f), swatch_y);
        const ImVec2 sw_max(sw_min.x + swatch_w, sw_min.y + swatch_h);
        char id[24];
        snprintf(id, sizeof(id), pstra("##wl_cp_theme%d"), index);
        if (Hitbox(id, sw_min, sw_max))
        {
            col[0] = swatches[index].Value.x;
            col[1] = swatches[index].Value.y;
            col[2] = swatches[index].Value.z;
            ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], hue, saturation, value);
        }
        RectFilled(dl, sw_min, sw_max, ImGui::GetColorU32(swatches[index].Value), 2.0f);
        RectStroke(dl, sw_min, sw_max, C.border, 2.0f);
    }

    *out_min = min;
    *out_max = max;
}

static void PopupColor(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    if (IsWhiteLabel())
    {
        PopupColorWhiteLabel(dl, out_min, out_max);
        return;
    }

    float* col = CP.col;
    if (!col) { CloseColorPicker(); return; }

    static ImGuiID cached_for = 0;
    static float H = 0.0f, Sv = 1.0f, V = 1.0f;
    static bool  hex_edit = false;
    static char  hex_buf[8]{};
    if (cached_for != CP.id)
    {
        ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], H, Sv, V);
        cached_for = CP.id;
        hex_edit = false;
    }

    static const int SWATCH_MAX = 8;
    static ImVec4 sw[SWATCH_MAX] = {
        ImVec4(0.451f, 0.306f, 0.765f, 1), ImVec4(0.929f, 0.259f, 0.259f, 1),
        ImVec4(0.961f, 0.620f, 0.161f, 1), ImVec4(0.290f, 0.816f, 0.471f, 1),
        ImVec4(0.290f, 0.596f, 1.000f, 1), ImVec4(0.949f, 0.949f, 0.976f, 1),
    };
    static int sw_count = 6;

    const float w      = 270.0f;
    const float pad    = 14.0f;
    const float head_h = 40.0f;
    const float sq_w   = w - pad * 2.0f;
    const float sq_h   = sq_w * 0.80f;
    const float hue_h  = 16.0f;
    const float row_h  = 34.0f;
    const float sw_h   = 30.0f;
    const float h = head_h + sq_h + 12.0f + hue_h + 14.0f + row_h + 14.0f + sw_h + pad;

    ImVec2 ds = Screen();
    if (!CP.placed)
    {
        CP.pos = ImVec2(ImClamp(CP.anchor_min.x - w * 0.5f + (CP.anchor_max.x - CP.anchor_min.x) * 0.5f,
                                12.0f, ds.x - w - 12.0f),
                        ImClamp(CP.anchor_max.y + 8.0f, 12.0f, ds.y - h - 12.0f));
        CP.placed = true;
    }
    CP.pos.x = ImClamp(CP.pos.x, 8.0f, ImMax(ds.x - w - 8.0f, 8.0f));
    CP.pos.y = ImClamp(CP.pos.y, 8.0f, ImMax(ds.y - h - 8.0f, 8.0f));

    ImVec2 min(CP.pos.x, CP.pos.y), max(min.x + w, min.y + h);
    PopupPanel(dl, min, max, 14.0f);

    ImU32 cur = ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], 1.0f));
    dl->AddCircleFilled(ImVec2(min.x + 24.0f, min.y + 20.0f), 8.0f, cur, 24);
    dl->AddCircle(ImVec2(min.x + 24.0f, min.y + 20.0f), 8.0f, IM_COL32(255, 255, 255, 60), 24);
    TextAt(dl, F_Med, ImVec2(min.x + 40.0f, min.y + 20.0f - Measure(F_Med, CP.label).y * 0.5f),
           C.text, CP.label);
    if (IconButton(pstra("##cp_close"), IC_CLOSE, ImVec2(max.x - 18.0f, min.y + 20.0f), 12.0f, C.text_dim, 12.0f))
    {
        CloseColorPicker();
        return;
    }

    ImVec2 smin(min.x + pad, min.y + head_h);
    ImVec2 smax(smin.x + sq_w, smin.y + sq_h);

    float hr, hg, hb;
    ImGui::ColorConvertHSVtoRGB(H, 1.0f, 1.0f, hr, hg, hb);
    ImU32 hue_col = ImGui::GetColorU32(ImVec4(hr, hg, hb, 1.0f));

    dl->AddRectFilledMultiColor(smin, smax, IM_COL32_WHITE, hue_col, hue_col, IM_COL32_WHITE);
    dl->AddRectFilledMultiColor(smin, smax, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
                                IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 255));
    RectStroke(dl, smin, smax, IM_COL32(0, 0, 0, 70), 8.0f);

    bool sv_hov = false;
    Hitbox(pstra("##cp_sv"), smin, smax, &sv_hov);
    ImGuiID sv_id = ImGui::GetCurrentWindow()->GetID(pstra("##cp_sv"));
    if (GImGui->ActiveId == sv_id && ImGui::IsMouseDown(0))
    {
        ImVec2 m = ImGui::GetIO().MousePos;
        Sv = ImClamp((m.x - smin.x) / sq_w, 0.0f, 1.0f);
        V  = 1.0f - ImClamp((m.y - smin.y) / sq_h, 0.0f, 1.0f);
    }

    ImVec2 cur_pos(smin.x + Sv * sq_w, smin.y + (1.0f - V) * sq_h);
    dl->AddCircle(cur_pos, 7.0f, IM_COL32(255, 255, 255, 255), 24, 2.2f);
    dl->AddCircle(cur_pos, 9.0f, IM_COL32(0, 0, 0, 90), 24, 1.0f);

    float hue_y = smax.y + 12.0f;
    ImVec2 dcp(min.x + pad + 8.0f, hue_y + hue_h * 0.5f);
    bool drop_hov = false;
    Hitbox(pstra("##cp_drop"), ImVec2(dcp.x - 11, dcp.y - 11), ImVec2(dcp.x + 11, dcp.y + 11), &drop_hov);
    DrawDropper(dl, dcp, 17.0f, drop_hov ? C.text : C.text_dim);

    ImVec2 hmin(min.x + pad + 26.0f, hue_y), hmax(max.x - pad, hue_y + hue_h);
    const float hround = hue_h * 0.5f;
    const int NH = 72;
    for (int i = 0; i < NH; i++)
    {
        float t0 = (float)i / NH;
        float x0 = ImLerp(hmin.x, hmax.x, t0);
        float x1 = ImLerp(hmin.x, hmax.x, (float)(i + 1) / NH) + (i < NH - 1 ? 0.6f : 0.0f);
        float rr, gg, bb;
        ImGui::ColorConvertHSVtoRGB(ImMin(t0, 0.9999f), 1.0f, 1.0f, rr, gg, bb);
        int flags = 0;
        if (i == 0)      flags |= ImDrawCornerFlags_Left;
        if (i == NH - 1) flags |= ImDrawCornerFlags_Right;
        RectFilled(dl, ImVec2(x0, hmin.y), ImVec2(x1, hmax.y),
                   ImGui::GetColorU32(ImVec4(rr, gg, bb, 1.0f)), flags ? hround : 0.0f, flags);
    }
    RectStroke(dl, hmin, hmax, IM_COL32(0, 0, 0, 70), hround);

    bool hue_hov = false;
    Hitbox(pstra("##cp_hue"), ImVec2(hmin.x, hmin.y - 3), ImVec2(hmax.x, hmax.y + 3), &hue_hov);
    ImGuiID hue_id = ImGui::GetCurrentWindow()->GetID(pstra("##cp_hue"));
    if (GImGui->ActiveId == hue_id && ImGui::IsMouseDown(0))
        H = ImClamp((ImGui::GetIO().MousePos.x - hmin.x) / (hmax.x - hmin.x), 0.0f, 0.9999f);

    float hx = ImLerp(hmin.x + 6.0f, hmax.x - 6.0f, H);
    ImVec2 kh_min(hx - 5.0f, hmin.y - 2.0f), kh_max(hx + 5.0f, hmax.y + 2.0f);
    RectFilled(dl, kh_min, kh_max, IM_COL32(255, 255, 255, 255), 5.0f);
    RectStroke(dl, kh_min, kh_max, IM_COL32(0, 0, 0, 60), 5.0f);

    ImGui::ColorConvertHSVtoRGB(H, Sv, V, col[0], col[1], col[2]);

    ImGuiIO& io = ImGui::GetIO();

    float rowy = hmax.y + 14.0f;
    float lw = sq_w * 0.55f;
    ImVec2 xmin(min.x + pad, rowy), xmax(xmin.x + lw, rowy + row_h);

    char hex[16];
    snprintf(hex, sizeof(hex), pstra("%02X%02X%02X"),
             (int)(col[0] * 255.0f + 0.5f), (int)(col[1] * 255.0f + 0.5f),
             (int)(col[2] * 255.0f + 0.5f));

    bool hex_hov = false;
    if (Hitbox(pstra("##cp_hex"), xmin, xmax, &hex_hov))
    {
        hex_edit = true;
        snprintf(hex_buf, sizeof(hex_buf), pstra("%s"), hex);
    }
    RectFilled(dl, xmin, xmax, C.pill, 9.0f);
    if (hex_edit) RectStroke(dl, xmin, xmax, Accent(0.7f), 9.0f);

    auto ApplyHex = [&](const char* s)
    {
        char digs[6]; int nd = 0;
        for (; *s && nd < 6; s++)
        {
            char c = *s;
            if (c >= '0' && c <= '9') digs[nd++] = c;
            else if (c >= 'a' && c <= 'f') digs[nd++] = c;
            else if (c >= 'A' && c <= 'F') digs[nd++] = c;
        }
        if (nd == 0) return;
        auto hv = [](char c)->int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return c - 'A' + 10;
        };
        int v[6]; for (int i = 0; i < 6; i++) v[i] = (i < nd) ? hv(digs[i]) : 0;
        col[0] = (v[0] * 16 + v[1]) / 255.0f;
        col[1] = (v[2] * 16 + v[3]) / 255.0f;
        col[2] = (v[4] * 16 + v[5]) / 255.0f;
        ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], H, Sv, V);
    };

    if (hex_edit)
    {
        for (int n = 0; n < io.InputQueueCharacters.Size; n++)
        {
            ImWchar c = io.InputQueueCharacters[n];
            bool okc = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
            int L = (int)strlen(hex_buf);
            if (okc && L < 6) { hex_buf[L] = (c >= 'a' && c <= 'f') ? (char)(c - 32) : (char)c; hex_buf[L + 1] = 0; }
        }
        if (ImGui::IsKeyPressed(0x08, true) && strlen(hex_buf) > 0) hex_buf[strlen(hex_buf) - 1] = 0;
        if (io.KeyCtrl && ImGui::IsKeyPressed('V', false))
        {
            const char* clip = ImGui::GetClipboardText();
            if (clip)
            {
                int nd = 0;
                for (const char* s = clip; *s && nd < 6; s++)
                {
                    char c = *s;
                    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) hex_buf[nd++] = c;
                    else if (c >= 'a' && c <= 'f') hex_buf[nd++] = (char)(c - 32);
                }
                hex_buf[nd] = 0;
            }
        }
        ApplyHex(hex_buf);
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false)) hex_edit = false;
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape), false)) hex_edit = false;
        if (ImGui::IsMouseClicked(0) && !hex_hov) hex_edit = false;
    }

    char hexlbl[16]; snprintf(hexlbl, sizeof(hexlbl), pstra("#%s"), hex_edit ? hex_buf : hex);
    ImVec2 hls = Measure(F_Med, hexlbl);
    TextAt(dl, F_Med, ImVec2(xmin.x + 14.0f, (xmin.y + xmax.y) * 0.5f - hls.y * 0.5f), C.text, hexlbl);
    if (hex_edit && fmodf((float)ImGui::GetTime(), 1.0f) < 0.5f)
    {
        float cxx = xmin.x + 14.0f + hls.x + 1.0f;
        dl->AddLine(ImVec2(cxx, (xmin.y + xmax.y) * 0.5f - 7), ImVec2(cxx, (xmin.y + xmax.y) * 0.5f + 7), C.text, 1.0f);
    }

    ImVec2 omin(xmax.x + 8.0f, rowy), omax(max.x - pad, rowy + row_h);
    RectFilled(dl, omin, omax, C.pill, 9.0f);
    bool op_hov = false;
    Hitbox(pstra("##cp_op"), omin, omax, &op_hov);
    ImGuiID op_id = ImGui::GetCurrentWindow()->GetID(pstra("##cp_op"));
    if (GImGui->ActiveId == op_id && ImGui::IsMouseDown(0))
    {
        col[3] = ImClamp(col[3] + io.MouseDelta.x * 0.006f, 0.0f, 1.0f);
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (op_hov && io.MouseWheel != 0.0f)
        col[3] = ImClamp(col[3] + io.MouseWheel * 0.05f, 0.0f, 1.0f);
    char pc[8]; snprintf(pc, sizeof(pc), pstra("%d%%"), (int)(col[3] * 100.0f + 0.5f));
    TextAt(dl, F_Med, ImVec2(omin.x + 12.0f, (omin.y + omax.y) * 0.5f - Measure(F_Med, pc).y * 0.5f),
           C.text, pc);
    DrawIcon(dl, IC_SUN, ImVec2(omax.x - 16.0f, (omin.y + omax.y) * 0.5f), 15.0f, C.text_dim, 1.4f);

    float swy = omax.y + 14.0f;
    float d = sw_h;
    float step = (sq_w - d) / (float)ImMax(sw_count, 1);
    for (int i = 0; i < sw_count; i++)
    {
        ImVec2 c(min.x + pad + d * 0.5f + step * i, swy + d * 0.5f);
        ImU32 sc = ImGui::GetColorU32(sw[i]);
        char sid[24]; snprintf(sid, sizeof(sid), pstra("##cp_sw%d"), i);
        bool shov = false;
        bool sclick = Hitbox(sid, ImVec2(c.x - d * 0.5f, c.y - d * 0.5f), ImVec2(c.x + d * 0.5f, c.y + d * 0.5f), &shov);
        if (shov && ImGui::IsMouseDoubleClicked(0))
        {
            for (int j = i; j < sw_count - 1; j++) sw[j] = sw[j + 1];
            sw_count--; i--; continue;
        }
        if (sclick)
        {
            col[0] = sw[i].x; col[1] = sw[i].y; col[2] = sw[i].z;
            ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], H, Sv, V);
            hex_edit = false;
        }
        dl->AddCircleFilled(c, shov ? d * 0.5f : d * 0.5f - 1.5f, sc, 28);
        dl->AddCircle(c, shov ? d * 0.5f : d * 0.5f - 1.5f, IM_COL32(255, 255, 255, 40), 28, 1.0f);
    }

    {
        ImVec2 c(min.x + pad + d * 0.5f + step * sw_count, swy + d * 0.5f);
        bool ahov = false;
        if (Hitbox(pstra("##cp_add"), ImVec2(c.x - d * 0.5f, c.y - d * 0.5f), ImVec2(c.x + d * 0.5f, c.y + d * 0.5f), &ahov))
        {
            int top = ImMin(sw_count, SWATCH_MAX - 1);
            for (int j = top; j > 0; j--) sw[j] = sw[j - 1];
            sw[0] = ImVec4(col[0], col[1], col[2], 1.0f);
            sw_count = ImMin(sw_count + 1, SWATCH_MAX);
        }
        dl->AddCircle(c, d * 0.5f - 1.5f, ahov ? C.text : C.text_dim, 28, 1.4f);
        DrawIcon(dl, IC_PLUS, c, 13.0f, ahov ? C.text : C.text_dim, 1.6f);
    }

    ImVec2 hdr_min(min.x, min.y), hdr_max(max.x, min.y + head_h);
    bool hdr_hov = false;
    Hitbox(pstra("##cp_drag"), hdr_min, hdr_max, &hdr_hov);
    ImGuiID drag_id = ImGui::GetCurrentWindow()->GetID(pstra("##cp_drag"));
    if (GImGui->ActiveId == drag_id && ImGui::IsMouseDown(0))
    {
        CP.pos.x += ImGui::GetIO().MouseDelta.x;
        CP.pos.y += ImGui::GetIO().MouseDelta.y;
        CP.pos.x = ImClamp(CP.pos.x, -w + 60.0f, ds.x - 60.0f);
        CP.pos.y = ImClamp(CP.pos.y, 0.0f, ds.y - 40.0f);
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    *out_min = min; *out_max = max;
}

static int         CaptureVK();
static std::string HKName(int vk);

static void PopupProfile(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    Settings& settings = Settings::Get();
    Settings::sInterface& ui = settings.Interface;
    const ProtectedStringList language_options{ pstra("English"), pstra("Portugues"), pstra("Russkiy"), pstra("Espanol") };

    struct Entry { Icon ic; ProtectedText label; };
    const Entry entries[] = {
        { IC_HOTKEY, pstra("Installed hotkeys") },
        { IC_LANG,   pstra("Menu language")     },
        { IC_DPI,    pstra("DPI Menu")          },
        { IC_STYLE,  pstra("Styles")            },
    };
    const int n = IM_ARRAYSIZE(entries);

    const float w = 300.0f;
    const float head_h = 96.0f;
    const float row_h = 44.0f;
    const float h = head_h + n * row_h + 12.0f;

    ImVec2 ds = Screen();
    float x = ImClamp(P.amax.x - w, 12.0f, ds.x - w - 12.0f);
    float y = ImClamp(P.amax.y + 8.0f, 12.0f, ds.y - h - 12.0f);
    y -= (1.0f - P.anim) * 6.0f;

    ImVec2 min(x, y), max(x + w, y + h);
    PopupPanel(dl, min, max, 16.0f);

    ImVec2 av(min.x + 52.0f, min.y + 40.0f);
    DrawAvatar(dl, av, 26.0f);

    TextAt(dl, F_Head, ImVec2(min.x + 88.0f, min.y + 26.0f), C.text, pstra("Project Oficial"));
    TextAt(dl, F_Med, ImVec2(min.x + 88.0f, min.y + 48.0f), C.text_mute, pstra("Till:"));
    TextAt(dl, F_Med, ImVec2(min.x + 88.0f + Measure(F_Med, pstra("Till: ")).x, min.y + 48.0f),
           C.text_dim, pstra("15 Jan 2026"));

    dl->AddLine(ImVec2(min.x + 18.0f, min.y + head_h - 12.0f),
                ImVec2(max.x - 18.0f, min.y + head_h - 12.0f), C.separator, 1.0f);

    for (int i = 0; i < n; i++)
    {
        ImVec2 rmin(min.x + 10.0f, min.y + head_h + i * row_h);
        ImVec2 rmax(max.x - 10.0f, rmin.y + row_h - 2.0f);
        float cy = (rmin.y + rmax.y) * 0.5f;

        if (i == 3)
        {
            ImVec2 sw(rmax.x - 20.0f, cy);
            if (Hitbox(pstra("##pf_swatch"), ImVec2(sw.x - 12, sw.y - 12), ImVec2(sw.x + 12, sw.y + 12)))
                OpenColorPicker(ImHashStr(pstra("blade_accent")),
                                ImVec2(sw.x - 12, sw.y - 12), ImVec2(sw.x + 12, sw.y + 12),
                                ColorPtr(ui.AccentColor), pstra("Accent (color)"));
        }

        char id[40];
        snprintf(id, sizeof(id), pstra("##pf_row%d"), i);
        bool hov = false;
        bool clicked = Hitbox(id, rmin, rmax, &hov);

        bool expanded = (P.sub == i);
        if (expanded)  RectFilled(dl, rmin, rmax, IM_COL32(255, 255, 255, 14), 9.0f);
        else if (hov)  RectFilled(dl, rmin, rmax, IM_COL32(255, 255, 255, 8), 9.0f);

        DrawIcon(dl, entries[i].ic, ImVec2(rmin.x + 20.0f, cy), 17.0f,
                 expanded ? Accent(1.0f) : C.text_dim, 1.5f);

        ImVec2 ts = Measure(F_Label, entries[i].label);
        TextAt(dl, F_Label, ImVec2(rmin.x + 40.0f, cy - ts.y * 0.5f), C.text, entries[i].label);

        if (i == 3)
        {

            DrawIcon(dl, IC_SUNDIM, ImVec2(rmax.x - 46.0f, cy), 15.0f, C.text_dim, 1.3f);

            ImVec2 sw(rmax.x - 20.0f, cy);
            dl->AddCircleFilled(sw, 9.0f, Accent(1.0f), 24);
            dl->AddCircle(sw, 9.0f, IM_COL32(255, 255, 255, 50), 24);
            if (ImGui::IsMouseHoveringRect(ImVec2(sw.x - 12, sw.y - 12), ImVec2(sw.x + 12, sw.y + 12)))
                dl->AddCircle(sw, 12.0f, IM_COL32(255, 255, 255, 60), 24);
        }
        else
        {
            DrawIcon(dl, expanded ? IC_CHEVRON_L : IC_CHEVRON_R,
                     ImVec2(rmax.x - 18.0f, cy), 14.0f, C.text_dim, 1.5f);
        }

        if (clicked)
        {
            if (i == 3)
            {

                OpenPopup(PK_SETTINGS, ImHashStr(pstra("blade_settings")), rmin, rmax);
                return;
            }
            P.sub = expanded ? -1 : i;
        }

        if (expanded)
        {
            if (i == 0)
            {
                struct HK { ProtectedText a; int* vk; int slot; };
                HK hk[] = {
                    { pstra("Open menu"), &settings.ShowMenuKey, 0 },
                    { pstra("Panic"),     &ui.PanicKey, 1 },
                };
                const int hn = IM_ARRAYSIZE(hk);
                const float rowh = 38.0f;
                float sw2 = 236.0f, sh = 12.0f + hn * rowh;
                ImVec2 pmin(min.x - sw2 - 10.0f, rmin.y - 6.0f);
                ImVec2 pmax(pmin.x + sw2, pmin.y + sh);
                PopupPanel(dl, pmin, pmax, 12.0f);
                for (int k = 0; k < hn; k++)
                {
                    ImVec2 rlo(pmin.x + 8.0f, pmin.y + 6.0f + k * rowh);
                    ImVec2 rhi(pmax.x - 8.0f, rlo.y + rowh - 4.0f);
                    float  mid = (rlo.y + rhi.y) * 0.5f;
                    bool   cap = (S.hk_capturing == hk[k].slot);

                    char id[24]; snprintf(id, sizeof(id), pstra("##pf_hk%d"), k);
                    bool rhov = false;
                    bool rclick = Hitbox(id, rlo, rhi, &rhov);
                    if (cap)       RectFilled(dl, rlo, rhi, Fade(Accent(1.0f), 0.14f), 9.0f);
                    else if (rhov) RectFilled(dl, rlo, rhi, IM_COL32(255, 255, 255, 10), 9.0f);
                    if (rclick) S.hk_capturing = cap ? -1 : hk[k].slot;

                    TextAt(dl, F_Body, ImVec2(rlo.x + 10.0f, mid - Measure(F_Body, hk[k].a).y * 0.5f),
                           C.text, hk[k].a);

                    if (cap)
                    {
                        float pulse = 0.55f + 0.45f * sinf((float)ImGui::GetTime() * 6.0f);
                        TextRight(dl, F_Body, ImVec2(rhi.x - 10.0f, mid - Measure(F_Body, pstra("Press...")).y * 0.5f),
                                  Fade(Accent(1.0f), pulse), pstra("Press..."));
                        int nv = CaptureVK();
                        if (nv > 0) { *hk[k].vk = nv; S.hk_capturing = -1; }
                    }
                    else
                    {
                        const std::string kn = HKName(*hk[k].vk);
                        TextRight(dl, F_Body, ImVec2(rhi.x - 10.0f, mid - Measure(F_Body, kn.c_str()).y * 0.5f),
                                  Accent(1.0f), kn.c_str());
                    }
                }
                Hitbox(pstra("##pf_sub0"), pmin, pmax);
            }
            else if (i == 1)
            {
                const int ln = language_options.Count();
                float sw2 = 200.0f, sh = 14.0f + ln * 34.0f;
                ImVec2 pmin(min.x - sw2 - 10.0f, rmin.y - 6.0f);
                ImVec2 pmax(pmin.x + sw2, pmin.y + sh);
                PopupPanel(dl, pmin, pmax, 12.0f);
                for (int k = 0; k < ln; k++)
                {
                    ImVec2 lmin(pmin.x + 12.0f, pmin.y + 7.0f + k * 34.0f);
                    ImVec2 lmax(pmax.x - 12.0f, lmin.y + 32.0f);
                    char lid[40]; snprintf(lid, sizeof(lid), pstra("##pf_lang%d"), k);
                    bool lhov = false;
                    if (Hitbox(lid, lmin, lmax, &lhov)) ui.MenuLang = k;
                    if (ui.MenuLang == k) RectFilled(dl, lmin, lmax, IM_COL32(255, 255, 255, 14), 8.0f);
                    else if (lhov)        RectFilled(dl, lmin, lmax, IM_COL32(255, 255, 255, 8), 8.0f);
                    TextAt(dl, F_Body, ImVec2(lmin.x + 12.0f, (lmin.y + lmax.y) * 0.5f - 6.0f),
                           ui.MenuLang == k ? C.text : C.text_dim, language_options[k]);
                    if (ui.MenuLang == k)
                        DrawIcon(dl, IC_CHECK, ImVec2(lmax.x - 16.0f, (lmin.y + lmax.y) * 0.5f),
                                 13.0f, Accent(1.0f), 1.4f);
                }
                Hitbox(pstra("##pf_sub1"), pmin, pmax);
            }
            else if (i == 2)
            {
                float sw2 = 236.0f, sh = 92.0f;
                ImVec2 pmin(min.x - sw2 - 10.0f, rmin.y - 6.0f);
                ImVec2 pmax(pmin.x + sw2, pmin.y + sh);
                PopupPanel(dl, pmin, pmax, 12.0f);

                TextAt(dl, F_Label, ImVec2(pmin.x + 18.0f, pmin.y + 20.0f),
                       ui.AutoScale ? C.text_dim : C.text, pstra("DPI"));
                {
                    float before = ui.UiScale;
                    if (ui.AutoScale) PushAlpha(0.5f);
                    SliderF(pstra("##pf_dpi"), ImVec2(pmin.x + 74.0f, pmin.y + 26.0f),
                            ImVec2(pmax.x - 18.0f, pmin.y + 26.0f), &ui.UiScale, 0.75f, 2.0f);
                    if (ui.AutoScale) { ui.UiScale = before; PopAlpha(); }
                }

                TextAt(dl, F_Label, ImVec2(pmin.x + 18.0f, pmin.y + 58.0f), C.text, pstra("Auto scale"));
                Checkbox(pstra("##pf_autoscale"), ImVec2(pmax.x - 28.0f, pmin.y + 64.0f), &ui.AutoScale, 22.0f);

                Hitbox(pstra("##pf_sub2"), pmin, pmax);
            }
        }
    }

    *out_min = min; *out_max = max;
}

static void PopupModules(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    int n = 0;
    const Module* mods = GetModules(S.menu_tab, &n);

    const float item_h = 34.0f;
    const float sub_h = 30.0f;
    const float w = 220.0f;

    int extra = 0;
    if (P.sub >= 0 && P.sub < n) extra = mods[P.sub].sub_count;
    float h = 10.0f + n * item_h + extra * sub_h;

    ImVec2 ds = Screen();
    float x = ImClamp(P.amin.x - 8.0f, 12.0f, ds.x - w - 12.0f);
    float y = ImClamp(P.amax.y + 6.0f, 12.0f, ds.y - h - 12.0f);
    y -= (1.0f - P.anim) * 6.0f;

    ImVec2 min(x, y), max(x + w, y + h);
    PopupPanel(dl, min, max, 12.0f);

    float cy = min.y + 5.0f;
    for (int i = 0; i < n; i++)
    {
        ImVec2 rmin(min.x + 10.0f, cy);
        ImVec2 rmax(max.x - 8.0f, cy + item_h - 2.0f);

        bool sel = (S.module_sel[S.menu_tab] == i);
        bool expandable = (mods[i].sub_count > 0);
        bool expanded = (P.sub == i);

        char id[48];
        snprintf(id, sizeof(id), pstra("##mod_%d"), i);
        bool hov = false;
        bool clicked = Hitbox(id, rmin, rmax, &hov);

        if (sel)      RectFilled(dl, rmin, rmax, IM_COL32(255, 255, 255, 16), 8.0f);
        else if (hov) RectFilled(dl, rmin, rmax, IM_COL32(255, 255, 255, 8), 8.0f);
        if (sel)
            RectFilled(dl, ImVec2(min.x + 2.0f, rmin.y + 4.0f), ImVec2(min.x + 5.5f, rmax.y - 4.0f),
                              Accent(1.0f), 2.0f);

        ImVec2 ts = Measure(F_Label, mods[i].name);
        TextAt(dl, F_Label, ImVec2(rmin.x + 12.0f, (rmin.y + rmax.y) * 0.5f - ts.y * 0.5f),
               sel ? C.text : C.text_dim, mods[i].name);

        if (expandable)
            DrawIcon(dl, expanded ? IC_CHEVRON : IC_CHEVRON_R,
                     ImVec2(rmax.x - 16.0f, (rmin.y + rmax.y) * 0.5f), 13.0f, C.text_mute, 1.4f);
        else if (sel)
            DrawIcon(dl, IC_CHEVRON_R, ImVec2(rmax.x - 16.0f, (rmin.y + rmax.y) * 0.5f),
                     13.0f, Accent(1.0f), 1.4f);

        if (clicked)
        {
            S.module_sel[S.menu_tab] = i;
            if (expandable) P.sub = expanded ? -1 : i;
            else            ClosePopup();
        }

        cy += item_h;

        if (expanded)
        {
            for (int k = 0; k < mods[i].sub_count; k++)
            {
                ImVec2 smin(min.x + 24.0f, cy);
                ImVec2 smax(max.x - 8.0f, cy + sub_h - 2.0f);

                char sid[48];
                snprintf(sid, sizeof(sid), pstra("##modsub_%d_%d"), i, k);
                bool shov = false;
                if (Hitbox(sid, smin, smax, &shov)) ClosePopup();
                if (shov) RectFilled(dl, smin, smax, IM_COL32(255, 255, 255, 8), 7.0f);

                ImVec2 sts = Measure(F_Body, mods[i].subs[k]);
                TextAt(dl, F_Body, ImVec2(smin.x + 12.0f, (smin.y + smax.y) * 0.5f - sts.y * 0.5f),
                       shov ? C.text : C.text_mute, mods[i].subs[k]);
                cy += sub_h;
            }
        }
    }

    *out_min = min; *out_max = max;
}

static void PopupItemPick(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    const float w = 300.0f;
    const float head_h = 52.0f;
    const float search_h = 34.0f;
    const float cell = 44.0f, cgap = 8.0f;
    const int   cols = 5;
    const float grid_h = 200.0f;
    const float btn_h = 34.0f;
    const float h = head_h + search_h + 12.0f + grid_h + 14.0f + btn_h + 14.0f;

    ImVec2 ds = Screen();
    ImVec2 min(floorf(ds.x * 0.5f - w * 0.5f + 84.0f), floorf(ds.y * 0.5f - h * 0.5f - 10.0f));
    ImVec2 max(min.x + w, min.y + h);

    float a = P.anim;
    min.y -= (1.0f - a) * 10.0f;
    max.y -= (1.0f - a) * 10.0f;

    PopupPanel(dl, min, max, 14.0f);

    TextAt(dl, F_Title, ImVec2(min.x + 16.0f, min.y + 14.0f), C.text, pstra("Choosing a subject"));
    TextAt(dl, F_Small, ImVec2(min.x + 16.0f, min.y + 31.0f), C.text_mute,
           pstra("Choosing the item you want to buy automatically"));
    if (IconButton(pstra("##ip_close"), IC_CLOSE, ImVec2(max.x - 20.0f, min.y + 20.0f), 12.0f, C.text_dim, 12.0f))
    {
        ClosePopup();
        return;
    }

    ImVec2 smin(min.x + 14.0f, min.y + head_h);
    ImVec2 smax(max.x - 14.0f, smin.y + search_h);
    bool s_hov = false;
    Hitbox(pstra("##ip_search"), smin, smax, &s_hov);
    RectFilled(dl, smin, smax, s_hov ? C.pill_hover : C.pill, 9.0f);
    RectStroke(dl, smin, smax, C.border, 9.0f);
    DrawIcon(dl, IC_SEARCH, ImVec2(smin.x + 17.0f, (smin.y + smax.y) * 0.5f), 14.0f, C.text_mute, 1.4f);
    TextAt(dl, F_Body, ImVec2(smin.x + 33.0f, (smin.y + smax.y) * 0.5f - 6.0f),
           C.text_mute, pstra("Search for an item"));

    ImVec2 gmin(min.x + 14.0f, smax.y + 12.0f);
    ImVec2 gmax(max.x - 14.0f, gmin.y + grid_h);

    int rows = (kItemCount + cols - 1) / cols;
    float content_h = rows * cell + (rows - 1) * cgap;

    if (ImGui::IsMouseHoveringRect(gmin, gmax) && ImGui::GetIO().MouseWheel != 0.0f)
        S.ab_scroll -= ImGui::GetIO().MouseWheel * 40.0f;
    S.ab_scroll = ImClamp(S.ab_scroll, 0.0f, ImMax(content_h - grid_h, 0.0f));

    dl->PushClipRect(gmin, gmax, true);
    float grid_w = (gmax.x - gmin.x) - 10.0f;
    float step = (grid_w - (cols - 1) * cgap) / cols;

    for (int i = 0; i < kItemCount; i++)
    {
        int r = i / cols, c = i % cols;
        ImVec2 cmin(gmin.x + c * (step + cgap), gmin.y + r * (cell + cgap) - S.ab_scroll);
        ImVec2 cmax(cmin.x + step, cmin.y + cell);
        if (cmax.y < gmin.y - 4.0f || cmin.y > gmax.y + 4.0f) continue;

        char id[40];
        snprintf(id, sizeof(id), pstra("##ip_item%d"), i);
        bool hov = false;
        if (Hitbox(id, cmin, cmax, &hov)) S.ab_pick = i;

        bool sel = (S.ab_pick == i);
        RectFilled(dl, cmin, cmax, sel ? IM_COL32(48, 44, 66, 255)
                                          : (hov ? C.pill_hover : C.pill), 9.0f);
        if (sel) RectStroke(dl, cmin, cmax, Accent(0.9f), 9.0f);

        DrawBlock(dl, ImVec2((cmin.x + cmax.x) * 0.5f, (cmin.y + cmax.y) * 0.5f + 1.0f),
                  cell * 0.56f, kItems[i].color);
    }
    dl->PopClipRect();

    if (content_h > grid_h)
    {
        float tx = gmax.x - 3.0f;
        float ratio = grid_h / content_h;
        float bh = ImMax(grid_h * ratio, 24.0f);
        float by = gmin.y + (grid_h - bh) * (S.ab_scroll / ImMax(content_h - grid_h, 1.0f));
        RectFilled(dl, ImVec2(tx - 1.5f, gmin.y), ImVec2(tx + 1.5f, gmax.y), C.separator, 1.5f);
        RectFilled(dl, ImVec2(tx - 1.5f, by), ImVec2(tx + 1.5f, by + bh), C.scroll, 1.5f);
    }

    ImVec2 bmin(min.x + 14.0f, gmax.y + 14.0f);
    ImVec2 bmax(max.x - 14.0f, bmin.y + btn_h);
    if (Button(pstra("##ip_choose"), bmin, bmax, pstra("Choose")))
    {
        if (S.ab_slot >= 0 && S.ab_slot < IM_ARRAYSIZE(S.ab_items))
            S.ab_items[S.ab_slot] = S.ab_pick;
        ClosePopup();
        return;
    }

    *out_min = min; *out_max = max;
}

static int g_set_rows = 0, g_set_i = 0;

static void SettingsSection(ImDrawList* dl, float x, float* y, const char* title, int rows)
{
    TextAt(dl, F_Small, ImVec2(x + 2.0f, *y + 8.0f), C.text_mute, title);
    *y += 27.0f;
    g_set_rows = rows;
    g_set_i = 0;
}

static float SettingsRow(ImDrawList* dl, float x, float* y, float w, const char* label)
{
    const float rh = 34.0f;
    const bool first = (g_set_i == 0);
    const bool last  = (g_set_i == g_set_rows - 1);

    int corners = 0;
    if (first) corners |= ImDrawCornerFlags_Top;
    if (last)  corners |= ImDrawCornerFlags_Bot;

    ImVec2 rmin(x, *y), rmax(x + w, *y + rh);
    RectFilled(dl, rmin, rmax, C.row, 10.0f, corners);
    if (!first)
        dl->AddLine(ImVec2(x + 14.0f, rmin.y), ImVec2(x + w - 14.0f, rmin.y), C.divider, 1.0f);

    ImVec2 ts = Measure(F_Body, label);
    TextAt(dl, F_Body, ImVec2(x + 14.0f, *y + (rh - ts.y) * 0.5f), C.text, label);

    float cy = *y + rh * 0.5f;
    *y += rh;
    g_set_i++;
    return cy;
}

static void SettingsColorRow(ImDrawList* dl, float x, float* y, float w,
                             const char* label, const char* id, float* col)
{
    float cy = SettingsRow(dl, x, y, w, label);

    char hex[16];
    snprintf(hex, sizeof(hex), pstra("%02X%02X%02X"),
             (int)(col[0] * 255.0f + 0.5f), (int)(col[1] * 255.0f + 0.5f), (int)(col[2] * 255.0f + 0.5f));
    TextRight(dl, F_Body, ImVec2(x + w - 38.0f, cy - Measure(F_Body, hex).y * 0.5f), C.text_mute, hex);

    ImVec2 sw(x + w - 22.0f, cy);
    bool hov = false;
    if (Hitbox(id, ImVec2(sw.x - 13, sw.y - 13), ImVec2(sw.x + 13, sw.y + 13), &hov))
    {
        char cl[40]; snprintf(cl, sizeof(cl), pstra("%s (color)"), label);
        OpenColorPicker(ImHashStr(id), ImVec2(sw.x - 13, sw.y - 13), ImVec2(sw.x + 13, sw.y + 13), col, cl);
    }

    dl->AddCircleFilled(sw, 10.0f, ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], 1.0f)), 24);
    dl->AddCircle(sw, 10.0f, IM_COL32(255, 255, 255, hov ? 110 : 50), 24, 1.4f);
}

static void SettingsSlider(ImDrawList* dl, float x, float* y, float w, const char* label,
                           const char* id, float* v, float lo, float hi, const char* fmt)
{
    float cy = SettingsRow(dl, x, y, w, label);
    char buf[16]; snprintf(buf, sizeof(buf), fmt, *v);
    TextAt(dl, F_Body, ImVec2(x + w * 0.50f - Measure(F_Body, buf).x, cy - 6.0f), C.text_dim, buf);
    SliderF(id, ImVec2(x + w * 0.55f, cy), ImVec2(x + w - 12.0f, cy), v, lo, hi);
}

static void SettingsToggleRow(ImDrawList* dl, float x, float* y, float w, const char* label,
                              const char* id, bool* v)
{
    float cy = SettingsRow(dl, x, y, w, label);
    Toggle(id, ImVec2(x + w - 12.0f, cy), v, 32.0f, 17.0f);
}

static void SettingsGradientRow(ImDrawList* dl, float x, float* y, float w, const char* label,
                                const char* idA, float* colA, const char* idB, float* colB)
{
    float cy = SettingsRow(dl, x, y, w, label);

    ImU32 ca = ImGui::GetColorU32(ImVec4(colA[0], colA[1], colA[2], 1.0f));
    ImU32 cb = ImGui::GetColorU32(ImVec4(colB[0], colB[1], colB[2], 1.0f));

    ImVec2 gmin(x + w - 100.0f, cy - 8.0f), gmax(x + w - 48.0f, cy + 8.0f);
    RectFilled(dl, gmin, gmax, IM_COL32(0, 0, 0, 60), 6.0f);
    GradientBarH(dl, gmin, gmax, 6.0f, ca, cb);
    RectStroke(dl, gmin, gmax, IM_COL32(255, 255, 255, 28), 6.0f);

    struct { ProtectedText id; float* col; ImU32 cc; float dx; } sw[2] = {
        { idA, colA, ca, 32.0f }, { idB, colB, cb, 10.0f }
    };
    for (int i = 0; i < 2; i++)
    {
        ImVec2 c(x + w - sw[i].dx, cy);
        bool hov = false;
        if (Hitbox(sw[i].id, ImVec2(c.x - 10, c.y - 10), ImVec2(c.x + 10, c.y + 10), &hov))
        {
            char cl[48]; snprintf(cl, sizeof(cl), pstra("%s %s"), label, i == 0 ? pstra("A") : pstra("B"));
            OpenColorPicker(ImHashStr(sw[i].id), ImVec2(c.x - 10, c.y - 10), ImVec2(c.x + 10, c.y + 10),
                            sw[i].col, cl);
        }
        dl->AddCircleFilled(c, 8.5f, sw[i].cc, 22);
        dl->AddCircle(c, 8.5f, IM_COL32(255, 255, 255, hov ? 120 : 55), 22, 1.3f);
    }
}

static void PopupSettings(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    Settings::sInterface& ui = Ui();
    const float w = 330.0f;
    const float head_h = 52.0f;
    const float pad = 14.0f;

    ImVec2 ds = Screen();
    const float h = ImMin(560.0f, ImMax(ds.y - 24.0f, 260.0f));

    if (!P.placed)
    {
        P.pos = ImVec2(ImClamp(P.amin.x - w - 12.0f, 12.0f, ds.x - w - 12.0f),
                       ImClamp(P.amin.y - 40.0f, 12.0f, ds.y - h - 12.0f));
        P.placed = true;
    }
    P.pos.x = ImClamp(P.pos.x, 8.0f, ImMax(ds.x - w - 8.0f, 8.0f));
    P.pos.y = ImClamp(P.pos.y, 8.0f, ImMax(ds.y - h - 8.0f, 8.0f));

    ImVec2 min(P.pos.x, P.pos.y), max(min.x + w, min.y + h);
    PopupPanel(dl, min, max, 16.0f);

    TextAt(dl, F_Title, ImVec2(min.x + 16.0f, min.y + 14.0f), C.text, pstra("Menu settings"));
    TextAt(dl, F_Small, ImVec2(min.x + 16.0f, min.y + 31.0f), C.text_mute,
           pstra("Cores, timers e comportamento - tudo editavel."));
    if (IconButton(pstra("##st_close"), IC_CLOSE, ImVec2(max.x - 20.0f, min.y + 20.0f), 12.0f, C.text_dim, 12.0f))
    {
        ClosePopup();
        return;
    }

    const float cx = min.x + pad;
    const float cw = w - pad * 2.0f - 4.0f;
    const float view_y0 = min.y + head_h;
    const float view_y1 = max.y - 8.0f;
    const float view_h  = view_y1 - view_y0;

    static float scroll = 0.0f, content_h = 0.0f;
    ImVec2 vmin(min.x + 2.0f, view_y0), vmax(max.x - 2.0f, view_y1);
    if (ImGui::IsMouseHoveringRect(vmin, vmax) && ImGui::GetIO().MouseWheel != 0.0f)
        scroll -= ImGui::GetIO().MouseWheel * 46.0f;
    scroll = ImClamp(scroll, 0.0f, ImMax(content_h - view_h, 0.0f));

    dl->PushClipRect(vmin, vmax, true);
    float y = view_y0 - scroll;

    SettingsSection(dl, cx, &y, pstra("Colors"), 4);
    SettingsColorRow(dl, cx, &y, cw, pstra("Accent color"), pstra("##st_accent"), ColorPtr(ui.AccentColor));
    SettingsColorRow(dl, cx, &y, cw, pstra("Text color"),   pstra("##st_text"),   ColorPtr(ui.TextColor));
    SettingsColorRow(dl, cx, &y, cw, pstra("Panel color"),  pstra("##st_panel"),  ColorPtr(ui.PanelColor));
    SettingsGradientRow(dl, cx, &y, cw, pstra("Text gradient"), pstra("##st_grada"), ColorPtr(ui.GradientStart), pstra("##st_gradb"), ColorPtr(ui.GradientEnd));

    y += 10.0f;
    SettingsSection(dl, cx, &y, pstra("Button colors"), 4);
    SettingsColorRow(dl, cx, &y, cw, pstra("Primary"), pstra("##st_btn"), ColorPtr(ui.PrimaryColor));
    SettingsColorRow(dl, cx, &y, cw, pstra("Success"), pstra("##st_suc"), ColorPtr(ui.SuccessColor));
    SettingsColorRow(dl, cx, &y, cw, pstra("Warning"), pstra("##st_wrn"), ColorPtr(ui.WarningColor));
    SettingsColorRow(dl, cx, &y, cw, pstra("Error"),   pstra("##st_err"), ColorPtr(ui.ErrorColor));

    y += 10.0f;
    SettingsSection(dl, cx, &y, pstra("Component gradients"), 3);
    SettingsGradientRow(dl, cx, &y, cw, pstra("Slider"),   pstra("##st_sla"), ColorPtr(ui.SliderStart), pstra("##st_slb"), ColorPtr(ui.SliderEnd));
    SettingsGradientRow(dl, cx, &y, cw, pstra("Toggle"),   pstra("##st_tga"), ColorPtr(ui.ToggleStart), pstra("##st_tgb"), ColorPtr(ui.ToggleEnd));
    SettingsGradientRow(dl, cx, &y, cw, pstra("Checkbox"), pstra("##st_cka"), ColorPtr(ui.CheckboxStart),  pstra("##st_ckb"), ColorPtr(ui.CheckboxEnd));

    y += 10.0f;
    SettingsSection(dl, cx, &y, pstra("Timing"), 5);
    SettingsSlider(dl, cx, &y, cw, pstra("Animation speed"), pstra("##st_anim"), &ui.AnimationSpeed, 0.1f, 3.0f, pstra("%.2fx"));
    SettingsSlider(dl, cx, &y, cw, pstra("Slider load"),     pstra("##st_tsl"),  &ui.SliderLoad, 0.2f, 3.0f, pstra("%.2fx"));
    SettingsSlider(dl, cx, &y, cw, pstra("Toggle / check"),  pstra("##st_ttg"),  &ui.ToggleLoad, 0.2f, 3.0f, pstra("%.2fx"));
    SettingsSlider(dl, cx, &y, cw, pstra("Intro cascade"),   pstra("##st_tin"),  &ui.IntroCascade,  0.2f, 3.0f, pstra("%.2fx"));
    SettingsSlider(dl, cx, &y, cw, pstra("ESP build"),       pstra("##st_tesp"), &ui.EspBuild,    0.2f, 3.0f, pstra("%.2fx"));

    y += 10.0f;
    SettingsSection(dl, cx, &y, pstra("Menu"), 5);
    {

        float before = ui.UiScale;
        if (ui.AutoScale) PushAlpha(0.5f);
        SettingsSlider(dl, cx, &y, cw, pstra("UI scale"),    pstra("##st_scale"), &ui.UiScale, 0.75f, 2.0f, pstra("%.2fx"));
        if (ui.AutoScale) { ui.UiScale = before; PopAlpha(); }
    }
    SettingsToggleRow(dl, cx, &y, cw, pstra("Auto scale"),   pstra("##st_autoscale"), &ui.AutoScale);
    SettingsSlider(dl, cx, &y, cw, pstra("Corner rounding"), pstra("##st_round"), &ui.CornerRounding, 0.0f, 1.8f, pstra("%.2fx"));
    SettingsToggleRow(dl, cx, &y, cw, pstra("Glass highlight"), pstra("##st_glass"),  &ui.Glass);
    SettingsToggleRow(dl, cx, &y, cw, pstra("Drop shadows"),    pstra("##st_shadow"), &ui.Shadows);

    y += 10.0f;
    SettingsSection(dl, cx, &y, pstra("Effects"), 7);
    SettingsToggleRow(dl, cx, &y, cw, pstra("Gradient text"), pstra("##st_gradtext"), &ui.GradientText);
    {
        float cy = SettingsRow(dl, cx, &y, cw, pstra("Intro animation"));
        if (Toggle(pstra("##st_intro"), ImVec2(cx + cw - 12.0f, cy), &ui.IntroAnimation, 32.0f, 17.0f))
            if (ui.IntroAnimation) TriggerIntro();
    }

    {
        float before = ui.SplashOpen;
        if (!ui.IntroAnimation) PushAlpha(0.5f);
        SettingsSlider(dl, cx, &y, cw, pstra("Splash open"), pstra("##st_splash"), &ui.SplashOpen, 0.3f, 3.0f, pstra("%.1fs"));
        if (!ui.IntroAnimation) { ui.SplashOpen = before; PopAlpha(); }
    }
    SettingsToggleRow(dl, cx, &y, cw, pstra("Logo glitch"), pstra("##st_glitch"), &ui.LogoGlitch);
    {

        float before = ui.LogoGlitchSpeed;
        if (!ui.LogoGlitch) PushAlpha(0.5f);
        SettingsSlider(dl, cx, &y, cw, pstra("Glitch speed"), pstra("##st_glspd"), &ui.LogoGlitchSpeed, 0.2f, 4.0f, pstra("%.1fx"));
        if (!ui.LogoGlitch) { ui.LogoGlitchSpeed = before; PopAlpha(); }
    }
    SettingsToggleRow(dl, cx, &y, cw, pstra("Wallpaper"), pstra("##st_wall"), &ui.Wallpaper);
    SettingsToggleRow(dl, cx, &y, cw, pstra("Particles"), pstra("##st_part"), &ui.Particles);

    content_h = (y + scroll) - view_y0;
    dl->PopClipRect();

    if (content_h > view_h)
    {
        float tx = max.x - 6.0f;
        float ratio = view_h / content_h;
        float bar_h = ImMax(view_h * ratio, 26.0f);
        float bar_y = view_y0 + (view_h - bar_h) * (scroll / ImMax(content_h - view_h, 1.0f));
        RectFilled(dl, ImVec2(tx - 1.5f, view_y0), ImVec2(tx + 1.5f, view_y1), C.separator, 1.5f);
        RectFilled(dl, ImVec2(tx - 1.5f, bar_y), ImVec2(tx + 1.5f, bar_y + bar_h), C.scroll, 1.5f);
    }

    ImVec2 hdr_min(min.x, min.y), hdr_max(max.x - 40.0f, min.y + head_h);
    Hitbox(pstra("##st_drag"), hdr_min, hdr_max);
    ImGuiID drag_id = ImGui::GetCurrentWindow()->GetID(pstra("##st_drag"));
    if (GImGui->ActiveId == drag_id && ImGui::IsMouseDown(0))
    {
        P.pos.x += ImGui::GetIO().MouseDelta.x;
        P.pos.y += ImGui::GetIO().MouseDelta.y;
        P.pos.x = ImClamp(P.pos.x, -w + 60.0f, ds.x - 60.0f);
        P.pos.y = ImClamp(P.pos.y, 0.0f, ds.y - 40.0f);
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    *out_min = min; *out_max = max;
}

static std::string VKeyName(int vk)
{
    char buf[8]{};
    if (vk >= 0x70 && vk <= 0x87) { snprintf(buf, sizeof(buf), pstra("F%d"), vk - 0x6F); return buf; }
    if (vk >= 'A' && vk <= 'Z')   return std::string(1, static_cast<char>(vk));
    if (vk >= '0' && vk <= '9')   return std::string(1, static_cast<char>(vk));
    switch (vk)
    {
    case 0x20: return pstra("Space");
    case 0x09: return pstra("Tab");
    case 0x0D: return pstra("Enter");
    case 0x08: return pstra("Back");
    case 0x2D: return pstra("Insert");
    case 0x2E: return pstra("Delete");
    case 0x24: return pstra("Home");
    case 0x23: return pstra("End");
    case 0x21: return pstra("PageUp");
    case 0x22: return pstra("PageDown");
    case 0x25: return pstra("Left");
    case 0x26: return pstra("Up");
    case 0x27: return pstra("Right");
    case 0x28: return pstra("Down");
    case 0x2C: return pstra("PrtSc");
    case 0x14: return pstra("Caps");
    case 0xC0: return pstra("`");  case 0xBD: return pstra("-");  case 0xBB: return pstra("=");
    case 0xDB: return pstra("[");  case 0xDD: return pstra("]");  case 0xDC: return pstra("\\");
    case 0xBA: return pstra(";");  case 0xDE: return pstra("'");  case 0xBC: return pstra(",");
    case 0xBE: return pstra(".");  case 0xBF: return pstra("/");
    }
    return {};
}

static bool CaptureBind(char* out, int cap)
{
    ImGuiIO& io = ImGui::GetIO();
    char mods[24]{};
    if (io.KeyCtrl)  strcat_s(mods, sizeof(mods), pstra("Ctrl + "));
    if (io.KeyShift) strcat_s(mods, sizeof(mods), pstra("Shift + "));
    if (io.KeyAlt)   strcat_s(mods, sizeof(mods), pstra("Alt + "));

    if (io.MouseClicked[3]) { snprintf(out, cap, pstra("%sMouse4"), mods); return true; }
    if (io.MouseClicked[4]) { snprintf(out, cap, pstra("%sMouse5"), mods); return true; }

    for (int vk = 0; vk < 256; vk++)
    {
        if (!io.KeysDown[vk]) continue;
        if (vk == 0x10 || vk == 0x11 || vk == 0x12) continue;
        if (vk >= 0xA0 && vk <= 0xA5) continue;
        if (vk == 0x1B) continue;
        const std::string nm = VKeyName(vk);
        if (nm.empty()) continue;
        snprintf(out, cap, pstra("%s%s"), mods, nm.c_str());
        return true;
    }
    return false;
}

static int CaptureVK()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseClicked[3]) return 0x05;
    if (io.MouseClicked[4]) return 0x06;
    for (int vk = 0; vk < 256; vk++)
    {
        if (!io.KeysDown[vk]) continue;
        if (vk == 0x01 || vk == 0x02) continue;
        if (vk == 0x10 || vk == 0x11 || vk == 0x12) continue;
        if (vk >= 0xA0 && vk <= 0xA5) continue;
        if (vk == 0x1B) continue;
        if (VKeyName(vk).empty()) continue;
        return vk;
    }
    return -1;
}

static std::string HKName(int vk)
{
    if (vk == 0x05) return pstra("Mouse4");
    if (vk == 0x06) return pstra("Mouse5");
    const std::string name = VKeyName(vk);
    return name.empty() ? std::string(pstra("None")) : name;
}

static void PopupHotkey(ImDrawList* dl, ImVec2* out_min, ImVec2* out_max)
{
    const float w        = 300.0f;
    const float key_h    = 40.0f;
    const float item_h   = 44.0f;
    const float pad      = 8.0f;

    int nk = ImClamp(S.aim_key_count, 0, (int)IM_ARRAYSIZE(S.aim_keys));

    float h = pad + nk * key_h + 10.0f + 3 * item_h + pad;

    ImVec2 ds = Screen();
    float x = ImClamp(P.amax.x - w * 0.4f, 12.0f, ds.x - w - 12.0f);
    float y = ImClamp(P.amax.y + 6.0f, 12.0f, ds.y - h - 12.0f);
    y -= (1.0f - P.anim) * 6.0f;

    ImVec2 min(x, y), max(x + w, y + h);
    PopupPanel(dl, min, max, 14.0f);

    float cy = min.y + pad;

    for (int k = 0; k < nk; k++)
    {
        float ry = cy + k * key_h;
        float mid = ry + key_h * 0.5f;

        char id[24];
        snprintf(id, sizeof(id), pstra("##hk_row%d"), k);
        bool rhov = false;
        bool rclick = Hitbox(id, ImVec2(min.x + 10.0f, ry + 2.0f),
                             ImVec2(max.x - 70.0f, ry + key_h - 2.0f), &rhov);
        if (S.aim_bind_edit == k)
            RectFilled(dl, ImVec2(min.x + 8.0f, ry + 2.0f), ImVec2(max.x - 8.0f, ry + key_h - 2.0f),
                       Fade(Accent(1.0f), 0.14f), 9.0f);
        else if (rhov)
            RectFilled(dl, ImVec2(min.x + 8.0f, ry + 2.0f), ImVec2(max.x - 8.0f, ry + key_h - 2.0f),
                       IM_COL32(255, 255, 255, 10), 9.0f);
        if (rclick) { S.aim_bind_edit = k; S.aim_capturing = false; }

        DrawIcon(dl, IC_SLIDERS, ImVec2(min.x + 24.0f, mid), 16.0f, C.text_dim, 1.4f);
        TextAt(dl, F_Label, ImVec2(min.x + 44.0f, mid - Measure(F_Label, S.aim_keys[k].key).y * 0.5f),
               C.text, S.aim_keys[k].key);

        snprintf(id, sizeof(id), pstra("##hk_eye%d"), k);
        if (IconButton(id, S.aim_keys[k].visible ? IC_EYE : IC_EYE_OFF,
                       ImVec2(max.x - 58.0f, mid), 15.0f, C.text_dim, 12.0f))
            S.aim_keys[k].visible = !S.aim_keys[k].visible;

        snprintf(id, sizeof(id), pstra("##hk_del%d"), k);
        if (IconButton(id, IC_TRASH, ImVec2(max.x - 28.0f, mid), 15.0f, C.text_dim, 12.0f))
        {
            for (int j = k; j < nk - 1; j++) S.aim_keys[j] = S.aim_keys[j + 1];
            S.aim_key_count--;
            if (S.aim_bind_edit == k) { S.aim_bind_edit = -1; S.aim_capturing = false; }
            else if (S.aim_bind_edit > k) S.aim_bind_edit--;
            break;
        }
    }
    cy += nk * key_h + 5.0f;

    dl->AddLine(ImVec2(min.x + 14.0f, cy), ImVec2(max.x - 14.0f, cy), IM_COL32(52, 52, 62, 220), 1.0f);
    cy += 5.0f;

    {
        ImVec2 rmin(min.x + 10.0f, cy), rmax(max.x - 10.0f, cy + item_h - 2.0f);
        bool hov = false;
        bool click = Hitbox(pstra("##hk_add"), rmin, rmax, &hov);
        if (hov) RectFilled(dl, rmin, rmax, IM_COL32(255, 255, 255, 10), 9.0f);
        float m = (rmin.y + rmax.y) * 0.5f;
        DrawIcon(dl, IC_COPY, ImVec2(rmin.x + 20.0f, m), 16.0f, C.text_dim, 1.4f);
        TextAt(dl, F_Label, ImVec2(rmin.x + 40.0f, m - Measure(F_Label, pstra("Add new hotkey")).y * 0.5f),
               C.text, pstra("Add new hotkey"));
        if (click && S.aim_key_count < (int)IM_ARRAYSIZE(S.aim_keys))
        {
            int ni = S.aim_key_count;
            snprintf(S.aim_keys[ni].key, sizeof(S.aim_keys[0].key), pstra("..."));
            S.aim_keys[ni].visible = true;
            S.aim_key_count++;

            S.aim_bind_edit = ni;
            S.aim_capturing = true;
        }
        cy += item_h;
    }

    {
        ImVec2 rmin(min.x + 10.0f, cy), rmax(max.x - 10.0f, cy + item_h - 2.0f);
        bool hov = false;
        bool click = Hitbox(pstra("##hk_fav"), rmin, rmax, &hov);
        if (hov) RectFilled(dl, rmin, rmax, IM_COL32(255, 255, 255, 10), 9.0f);
        float m = (rmin.y + rmax.y) * 0.5f;
        DrawIcon(dl, IC_BOOKMARK, ImVec2(rmin.x + 20.0f, m), 16.0f, C.text_dim, 1.4f);
        TextAt(dl, F_Label, ImVec2(rmin.x + 40.0f, m - Measure(F_Label, pstra("Favorites")).y * 0.5f),
               C.text, pstra("Favorites"));
        if (S.aim_favorites)
            DrawIcon(dl, IC_CHECK, ImVec2(rmax.x - 18.0f, m), 15.0f, C.text, 1.5f);
        if (click) S.aim_favorites = !S.aim_favorites;
        cy += item_h;
    }

    {
        ImVec2 rmin(min.x + 10.0f, cy), rmax(max.x - 10.0f, cy + item_h - 2.0f);
        bool hov = false;
        bool click = Hitbox(pstra("##hk_reset"), rmin, rmax, &hov);
        if (hov) RectFilled(dl, rmin, rmax, IM_COL32(255, 60, 60, 20), 9.0f);
        float m = (rmin.y + rmax.y) * 0.5f;
        ImU32 red = IM_COL32(235, 80, 80, 255);
        DrawIcon(dl, IC_RESET, ImVec2(rmin.x + 20.0f, m), 16.0f, red, 1.4f);
        TextAt(dl, F_Label, ImVec2(rmin.x + 40.0f, m - Measure(F_Label, pstra("Reset settings")).y * 0.5f),
               red, pstra("Reset settings"));
        if (click)
        {
            S.aim_key_count = 0;
            S.aim_key_mode = 0;
            S.aim_favorites = false;
            S.aim_bind_edit = -1;
            S.aim_capturing = false;
        }
        cy += item_h;
    }

    ImVec2 umin = min, umax = max;

    if (S.aim_bind_edit >= 0 && S.aim_bind_edit < S.aim_key_count)
    {
        int  ei = S.aim_bind_edit;
        const float ew = 210.0f;
        const float eh = 184.0f;

        float ex = max.x + 10.0f;
        if (ex + ew > ds.x - 12.0f) ex = min.x - ew - 10.0f;
        ex = ImClamp(ex, 12.0f, ds.x - ew - 12.0f);
        float ey = ImClamp(min.y, 12.0f, ds.y - eh - 12.0f);

        ImVec2 emin(ex, ey), emax(ex + ew, ey + eh);
        PopupPanel(dl, emin, emax, 14.0f);

        float ecy = emin.y + 14.0f;

        TextAt(dl, F_Med, ImVec2(emin.x + 16.0f, ecy), C.text, pstra("Edit bind"));
        if (IconButton(pstra("##bind_close"), IC_CLOSE, ImVec2(emax.x - 18.0f, ecy + 6.0f),
                       13.0f, C.text_dim, 11.0f))
        { S.aim_bind_edit = -1; S.aim_capturing = false; }
        ecy += 30.0f;

        {
            ImVec2 bmin(emin.x + 14.0f, ecy), bmax(emax.x - 14.0f, ecy + 42.0f);
            bool boxhov = false;
            bool boxclick = Hitbox(pstra("##bind_box"), bmin, bmax, &boxhov);
            ImU32 border = S.aim_capturing ? Accent(1.0f)
                         : (boxhov ? IM_COL32(90, 90, 105, 255) : IM_COL32(52, 52, 62, 255));
            RectFilled(dl, bmin, bmax, IM_COL32(26, 26, 32, 255), 10.0f);
            RectStroke(dl, bmin, bmax, border, 10.0f, ImDrawCornerFlags_All,
                       S.aim_capturing ? 1.6f : 1.0f);

            const std::string txt = S.aim_capturing ? std::string(pstra("Press a key...")) : std::string(S.aim_keys[ei].key);
            ImU32 tc = S.aim_capturing ? Accent(1.0f) : C.text;
            if (S.aim_capturing)
            {
                float pulse = 0.55f + 0.45f * sinf((float)ImGui::GetTime() * 6.0f);
                tc = Fade(Accent(1.0f), pulse);
            }
            TextCentered(dl, F_Med, ImVec2((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f), tc, txt.c_str());

            if (boxclick) S.aim_capturing = true;
        }
        ecy += 42.0f + 10.0f;

        {
            ImVec2 pmin2(emin.x + 14.0f, ecy), pmax2(emax.x - 14.0f, ecy + 34.0f);
            bool phov = false;
            bool pclick = Hitbox(pstra("##bind_plus"), pmin2, pmax2, &phov);
            ImU32 fill = S.aim_capturing ? Fade(Accent(1.0f), 0.9f)
                       : (phov ? IM_COL32(46, 46, 56, 255) : IM_COL32(34, 34, 42, 255));
            RectFilled(dl, pmin2, pmax2, fill, 9.0f);
            float pm = (pmin2.y + pmax2.y) * 0.5f;
            ImU32 pic = S.aim_capturing ? IM_COL32(20, 20, 24, 255) : C.text;
            DrawIcon(dl, IC_PLUS, ImVec2((pmin2.x + pmax2.x) * 0.5f - 40.0f, pm), 15.0f, pic, 1.6f);
            TextAt(dl, F_Label, ImVec2((pmin2.x + pmax2.x) * 0.5f - 26.0f,
                                       pm - Measure(F_Label, pstra("Add key")).y * 0.5f),
                   pic, pstra("Add key"));
            if (pclick) S.aim_capturing = !S.aim_capturing;
        }
        ecy += 34.0f + 12.0f;

        {
            ImVec2 smin(emin.x + 14.0f, ecy), smax(emax.x - 14.0f, ecy + 36.0f);
            RectFilled(dl, smin, smax, IM_COL32(30, 30, 38, 255), 10.0f);
            float halfx = (smin.x + smax.x) * 0.5f;

            static float slide = 0.0f;
            slide = ImLerp(slide, (float)S.aim_key_mode, ImClamp(ImGui::GetIO().DeltaTime * 14.0f, 0.0f, 1.0f));
            float pw = (smax.x - smin.x) * 0.5f - 6.0f;
            float px = ImLerp(smin.x + 3.0f, halfx + 3.0f, slide);
            RectFilled(dl, ImVec2(px, smin.y + 3.0f), ImVec2(px + pw, smax.y - 3.0f), Accent(1.0f), 8.0f);

            bool h0 = false, h1 = false;
            if (Hitbox(pstra("##hk_toggle"), smin, ImVec2(halfx, smax.y), &h0)) S.aim_key_mode = 0;
            if (Hitbox(pstra("##hk_hold"), ImVec2(halfx, smin.y), smax, &h1))   S.aim_key_mode = 1;

            ImU32 c0 = (S.aim_key_mode == 0) ? IM_COL32(20, 20, 24, 255) : C.text_dim;
            ImU32 c1 = (S.aim_key_mode == 1) ? IM_COL32(20, 20, 24, 255) : C.text_dim;
            TextCentered(dl, F_Label, ImVec2((smin.x + halfx) * 0.5f, (smin.y + smax.y) * 0.5f), c0, pstra("Toggle"));
            TextCentered(dl, F_Label, ImVec2((halfx + smax.x) * 0.5f, (smin.y + smax.y) * 0.5f), c1, pstra("Hold"));
        }

        if (S.aim_capturing)
        {
            char cap[24];
            if (CaptureBind(cap, sizeof(cap)))
            {
                snprintf(S.aim_keys[ei].key, sizeof(S.aim_keys[0].key), pstra("%s"), cap);
                S.aim_capturing = false;
            }
        }

        umin.x = ImMin(umin.x, emin.x); umin.y = ImMin(umin.y, emin.y);
        umax.x = ImMax(umax.x, emax.x); umax.y = ImMax(umax.y, emax.y);
    }

    *out_min = umin; *out_max = umax;
}

void Blade::RenderPopups()
{
    if (P.kind == PK_NONE && !CP.open) return;

    ImVec2 ds = Screen();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    PushDrawList(dl);

    float dt = ImGui::GetIO().DeltaTime;
    P.anim  = IsWhiteLabel() ? 1.0f : ImMin(P.anim  + dt * 12.0f, 1.0f);
    CP.anim = IsWhiteLabel() ? 1.0f : ImMin(CP.anim + dt * 12.0f, 1.0f);

    dl->ChannelsSplit(2);

    const ImVec2 sc_clip(100000.0f, 100000.0f);

    ImVec2 cpmin(0, 0), cpmax(0, 0);
    bool cp_was_open = CP.open;
    if (CP.open)
    {
        dl->ChannelsSetCurrent(1);
        dl->PushClipRect(ImVec2(0, 0), sc_clip, false);
        PopupColor(dl, &cpmin, &cpmax);
        dl->PopClipRect();
    }

    dl->ChannelsSetCurrent(0);
    dl->PushClipRect(ImVec2(0, 0), sc_clip, false);

    PopupKind kind = P.kind;
    ImVec2 pmin(0, 0), pmax(0, 0);

    if (kind != PK_NONE)
    {

        if (kind == PK_ITEMPICK)
            RectFilled(dl, ImVec2(0, 0), ds, IM_COL32(0, 0, 0, (int)(150 * P.anim)));

        switch (kind)
        {
        case PK_DROPDOWN: PopupDropdown(dl, &pmin, &pmax); break;
        case PK_PROFILE:  PopupProfile(dl, &pmin, &pmax);  break;
        case PK_MODULES:  PopupModules(dl, &pmin, &pmax);  break;
        case PK_ITEMPICK: PopupItemPick(dl, &pmin, &pmax); break;
        case PK_SETTINGS: PopupSettings(dl, &pmin, &pmax); break;
        case PK_HOTKEY:   PopupHotkey(dl, &pmin, &pmax);   break;
        default: break;
        }
    }

    dl->PopClipRect();
    dl->ChannelsMerge();

    if (cp_was_open && CP.open && cpmax.x > cpmin.x)
        Hitbox(pstra("##cp_body"), cpmin, cpmax);

    if (P.kind == kind && kind != PK_NONE && pmax.x > pmin.x)
    {

        if (Hitbox(pstra("##popup_body"), pmin, pmax) && CP.open)
            CloseColorPicker();
    }

    bool modal = (P.kind != PK_SETTINGS) || CP.open;
    if (modal && Hitbox(pstra("##popup_blocker"), ImVec2(0, 0), ds))
    {
        if (CP.open)                    CloseColorPicker();
        else if (P.kind != PK_SETTINGS) ClosePopup();
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape), false))
    {
        if (S.aim_capturing)            S.aim_capturing = false;
        else if (S.hk_capturing >= 0)   S.hk_capturing = -1;
        else if (CP.open)               CloseColorPicker();
        else if (P.kind != PK_SETTINGS) ClosePopup();
    }

    PopDrawList();
}
