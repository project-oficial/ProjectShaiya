#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"
#include "Game/Visuals/SkinChanger.h"
#include "Core/StringUtils.h"

#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <vector>

using namespace Blade;
using namespace ShaiyaOverlay;

static Settings::sInterface& Ui()
{
    return Settings::Get().Interface;
}

static Shaiya::Config::VisualsConfig& VisualsCfg()
{
    return Settings::Get().Visuals;
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

static const float VROW_H = 40.0f;

struct VCol
{
    ImDrawList* dl;
    float x, y, w;
    int   rows = 0, i = 0, block = 0;
    float a = 1.0f;

    float Delay() const { return block * 0.045f; }
    void Card(int n) { rows = n; i = 0; }

    float Row(const char* label, ImFont* font = nullptr)
    {
        a = IntroT(Delay());
        float dy = (1.0f - a) * 12.0f;

        int corners = 0;
        if (i == 0)          corners |= ImDrawCornerFlags_Top;
        if (i == rows - 1)   corners |= ImDrawCornerFlags_Bot;

        ImVec2 rmin(x, y + dy), rmax(x + w, y + VROW_H + dy);
        RectFilled(dl, rmin, rmax, Fade(C.row, a), 10.0f, corners);

        if (i > 0)
            dl->AddLine(ImVec2(x + 14.0f, rmin.y), ImVec2(x + w - 14.0f, rmin.y),
                        Fade(C.divider, a), 1.0f);

        if (label)
        {
            ImFont* f = font ? font : F_Body;
            ImVec2 ts = Measure(f, label);
            const float text_padding = IsWhiteLabel() ? 14.0f : 16.0f;
            TextAt(dl, f, ImVec2(x + text_padding, rmin.y + (VROW_H - ts.y) * 0.5f), Fade(C.text, a), label);
        }

        float cy = rmin.y + VROW_H * 0.5f;
        y += VROW_H;
        i++; block++;
        return cy;
    }
};

static void DotAndCheck(VCol& c, float cy, const char* id, bool* v, ImVec4& col, const char* label)
{
    PushAlpha(c.a);
    char cid[64];
    snprintf(cid, sizeof(cid), pstra("%s_dot"), id);
    const bool white_label = IsWhiteLabel();
    const float control_padding = white_label ? 14.0f : 8.0f;
    const float swatch_spacing = white_label ? 52.0f : 54.0f;
    ImVec2 dot(c.x + c.w - control_padding - swatch_spacing, cy);
    ColorSwatch(cid, dot, 18.0f, &col.x, label);
    Toggle(id, ImVec2(c.x + c.w - control_padding, cy), v, 32.0f, 17.0f);
    PopAlpha();
}

static void Dot(VCol& c, float cy, const char* id, ImVec4& col, const char* label)
{
    PushAlpha(c.a);
    const bool white_label = IsWhiteLabel();
    const float control_padding = white_label ? 14.0f : 8.0f;
    ImVec2 dot(c.x + c.w - control_padding - 12.0f, cy);
    ColorSwatch(id, dot, 18.0f, &col.x, label);
    PopAlpha();
}

static void ToggleRow(VCol& c, const char* label, const char* id, bool* v)
{
    float cy = c.Row(label);
    PushAlpha(c.a);
    const bool white_label = IsWhiteLabel();
    const float control_padding = white_label ? 14.0f : 8.0f;
    Toggle(id, ImVec2(c.x + c.w - control_padding, cy), v, 32.0f, 17.0f);
    PopAlpha();
}

static void SliderRow(VCol& c, const char* label, const char* id, float* v, float lo, float hi, const char* fmt)
{
    float cy = c.Row(label);
    const float slider_padding = IsWhiteLabel() ? 14.0f : 16.0f;
    const float sx1 = c.x + c.w - slider_padding;
    const float sx0 = sx1 - 96.0f;
    PushAlpha(c.a);
    char buf[24];
    snprintf(buf, sizeof(buf), fmt, *v);
    ImVec2 ts = Measure(F_Body, buf);
    TextAt(c.dl, F_Body, ImVec2(sx0 - 10.0f - ts.x, cy - ts.y * 0.5f), C.text, buf);
    SliderF(id, ImVec2(sx0, cy), ImVec2(sx1, cy), v, lo, hi, c.a);
    PopAlpha();
}

static bool ActionButton(VCol& c, const char* id, const char* label, bool accent = false)
{
    float cy = c.Row(nullptr);
    const float btn_padding = IsWhiteLabel() ? 12.0f : 14.0f;
    ImVec2 bmin(c.x + btn_padding, cy - 13.0f);
    ImVec2 bmax(c.x + c.w - btn_padding, cy + 13.0f);
    PushAlpha(c.a);
    bool pressed = Button(id, bmin, bmax, label, accent);
    PopAlpha();
    return pressed;
}

static void VRowTextInput(VCol& c, const char* label, const char* id, char* buf, size_t bufSize, const char* placeholder = nullptr)
{
    float iw = 136.0f, ih = 22.0f;
    float cy = c.Row(label);
    const float control_padding = IsWhiteLabel() ? 14.0f : 8.0f;
    ImVec2 imin(c.x + c.w - control_padding - iw, cy - ih * 0.5f);
    ImVec2 imax(c.x + c.w - control_padding, cy + ih * 0.5f);

    ImGuiID input_id = ImGui::GetCurrentWindow()->GetID(id);
    static ImGuiID focused_input_id = 0;
    bool is_focused = (focused_input_id == input_id);

    bool hov = false;
    if (Hitbox(id, imin, imax, &hov))
    {
        focused_input_id = input_id;
        is_focused = true;
    }

    if (is_focused)
    {
        if (ImGui::IsMouseClicked(0) && !hov)
        {
            focused_input_id = 0;
            is_focused = false;
        }
        else
        {
            ImGuiIO& io = ImGui::GetIO();
            for (int n = 0; n < io.InputQueueCharacters.Size; n++)
            {
                ImWchar ch = io.InputQueueCharacters[n];
                if (ch >= 32 && ch < 255)
                {
                    size_t len = strlen(buf);
                    if (len + 1 < bufSize)
                    {
                        buf[len] = static_cast<char>(ch);
                        buf[len + 1] = '\0';
                    }
                }
            }
            if (ImGui::IsKeyPressed(0x08, true)) // Backspace
            {
                size_t len = strlen(buf);
                if (len > 0)
                    buf[len - 1] = '\0';
            }
            if (ImGui::IsKeyPressed(0x0D)) // Enter
            {
                focused_input_id = 0;
                is_focused = false;
            }
        }
    }

    ImDrawList* dl = CurDL();
    ImU32 border_col = is_focused ? Accent(1.0f) : (hov ? C.text_dim : C.border);
    RectFilled(dl, imin, imax, is_focused ? C.pill_hover : C.pill, 6.0f);
    RectStroke(dl, imin, imax, border_col, 6.0f);

    float text_x = imin.x + 8.0f;
    float text_y = cy - Measure(F_Small, "A").y * 0.5f;

    if (buf[0] != '\0')
    {
        char disp[64];
        const char* shown = FitEllipsis(F_Small, buf, iw - 16.0f, disp, sizeof(disp));
        TextAt(dl, F_Small, ImVec2(text_x, text_y), C.text, shown);
    }
    else if (placeholder)
    {
        char disp[64];
        const char* shown = FitEllipsis(F_Small, placeholder, iw - 16.0f, disp, sizeof(disp));
        TextAt(dl, F_Small, ImVec2(text_x, text_y), C.text_mute, shown);
    }
}

void Blade::DrawVisualsContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                        float cx0, float cy0, float cw, float pad)
{
    const float view_y0 = cy0 + 28.0f;
    const float view_y1 = max.y - 10.0f;
    const float view_h  = view_y1 - view_y0;
    const bool white_label = IsWhiteLabel();
    const float gap = white_label ? 14.0f : 12.0f;
    auto& cfg = VisualsCfg();

    static float scroll = 0.0f;
    static float content_h = 0.0f;

    ImVec2 full_min(cx0, view_y0), full_max(max.x - 4.0f, view_y1);

    int selected_module = S.module_sel[1];

    // Module 1: Skin Changer
    if (selected_module == 1)
    {
        const float col_w = (cw - pad * 2.0f - gap) * 0.5f;
        dl->PushClipRect(full_min, full_max, true);
        VCol L{ dl, cx0 + pad, view_y0, col_w };
        VCol R{ dl, cx0 + pad + col_w + gap, view_y0, col_w };

        auto& sc = ShaiyaOverlay::SkinChanger::GetConfig();

        L.Card(5);
        {
            float cy = L.Row(nullptr);
            PushAlpha(L.a);
            const float text_x = white_label ? L.x + pad : L.x + 16.0f;
            TextAt(dl, F_Label, ImVec2(text_x, cy - Measure(F_Label, pstra("Skin Changer Engine")).y * 0.5f), C.text, pstra("Skin Changer Engine"));
            Toggle(pstra("##v_skin_en"), ImVec2(L.x + L.w - 14.0f, cy), &sc.Enabled, 32.0f, 17.0f);
            PopAlpha();
        }
        float w1Glow = (float)sc.Weapon1Glow;
        SliderRow(L, pstra("Main Weapon Glow"), pstra("##v_w1glow"), &w1Glow, 0.0f, 20.0f, pstra("+%.0f"));
        if ((ShaiyaOverlay::U8)w1Glow != sc.Weapon1Glow)
        {
            sc.Weapon1Glow = (ShaiyaOverlay::U8)w1Glow;
            sc.OverrideGlow = true;
            if (sc.Enabled) ShaiyaOverlay::SkinChanger::ApplySkins();
        }

        float w2Glow = (float)sc.Weapon2Glow;
        SliderRow(L, pstra("Offhand/Shield Glow"), pstra("##v_w2glow"), &w2Glow, 0.0f, 20.0f, pstra("+%.0f"));
        if ((ShaiyaOverlay::U8)w2Glow != sc.Weapon2Glow)
        {
            sc.Weapon2Glow = (ShaiyaOverlay::U8)w2Glow;
            sc.OverrideGlow = true;
            if (sc.Enabled) ShaiyaOverlay::SkinChanger::ApplySkins();
        }

        float hairVal = (float)sc.Hair;
        SliderRow(L, pstra("Hair Style"), pstra("##v_hair"), &hairVal, 0.0f, 8.0f, pstra("Style %.0f"));
        if ((ShaiyaOverlay::U8)hairVal != sc.Hair)
        {
            sc.Hair = (ShaiyaOverlay::U8)hairVal;
            sc.OverrideHair = (hairVal > 0.0f);
            if (sc.Enabled) ShaiyaOverlay::SkinChanger::ApplySkins();
        }

        float faceVal = (float)sc.Face;
        SliderRow(L, pstra("Face Style"), pstra("##v_face"), &faceVal, 0.0f, 8.0f, pstra("Face %.0f"));
        if ((ShaiyaOverlay::U8)faceVal != sc.Face)
        {
            sc.Face = (ShaiyaOverlay::U8)faceVal;
            sc.OverrideFace = (faceVal > 0.0f);
            if (sc.Enabled) ShaiyaOverlay::SkinChanger::ApplySkins();
        }

        // Ensure catalog is loaded
        const auto& allWings = ShaiyaOverlay::SkinChanger::GetAvailableWings();
        const auto& allCostumes = ShaiyaOverlay::SkinChanger::GetAvailableCostumes();

        static char s_wings_filter[48] = { 0 };
        static float s_wings_scroll = 0.0f;
        static bool s_wings_dragging = false;

        static char s_costume_filter[48] = { 0 };
        static float s_costume_scroll = 0.0f;
        static bool s_costume_dragging = false;

        static ImVec2 s_wings_box_min(0, 0), s_wings_box_max(0, 0);
        static ImVec2 s_costume_box_min(0, 0), s_costume_box_max(0, 0);

        std::vector<const ShaiyaOverlay::SkinItemInfo*> matchedWings;
        matchedWings.reserve(allWings.size());
        for (const auto& w : allWings)
        {
            if (s_wings_filter[0] != '\0' && !StringUtils::ContainsCaseInsensitive(w.Name, s_wings_filter))
                continue;
            matchedWings.push_back(&w);
        }

        std::vector<const ShaiyaOverlay::SkinItemInfo*> matchedCostumes;
        matchedCostumes.reserve(allCostumes.size());
        for (const auto& cItem : allCostumes)
        {
            if (s_costume_filter[0] != '\0' && !StringUtils::ContainsCaseInsensitive(cItem.Name, s_costume_filter))
                continue;
            matchedCostumes.push_back(&cItem);
        }

        // Left Column - Card 2: Appearance Actions
        L.y += gap;
        L.Card(2);
        if (ActionButton(L, pstra("##btn_apply_skin"), pstra("Reload Appearance"), true))
        {
            ShaiyaOverlay::SkinChanger::GetConfig().Enabled = true;
            ShaiyaOverlay::SkinChanger::ApplySkins();
            Blade::PushNotification(pstra("Appearance reloaded!"), NT_SUCCESS);
        }
        if (ActionButton(L, pstra("##btn_restore_skin"), pstra("Restore Original Equipment"), false))
        {
            ShaiyaOverlay::SkinChanger::RestoreOriginal();
            Blade::PushNotification(pstra("Original equipment restored."), NT_INFO);
        }

        // Left Column - Card 3: Wings Selector (Type 121)
        L.y += gap;
        L.Card(2);
        VRowTextInput(L, pstra("Filter Wings"), pstra("##v_wings_flt"), s_wings_filter, sizeof(s_wings_filter), pstra("Search wings..."));

        bool wingsActive = sc.OverrideWings && sc.WingsTypeId > 0;
        char wingsUnequipLabel[64];
        snprintf(wingsUnequipLabel, sizeof(wingsUnequipLabel), wingsActive ? pstra("Unequip Wings (Current: #%u)") : pstra("Unequip Wings"), sc.WingsTypeId);
        if (ActionButton(L, pstra("##wings_none"), wingsUnequipLabel, false))
        {
            ShaiyaOverlay::SkinChanger::SetWings(0);
            Blade::PushNotification(pstra("Wings unequipped."), NT_INFO);
        }

        // Inner scroll container for Wings
        const float inner_wings_h = 160.0f;
        const float wing_item_h = 32.0f;
        float wings_content_h = matchedWings.empty() ? 40.0f : (matchedWings.size() * wing_item_h);

        ImVec2 w_gmin(L.x, L.y + 4.0f);
        ImVec2 w_gmax(L.x + L.w, w_gmin.y + inner_wings_h);
        s_wings_box_min = w_gmin;
        s_wings_box_max = w_gmax;

        if (wings_content_h > inner_wings_h)
        {
            float tx = w_gmax.x - 6.0f;
            float ratio = inner_wings_h / wings_content_h;
            float bh = ImMax(inner_wings_h * ratio, 20.0f);
            ImVec2 track_min(tx - 6.0f, w_gmin.y);
            ImVec2 track_max(w_gmax.x, w_gmax.y);

            if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(track_min, track_max))
                s_wings_dragging = true;
            if (!ImGui::IsMouseDown(0))
                s_wings_dragging = false;

            if (s_wings_dragging)
            {
                float my = ImGui::GetIO().MousePos.y - w_gmin.y - bh * 0.5f;
                float norm = ImClamp(my / ImMax(inner_wings_h - bh, 1.0f), 0.0f, 1.0f);
                s_wings_scroll = norm * (wings_content_h - inner_wings_h);
            }
        }
        else
            s_wings_dragging = false;

        if ((ImGui::IsMouseHoveringRect(w_gmin, w_gmax) || s_wings_dragging) && ImGui::GetIO().MouseWheel != 0.0f)
            s_wings_scroll -= ImGui::GetIO().MouseWheel * 32.0f;
        s_wings_scroll = ImClamp(s_wings_scroll, 0.0f, ImMax(wings_content_h - inner_wings_h, 0.0f));

        RectFilled(dl, w_gmin, w_gmax, Fade(C.row, L.a * 0.65f), 8.0f);
        RectStroke(dl, w_gmin, w_gmax, Fade(C.divider, L.a), 8.0f);

        dl->PushClipRect(w_gmin, w_gmax, true);
        if (matchedWings.empty())
        {
            const char* emptyMsg = pstra("No wings found matching filter.");
            ImVec2 ems = Measure(F_Small, emptyMsg);
            TextAt(dl, F_Small, ImVec2(w_gmin.x + (L.w - ems.x) * 0.5f, w_gmin.y + (inner_wings_h - ems.y) * 0.5f), Fade(C.text_mute, L.a), emptyMsg);
        }
        else
        {
            float cur_y = w_gmin.y + 2.0f - s_wings_scroll;
            for (size_t i = 0; i < matchedWings.size(); ++i)
            {
                float row_y0 = cur_y + i * wing_item_h;
                float row_y1 = row_y0 + wing_item_h;
                if (row_y1 >= w_gmin.y && row_y0 <= w_gmax.y)
                {
                    ImVec2 rmin(w_gmin.x + 4.0f, row_y0);
                    ImVec2 rmax(w_gmax.x - (wings_content_h > inner_wings_h ? 12.0f : 4.0f), row_y1);

                    bool hov = ImGui::IsMouseHoveringRect(rmin, rmax);
                    if (hov)
                        RectFilled(dl, rmin, rmax, Fade(C.row_hover, L.a), 5.0f);

                    if (i > 0)
                        dl->AddLine(ImVec2(rmin.x + 6.0f, row_y0), ImVec2(rmax.x - 6.0f, row_y0), Fade(C.divider, L.a * 0.5f), 1.0f);

                    const auto& w = *matchedWings[i];
                    bool isEquipped = sc.OverrideWings && (sc.WingsTypeId == w.TypeId);

                    char label[96];
                    snprintf(label, sizeof(label), pstra("[#%u] %s"), w.TypeId, w.Name);
                    char disp[96];
                    float text_max_w = rmax.x - rmin.x - 72.0f;
                    const char* shown = FitEllipsis(F_Small, label, text_max_w, disp, sizeof(disp));
                    ImVec2 ts = Measure(F_Small, shown);
                    TextAt(dl, F_Small, ImVec2(rmin.x + 8.0f, (row_y0 + row_y1) * 0.5f - ts.y * 0.5f), isEquipped ? Accent(1.0f) : Fade(C.text, L.a), shown);

                    float bw = 58.0f, bh = 22.0f;
                    ImVec2 bmin(rmax.x - bw - 4.0f, (row_y0 + row_y1) * 0.5f - bh * 0.5f);
                    ImVec2 bmax(rmax.x - 4.0f, (row_y0 + row_y1) * 0.5f + bh * 0.5f);
                    char bid[32]; snprintf(bid, sizeof(bid), pstra("##eq_w_%u"), w.TypeId);
                    PushAlpha(L.a);
                    if (Button(bid, bmin, bmax, isEquipped ? pstra("Equipped") : pstra("Equip"), isEquipped))
                    {
                        ShaiyaOverlay::SkinChanger::SetWings(w.TypeId);
                        Blade::PushNotification(pstra("Wings equipped!"), NT_SUCCESS);
                    }
                    PopAlpha();
                }
            }
        }
        dl->PopClipRect();

        if (wings_content_h > inner_wings_h)
        {
            float tx = w_gmax.x - 5.0f;
            float ratio = inner_wings_h / wings_content_h;
            float bh = ImMax(inner_wings_h * ratio, 20.0f);
            float by = w_gmin.y + (inner_wings_h - bh) * (s_wings_scroll / ImMax(wings_content_h - inner_wings_h, 1.0f));
            RectFilled(dl, ImVec2(tx - 1.5f, w_gmin.y + 4.0f), ImVec2(tx + 1.5f, w_gmax.y - 4.0f), Fade(C.separator, L.a), 1.5f);
            RectFilled(dl, ImVec2(tx - 1.5f, by), ImVec2(tx + 1.5f, by + bh), Fade(s_wings_dragging ? C.accent : C.scroll, L.a), 1.5f);
        }

        L.y = w_gmax.y + gap;

        // Right Column: Costumes & Transformations Selector (Type 150)
        R.Card(2);
        VRowTextInput(R, pstra("Filter Costumes"), pstra("##v_cost_flt"), s_costume_filter, sizeof(s_costume_filter), pstra("Search costumes..."));

        bool costumeActive = sc.OverrideCostume && sc.CostumeTypeId > 0;
        char costUnequipLabel[64];
        snprintf(costUnequipLabel, sizeof(costUnequipLabel), costumeActive ? pstra("Unequip Costume (Current: #%u)") : pstra("Unequip Costume"), sc.CostumeTypeId);
        if (ActionButton(R, pstra("##cost_none"), costUnequipLabel, false))
        {
            sc.OverrideCostume = false;
            sc.CostumeTypeId = 0;
            ShaiyaOverlay::SkinChanger::ApplySkins();
            Blade::PushNotification(pstra("Costume unequipped."), NT_INFO);
        }

        // Inner scroll container for Costumes
        const float inner_costume_h = 320.0f;
        const float cost_item_h = 32.0f;
        float costume_content_h = matchedCostumes.empty() ? 40.0f : (matchedCostumes.size() * cost_item_h);

        ImVec2 c_gmin(R.x, R.y + 4.0f);
        ImVec2 c_gmax(R.x + R.w, c_gmin.y + inner_costume_h);
        s_costume_box_min = c_gmin;
        s_costume_box_max = c_gmax;

        if (costume_content_h > inner_costume_h)
        {
            float tx = c_gmax.x - 6.0f;
            float ratio = inner_costume_h / costume_content_h;
            float bh = ImMax(inner_costume_h * ratio, 20.0f);
            ImVec2 track_min(tx - 6.0f, c_gmin.y);
            ImVec2 track_max(c_gmax.x, c_gmax.y);

            if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(track_min, track_max))
                s_costume_dragging = true;
            if (!ImGui::IsMouseDown(0))
                s_costume_dragging = false;

            if (s_costume_dragging)
            {
                float my = ImGui::GetIO().MousePos.y - c_gmin.y - bh * 0.5f;
                float norm = ImClamp(my / ImMax(inner_costume_h - bh, 1.0f), 0.0f, 1.0f);
                s_costume_scroll = norm * (costume_content_h - inner_costume_h);
            }
        }
        else
            s_costume_dragging = false;

        if ((ImGui::IsMouseHoveringRect(c_gmin, c_gmax) || s_costume_dragging) && ImGui::GetIO().MouseWheel != 0.0f)
            s_costume_scroll -= ImGui::GetIO().MouseWheel * 32.0f;
        s_costume_scroll = ImClamp(s_costume_scroll, 0.0f, ImMax(costume_content_h - inner_costume_h, 0.0f));

        RectFilled(dl, c_gmin, c_gmax, Fade(C.row, R.a * 0.65f), 8.0f);
        RectStroke(dl, c_gmin, c_gmax, Fade(C.divider, R.a), 8.0f);

        dl->PushClipRect(c_gmin, c_gmax, true);
        if (matchedCostumes.empty())
        {
            const char* emptyMsg = pstra("No costumes found matching filter.");
            ImVec2 ems = Measure(F_Small, emptyMsg);
            TextAt(dl, F_Small, ImVec2(c_gmin.x + (R.w - ems.x) * 0.5f, c_gmin.y + (inner_costume_h - ems.y) * 0.5f), Fade(C.text_mute, R.a), emptyMsg);
        }
        else
        {
            float cur_y = c_gmin.y + 2.0f - s_costume_scroll;
            for (size_t i = 0; i < matchedCostumes.size(); ++i)
            {
                float row_y0 = cur_y + i * cost_item_h;
                float row_y1 = row_y0 + cost_item_h;
                if (row_y1 >= c_gmin.y && row_y0 <= c_gmax.y)
                {
                    ImVec2 rmin(c_gmin.x + 4.0f, row_y0);
                    ImVec2 rmax(c_gmax.x - (costume_content_h > inner_costume_h ? 12.0f : 4.0f), row_y1);

                    bool hov = ImGui::IsMouseHoveringRect(rmin, rmax);
                    if (hov)
                        RectFilled(dl, rmin, rmax, Fade(C.row_hover, R.a), 5.0f);

                    if (i > 0)
                        dl->AddLine(ImVec2(rmin.x + 6.0f, row_y0), ImVec2(rmax.x - 6.0f, row_y0), Fade(C.divider, R.a * 0.5f), 1.0f);

                    const auto& cItem = *matchedCostumes[i];
                    bool isEquipped = sc.OverrideCostume && (sc.CostumeTypeId == cItem.TypeId);

                    char label[96];
                    snprintf(label, sizeof(label), pstra("[#%u] %s"), cItem.TypeId, cItem.Name);
                    char disp[96];
                    float text_max_w = rmax.x - rmin.x - 72.0f;
                    const char* shown = FitEllipsis(F_Small, label, text_max_w, disp, sizeof(disp));
                    ImVec2 ts = Measure(F_Small, shown);
                    TextAt(dl, F_Small, ImVec2(rmin.x + 8.0f, (row_y0 + row_y1) * 0.5f - ts.y * 0.5f), isEquipped ? Accent(1.0f) : Fade(C.text, R.a), shown);

                    float bw = 58.0f, bh = 22.0f;
                    ImVec2 bmin(rmax.x - bw - 4.0f, (row_y0 + row_y1) * 0.5f - bh * 0.5f);
                    ImVec2 bmax(rmax.x - 4.0f, (row_y0 + row_y1) * 0.5f + bh * 0.5f);
                    char bid[32]; snprintf(bid, sizeof(bid), pstra("##eq_c_%u"), cItem.TypeId);
                    PushAlpha(R.a);
                    if (Button(bid, bmin, bmax, isEquipped ? pstra("Equipped") : pstra("Equip"), isEquipped))
                    {
                        ShaiyaOverlay::SkinChanger::SetTransformation(cItem.TypeId, sc.WingsTypeId, sc.Weapon1Glow);
                        Blade::PushNotification(pstra("Costume equipped!"), NT_SUCCESS);
                    }
                    PopAlpha();
                }
            }
        }
        dl->PopClipRect();

        if (costume_content_h > inner_costume_h)
        {
            float tx = c_gmax.x - 5.0f;
            float ratio = inner_costume_h / costume_content_h;
            float bh = ImMax(inner_costume_h * ratio, 20.0f);
            float by = c_gmin.y + (inner_costume_h - bh) * (s_costume_scroll / ImMax(costume_content_h - inner_costume_h, 1.0f));
            RectFilled(dl, ImVec2(tx - 1.5f, c_gmin.y + 4.0f), ImVec2(tx + 1.5f, c_gmax.y - 4.0f), Fade(C.separator, R.a), 1.5f);
            RectFilled(dl, ImVec2(tx - 1.5f, by), ImVec2(tx + 1.5f, by + bh), Fade(s_costume_dragging ? C.accent : C.scroll, R.a), 1.5f);
        }

        R.y = c_gmax.y + gap;

        dl->PopClipRect();
        return;
    }

    // Module 2: Loot Snaplines
    if (selected_module == 2)
    {
        const float col_w = (cw - pad * 2.0f - gap) * 0.5f;
        dl->PushClipRect(full_min, full_max, true);
        VCol L{ dl, cx0 + pad, view_y0, col_w };
        VCol R{ dl, cx0 + pad + col_w + gap, view_y0, col_w };

        L.Card(3);
        {
            float cy = L.Row(nullptr);
            PushAlpha(L.a);
            const float text_x = white_label ? L.x + pad : L.x + 16.0f;
            TextAt(dl, F_Label, ImVec2(text_x, cy - Measure(F_Label, pstra("Ground Loot ESP")).y * 0.5f), C.text, pstra("Ground Loot ESP"));
            Toggle(pstra("##v_loot_esp"), ImVec2(L.x + L.w - 14.0f, cy), &cfg.Loot, 32.0f, 17.0f);
            PopAlpha();
        }
        ToggleRow(L, pstra("Loot Snaplines"), pstra("##v_loot_lines"), &cfg.LootSnaplines);
        Dot(L, L.Row(pstra("Loot Color")), pstra("##v_loot_col"), cfg.LootColor, pstra("Loot Color"));

        R.Card(3);
        R.Row(pstra("Quest Items: Magenta Glow"), F_Small);
        R.Row(pstra("Gold Drops: Golden Line"), F_Small);
        R.Row(pstra("Near Items (<10m): Bright Green"), F_Small);

        dl->PopClipRect();
        return;
    }

    // Module 3: Quest Waypoints
    if (selected_module == 3)
    {
        const float col_w = (cw - pad * 2.0f - gap) * 0.5f;
        dl->PushClipRect(full_min, full_max, true);
        VCol L{ dl, cx0 + pad, view_y0, col_w };
        VCol R{ dl, cx0 + pad + col_w + gap, view_y0, col_w };

        L.Card(2);
        {
            float cy = L.Row(nullptr);
            PushAlpha(L.a);
            const float text_x = white_label ? L.x + pad : L.x + 16.0f;
            TextAt(dl, F_Label, ImVec2(text_x, cy - Measure(F_Label, pstra("Quest Waypoints")).y * 0.5f), C.text, pstra("Quest Waypoints"));
            Toggle(pstra("##v_quest_wp"), ImVec2(L.x + L.w - 14.0f, cy), &cfg.QuestWaypoints, 32.0f, 17.0f);
            PopAlpha();
        }
        Dot(L, L.Row(pstra("Waypoint Color")), pstra("##v_quest_col"), cfg.QuestColor, pstra("Waypoint Color"));

        R.Card(2);
        R.Row(pstra("Auto navigates to turn-in NPCs"), F_Small);
        R.Row(pstra("Displays distance to active objectives"), F_Small);

        dl->PopClipRect();
        return;
    }

    // Module 0: Entity ESP (Monsters & NPCs)
    const float col_w = (cw - pad * 2.0f - gap) * 0.5f;

    if (ImGui::IsMouseHoveringRect(full_min, full_max) && ImGui::GetIO().MouseWheel != 0.0f)
        scroll -= ImGui::GetIO().MouseWheel * 42.0f;
    scroll = ImClamp(scroll, 0.0f, ImMax(content_h - view_h, 0.0f));

    dl->PushClipRect(full_min, full_max, true);

    VCol L{ dl, cx0 + pad, view_y0 - scroll, col_w };
    VCol R{ dl, cx0 + pad + col_w + gap, view_y0 - scroll, col_w };
    const float base_y = L.y;

    L.Card(3);
    {
        float cy = L.Row(nullptr);
        PushAlpha(L.a);
        if (!white_label)
            DrawIcon(dl, IC_EYE, ImVec2(L.x + 26.0f, cy), 16.0f, Accent(1.0f), 1.5f);
        const float text_x = white_label ? L.x + pad : L.x + 42.0f;
        TextAt(dl, F_Label, ImVec2(text_x, cy - Measure(F_Label, pstra("Monster ESP")).y * 0.5f), C.text, pstra("Monster ESP"));
        Toggle(pstra("##v_mob_esp"), ImVec2(L.x + L.w - 14.0f, cy), &cfg.MonsterEsp, 32.0f, 17.0f);
        PopAlpha();
    }
    ToggleRow(L, pstra("Distance Tag"), pstra("##v_mob_dist"), &cfg.MonsterDist);
    Dot(L, L.Row(pstra("Marker Color")), pstra("##v_mob_col"), cfg.MonsterColor, pstra("Marker Color"));

    L.y += gap;
    L.Card(2);
    {
        float cy = L.Row(nullptr);
        PushAlpha(L.a);
        const float text_x = white_label ? L.x + pad : L.x + 16.0f;
        TextAt(dl, F_Label, ImVec2(text_x, cy - Measure(F_Label, pstra("NPC & Quest Givers")).y * 0.5f), C.text, pstra("NPC & Quest Givers"));
        Toggle(pstra("##v_npc_esp"), ImVec2(L.x + L.w - 14.0f, cy), &cfg.NpcEsp, 32.0f, 17.0f);
        PopAlpha();
    }
    Dot(L, L.Row(pstra("NPC Color")), pstra("##v_npc_col"), cfg.NpcColor, pstra("NPC Color"));

    R.Card(3);
    R.Row(pstra("Visual Information"), F_Small);
    R.Row(pstra("Name, HP & Level displayed by game natively"), F_Small);
    R.Row(pstra("Overlay displays clean distance indicator"), F_Small);

    content_h = ImMax(L.y, R.y) - base_y;

    dl->PopClipRect();

    if (content_h > view_h)
    {
        float track_x = max.x - 7.0f;
        float ratio = view_h / content_h;
        float bar_h = ImMax(view_h * ratio, 24.0f);
        float bar_y = view_y0 + (view_h - bar_h) * (scroll / ImMax(content_h - view_h, 1.0f));

        RectFilled(dl, ImVec2(track_x - 1.5f, view_y0), ImVec2(track_x + 1.5f, view_y1), C.separator, 1.5f);
        RectFilled(dl, ImVec2(track_x - 1.5f, bar_y), ImVec2(track_x + 1.5f, bar_y + bar_h), C.scroll, 1.5f);
    }
}
