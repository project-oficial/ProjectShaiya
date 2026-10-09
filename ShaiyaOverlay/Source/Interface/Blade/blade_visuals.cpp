#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"
#include "Game/Visuals/SkinChanger.h"

#include <algorithm>
#include <stdio.h>
#include <math.h>

using namespace Blade;

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

        L.y += gap;
        L.Card(4);
        L.Row(pstra("Special Wings (Type 121)"), F_Small);
        if (ActionButton(L, pstra("##wings_demon"), pstra("Asas de Demonio"), true))
        {
            ShaiyaOverlay::SkinChanger::SetWings(4);
            Blade::PushNotification(pstra("Asas de Demonio aplicadas!"), NT_SUCCESS);
        }
        if (ActionButton(L, pstra("##wings_angel"), pstra("Asas de Anjo"), false))
        {
            ShaiyaOverlay::SkinChanger::SetWings(1);
            Blade::PushNotification(pstra("Asas de Anjo aplicadas!"), NT_SUCCESS);
        }
        if (ActionButton(L, pstra("##wings_thor"), pstra("Asas de Thor"), false))
        {
            ShaiyaOverlay::SkinChanger::SetWings(5);
            Blade::PushNotification(pstra("Asas de Thor aplicadas!"), NT_SUCCESS);
        }

        R.Card(6);
        R.Row(pstra("Transformacoes (Costume & Armor Morphs)"), F_Small);
        if (ActionButton(R, pstra("##trans_dragon"), pstra("Dragao Negro (Guerreiro Dragao)"), true))
        {
            ShaiyaOverlay::SkinChanger::SetTransformation(93, 4, 20);
            Blade::PushNotification(pstra("Dragao Negro ativado!"), NT_SUCCESS);
        }
        if (ActionButton(R, pstra("##trans_gold"), pstra("Lamina de Ouro (Cavaleiro Dourado)"), false))
        {
            ShaiyaOverlay::SkinChanger::SetTransformation(52, 1, 20);
            Blade::PushNotification(pstra("Cavaleiro Dourado ativado!"), NT_SUCCESS);
        }
        if (ActionButton(R, pstra("##trans_raven"), pstra("O Corvo (Cavaleiro Sombrio)"), false))
        {
            ShaiyaOverlay::SkinChanger::SetTransformation(45, 4, 20);
            Blade::PushNotification(pstra("O Corvo ativado!"), NT_SUCCESS);
        }
        if (ActionButton(R, pstra("##trans_dracula"), pstra("Lord Dracula (Lorde Vampiro)"), false))
        {
            ShaiyaOverlay::SkinChanger::SetTransformation(67, 4, 20);
            Blade::PushNotification(pstra("Dracula ativado!"), NT_SUCCESS);
        }
        if (ActionButton(R, pstra("##trans_bear"), pstra("Urso de Batalha Gigante"), false))
        {
            ShaiyaOverlay::SkinChanger::SetTransformation(19, 0, 20);
            Blade::PushNotification(pstra("Urso de Batalha ativado!"), NT_SUCCESS);
        }

        R.y += gap;
        R.Card(3);
        if (ActionButton(R, pstra("##btn_apply_skin"), pstra("Recarregar Visual"), true))
        {
            ShaiyaOverlay::SkinChanger::GetConfig().Enabled = true;
            ShaiyaOverlay::SkinChanger::ApplySkins();
            Blade::PushNotification(pstra("Visual recarregado!"), NT_SUCCESS);
        }
        if (ActionButton(R, pstra("##btn_restore_skin"), pstra("Restaurar Itens Originais"), false))
        {
            ShaiyaOverlay::SkinChanger::RestoreOriginal();
            Blade::PushNotification(pstra("Itens originais restaurados."), NT_INFO);
        }
        R.Row(pstra("Client-Side Only (100% Seguro)"), F_Small);

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
