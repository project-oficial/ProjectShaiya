#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"
#include "Core/Types.h"
#include "Game/Entities/EntityManager.h"
#include "Game/Skills/SkillManager.h"

#include <stdio.h>
#include <math.h>

using namespace Blade;

static const ProtectedStringList kTargetHudStyles{ pstra("HP Line"), pstra("HP Circle") };

static ImU32 ColorU32(const ImColor& color, float alpha = 1.0f)
{
    ImVec4 value = color.Value;
    value.w = alpha;
    return ImGui::GetColorU32(value);
}

static void PanelHeader(ImDrawList* dl, ImVec2 min, float w,
                        const char* title, const char* subtitle,
                        const char* close_id, bool* open)
{
    TextAt(dl, F_Title, ImVec2(min.x + 16.0f, min.y + 13.0f), C.text, title);
    TextAt(dl, F_Small, ImVec2(min.x + 16.0f, min.y + 30.0f), C.text_mute, subtitle);

    if (close_id && open)
        if (IconButton(close_id, IC_CLOSE, ImVec2(min.x + w - 20.0f, min.y + 19.0f), 12.0f, C.text_dim, 11.0f))
            *open = false;
}

void Blade::DrawWatermarkSettings(ImDrawList* dl)
{

    if (!S.wm_panel_open) return;

    struct Item { ProtectedText label; bool* value; };
    Item items[5] = {
        { pstra("Brand name"),   &S.wm_brand   },
        { pstra("Show FPS"),     &S.wm_fps     },
        { pstra("Server name"),  &S.wm_server  },
        { pstra("Latency"),      &S.wm_latency },
        { pstra("Current time"), &S.wm_time    },
    };

    const float w = 262.0f;
    const float head_h = 50.0f;
    const float row_h = 31.0f;
    const float pad = 10.0f;
    const int   n = 5;
    const float h = head_h + n * row_h + pad;

    ImVec2 hp = HudBegin(pstra("hud_wmset"), ImVec2(540.0f, 48.0f));
    const float x0 = hp.x, y0 = hp.y;
    ImVec2 min(x0, y0), max(x0 + w, y0 + h);

    Panel(dl, min, max, 14.0f);
    PanelHeader(dl, min, w, pstra("Watermark settings"),
                pstra("Additional display settings for details."), pstra("##wm_close"), &S.wm_panel_open);

    static int drag_idx = -1;
    static float drag_off = 0.0f;
    float mouse_y = ImGui::GetIO().MousePos.y;

    for (int slot = 0; slot < n; slot++)
    {
        int idx = S.wm_order[slot];
        if (idx < 0 || idx >= n) { idx = slot; S.wm_order[slot] = slot; }

        float row_y = y0 + head_h + slot * row_h;
        bool dragging = (drag_idx == idx);
        if (dragging)
            row_y = ImClamp(mouse_y - drag_off, y0 + head_h, y0 + head_h + (n - 1) * row_h);

        ImVec2 rmin(x0 + pad, row_y + 1.0f);
        ImVec2 rmax(x0 + w - pad, row_y + row_h - 2.0f);

        char id[48];

        bool row_hovered = ImGui::IsMouseHoveringRect(rmin, rmax);

        RectFilled(dl, rmin, rmax, dragging ? C.row_hover : (row_hovered ? C.row_hover : C.row), 8.0f);
        if (dragging)
            RectStroke(dl, rmin, rmax, Accent(0.6f), 8.0f);

        float cy = (rmin.y + rmax.y) * 0.5f;

        snprintf(id, sizeof(id), pstra("##wm_drag%d"), idx);
        bool h_drag = false;
        Hitbox(id, ImVec2(rmin.x + 2, rmin.y), ImVec2(rmin.x + 24, rmax.y), &h_drag);
        ImGuiID drag_id = ImGui::GetCurrentWindow()->GetID(id);
        if (GImGui->ActiveId == drag_id && ImGui::IsMouseDown(0))
        {
            if (drag_idx != idx) { drag_idx = idx; drag_off = mouse_y - (y0 + head_h + slot * row_h); }
        }
        DrawIcon(dl, IC_DRAG, ImVec2(rmin.x + 13.0f, cy), 14.0f,
                 (h_drag || dragging) ? C.text_dim : C.text_mute, 1.0f);

        ImVec2 ts = Measure(F_Body, items[idx].label);
        TextAt(dl, F_Body, ImVec2(rmin.x + 30.0f, cy - ts.y * 0.5f), C.text, items[idx].label);

        snprintf(id, sizeof(id), pstra("##wm_tg%d"), idx);
        Toggle(id, ImVec2(rmax.x - 8.0f, cy), items[idx].value, 32.0f, 17.0f);
    }

    if (drag_idx >= 0)
    {
        if (!ImGui::IsMouseDown(0))
        {
            int from = -1;
            for (int i = 0; i < n; i++) if (S.wm_order[i] == drag_idx) from = i;
            int to = (int)ImClamp(floorf((mouse_y - drag_off - (y0 + head_h)) / row_h + 0.5f), 0.0f, (float)(n - 1));
            if (from >= 0 && to != from)
            {
                int moved = S.wm_order[from];
                if (from < to) for (int i = from; i < to; i++) S.wm_order[i] = S.wm_order[i + 1];
                else           for (int i = from; i > to; i--) S.wm_order[i] = S.wm_order[i - 1];
                S.wm_order[to] = moved;
            }
            drag_idx = -1;
        }
    }

    if (drag_idx < 0)
        HudEnd(pstra("hud_wmset"), min, ImVec2(max.x, y0 + head_h));
}

static void DrawAlbumArt(ImDrawList* dl, ImVec2 min, float sz)
{
    ImVec2 max(min.x + sz, min.y + sz);

    RectFilled(dl, min, max, IM_COL32(92, 140, 168, 255), 8.0f);
    FadeTopRect(dl, min, ImVec2(max.x, min.y + sz * 0.6f), IM_COL32(160, 204, 222, 255), 8.0f);

    dl->PushClipRect(min, max, true);
    ImVec2 m1[3] = { ImVec2(min.x - 2, max.y), ImVec2(min.x + sz * 0.42f, min.y + sz * 0.34f), ImVec2(min.x + sz * 0.86f, max.y) };
    dl->AddConvexPolyFilled(m1, 3, IM_COL32(48, 84, 66, 255));
    ImVec2 m2[3] = { ImVec2(min.x + sz * 0.34f, max.y), ImVec2(min.x + sz * 0.78f, min.y + sz * 0.48f), ImVec2(max.x + 2, max.y) };
    dl->AddConvexPolyFilled(m2, 3, IM_COL32(36, 66, 52, 255));
    dl->AddCircleFilled(ImVec2(min.x + sz * 0.76f, min.y + sz * 0.22f), sz * 0.10f, IM_COL32(246, 226, 150, 255), 16);
    dl->PopClipRect();
    RectStroke(dl, min, max, IM_COL32(255, 255, 255, 26), 8.0f);
}

void Blade::DrawMediaPlayer(ImDrawList* dl)
{
    if (!S.hud_media) return;

    const float w = 268.0f, h = 108.0f;
    ImVec2 ds = Screen();
    ImVec2 hp = HudBegin(pstra("hud_media"), ImVec2(ds.x * 0.5f - w * 0.5f + 56.0f, 48.0f));
    float x0 = hp.x, y0 = hp.y;

    ImVec2 min(x0, y0), max(x0 + w, y0 + h);
    Panel(dl, min, max, 14.0f);

    const float art = 42.0f;
    DrawAlbumArt(dl, ImVec2(x0 + 14.0f, y0 + 14.0f), art);

    TextAt(dl, F_Title, ImVec2(x0 + 14.0f + art + 12.0f, y0 + 17.0f), C.text, pstra("Shangri-La"));
    TextAt(dl, F_Small, ImVec2(x0 + 14.0f + art + 12.0f, y0 + 34.0f), C.text_mute, pstra("The Kinks"));

    DrawIcon(dl, IC_SUN, ImVec2(x0 + w - 20.0f, y0 + 20.0f), 15.0f, IM_COL32(245, 205, 96, 255), 1.4f);

    float cy = y0 + 72.0f;
    TextAt(dl, F_Small, ImVec2(x0 + 16.0f, cy - Measure(F_Small, pstra("0:45")).y * 0.5f), C.text_dim, pstra("0:45"));
    TextRight(dl, F_Small, ImVec2(x0 + w - 16.0f, cy - Measure(F_Small, pstra("5:20")).y * 0.5f), C.text_dim, pstra("5:20"));

    float mid = x0 + w * 0.5f;
    if (IconButton(pstra("##md_prev"), IC_PREV, ImVec2(mid - 26.0f, cy), 13.0f, C.text, 11.0f))
        S.media_pos = 0.0f;
    if (IconButton(pstra("##md_play"), S.media_play ? IC_PAUSE : IC_NEXT, ImVec2(mid, cy), 13.0f, C.text, 11.0f))
        S.media_play = !S.media_play;
    if (IconButton(pstra("##md_next"), IC_NEXT, ImVec2(mid + 26.0f, cy), 13.0f, C.text, 11.0f))
        S.media_pos = S.media_len;

    if (S.media_play)
    {
        S.media_pos += ImGui::GetIO().DeltaTime;
        if (S.media_pos > S.media_len) S.media_pos = 0.0f;
    }
    SliderF(pstra("##md_seek"), ImVec2(x0 + 16.0f, y0 + h - 20.0f), ImVec2(x0 + w - 16.0f, y0 + h - 20.0f),
            &S.media_pos, 0.0f, S.media_len);

    HudEnd(pstra("hud_media"), min, max);
}

void Blade::DrawKeybinds(ImDrawList* dl)
{
    if (!S.hud_keybinds) return;

    struct Bind { ProtectedText action; ProtectedText mode; ProtectedText key; };
    static const Bind binds[] = {
        { pstra("Enable killaura"),   pstra("Toggle"), pstra("Mouse5")   },
        { pstra("Enable ESP"),        pstra("Hold"),   pstra("RShift")   },
        { pstra("Disable sprinting"), pstra("Toggle"), pstra("CapsLock") },
    };
    const int n = IM_ARRAYSIZE(binds);

    const float w = 330.0f;
    const float head_h = 48.0f;
    const float row_h = 31.0f;
    const float h = head_h + n * row_h + 12.0f;

    ImVec2 hp = HudBegin(pstra("hud_keybinds"), ImVec2(180.0f, 340.0f));
    const float x0 = hp.x, y0 = hp.y;
    ImVec2 min(x0, y0), max(x0 + w, y0 + h);
    Panel(dl, min, max, 14.0f);

    ImVec2 bmin(x0 + 14.0f, y0 + 12.0f), bmax(x0 + 40.0f, y0 + 38.0f);
    RectFilled(dl, bmin, bmax, C.row, 7.0f);
    DrawIcon(dl, IC_KEYBIND, ImVec2((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f), 15.0f, C.text_dim, 1.4f);
    TextAt(dl, F_Title, ImVec2(x0 + 48.0f, y0 + 25.0f - Measure(F_Title, pstra("Keybinds")).y * 0.5f), C.text, pstra("Keybinds"));

    if (IconButton(pstra("##kb_pin"), IC_PIN, ImVec2(x0 + w - 22.0f, y0 + 25.0f), 15.0f,
                   S.keybinds_pinned ? Accent(1.0f) : C.text_dim, 12.0f))
        S.keybinds_pinned = !S.keybinds_pinned;

    for (int i = 0; i < n; i++)
    {
        float cy = y0 + head_h + i * row_h + row_h * 0.5f;

        char id[48];
        snprintf(id, sizeof(id), pstra("##kb_dots%d"), i);
        IconButton(id, IC_DOTS, ImVec2(x0 + w - 22.0f, cy), 13.0f, C.text_mute, 10.0f);

        snprintf(id, sizeof(id), pstra("##kb_row%d"), i);
        bool hov = false;
        Hitbox(id, ImVec2(x0 + 8.0f, cy - row_h * 0.5f + 1), ImVec2(x0 + w - 8.0f, cy + row_h * 0.5f - 1), &hov);
        if (hov)
            RectFilled(dl, ImVec2(x0 + 8.0f, cy - row_h * 0.5f + 1), ImVec2(x0 + w - 8.0f, cy + row_h * 0.5f - 1),
                              IM_COL32(255, 255, 255, 10), 8.0f);

        TextAt(dl, F_Body, ImVec2(x0 + 18.0f, cy - Measure(F_Body, binds[i].action).y * 0.5f),
               C.text, binds[i].action);

        float right = x0 + w - 40.0f;
        ImVec2 ks = Measure(F_Body, binds[i].key);
        TextAt(dl, F_Body, ImVec2(right - ks.x, cy - ks.y * 0.5f), C.text, binds[i].key);

        ImVec2 ms = Measure(F_Body, binds[i].mode);
        TextAt(dl, F_Body, ImVec2(right - ks.x - 12.0f - ms.x, cy - ms.y * 0.5f), C.text_mute, binds[i].mode);
    }

    HudEnd(pstra("hud_keybinds"), min, max);
}

static void DrawPlayerHead(ImDrawList* dl, ImVec2 min, float sz)
{
    ImVec2 max(min.x + sz, min.y + sz);
    RectFilled(dl, min, max, IM_COL32(232, 232, 234, 255), 8.0f);

    const float u = sz / 16.0f;
    auto Px = [&](int x, int y, int wpx, int hpx, ImU32 col) {
        RectFilled(dl, ImVec2(min.x + x * u, min.y + y * u),
                          ImVec2(min.x + (x + wpx) * u, min.y + (y + hpx) * u), col);
    };

    dl->PushClipRect(min, max, true);

    const ImU32 dark  = IM_COL32(28, 28, 32, 255);
    const ImU32 mid   = IM_COL32(96, 96, 102, 255);
    const ImU32 light = IM_COL32(206, 206, 210, 255);

    Px(0, 0, 16, 4, dark);
    Px(0, 4, 2,  6, dark);
    Px(14, 4, 2, 6, dark);
    Px(3, 6, 3, 2, dark);
    Px(10, 6, 3, 2, dark);
    Px(4, 6, 1, 1, light);
    Px(11, 6, 1, 1, light);
    Px(7, 8, 2, 3, mid);
    Px(5, 12, 6, 1, dark);
    Px(0, 14, 16, 2, mid);

    dl->PopClipRect();
    RectStroke(dl, min, max, IM_COL32(0, 0, 0, 50), 8.0f);
}

static void DrawHpRing(ImDrawList* dl, ImVec2 c, float r, float pct)
{
    const float th = 4.0f;
    const float a0 = -IM_PI * 0.5f;

    dl->PathClear();
    dl->PathArcTo(c, r, a0, a0 + IM_PI * 2.0f, 48);
    dl->PathStroke(IM_COL32(44, 44, 52, 255), false, th);

    const int steps = 40;
    int filled = (int)(steps * ImClamp(pct, 0.0f, 1.0f));
    for (int i = 0; i < filled; i++)
    {
        float t0 = (float)i / steps, t1 = (float)(i + 1) / steps;
        float ang0 = a0 + t0 * IM_PI * 2.0f;
        float ang1 = a0 + t1 * IM_PI * 2.0f + 0.02f;

        float rr, gg, bb;
        ImGui::ColorConvertHSVtoRGB(t0 * 0.33f, 0.85f, 0.95f, rr, gg, bb);

        dl->PathClear();
        dl->PathArcTo(c, r, ang0, ang1, 4);
        dl->PathStroke(ImGui::GetColorU32(ImVec4(rr, gg, bb, 1.0f)), false, th);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), pstra("%d%%"), (int)(pct * 100.0f));
    TextCentered(dl, F_Body, c, C.text, buf);
}

void Blade::DrawTargetHud(ImDrawList* dl)
{
    if (!S.hud_target) return;

    unsigned int targetId = ShaiyaOverlay::SkillManager::GetSelectedTargetWorldId();
    if (targetId == 0 || targetId == 0xFFFFFFFF)
        return;

    const ShaiyaOverlay::MonsterEntity* target = nullptr;
    const auto& monsters = ShaiyaOverlay::EntityManager::GetNearbyMonsters();
    for (unsigned int i = 0; i < monsters.GetCount(); i++)
    {
        if (monsters[i].WorldId == targetId && monsters[i].Alive)
        {
            target = &monsters[i];
            break;
        }
    }

    if (!target || target->CurrentHp == 0)
        return;

    float hpRatio = target->MaxHp > 0 ? (float)target->CurrentHp / (float)target->MaxHp : 0.0f;
    hpRatio = ImClamp(hpRatio, 0.0f, 1.0f);
    S.target_hp = hpRatio;

    const float w = 300.0f, h = 68.0f;
    ImVec2 ds = Screen();

    ImVec2 hp = HudBegin(pstra("hud_target"), ImVec2(ds.x - 590.0f, ds.y * 0.5f - 30.0f));
    float x0 = hp.x, y0 = hp.y;

    ImVec2 min(x0, y0), max(x0 + w, y0 + h);
    Panel(dl, min, max, 12.0f);

    DrawPlayerHead(dl, ImVec2(x0 + 10.0f, y0 + 10.0f), 46.0f);

    float tx = x0 + 10.0f + 46.0f + 12.0f;

    if (S.th_nickname)
    {
        char targetSub[64];
        snprintf(targetSub, sizeof(targetSub), pstra("Lv.%u | Dist: %.1fm"), target->Level, target->Distance);

        TextAt(dl, F_Title, ImVec2(tx, y0 + 12.0f), C.text, target->Name);
        TextAt(dl, F_Tiny,  ImVec2(tx, y0 + 27.0f), C.text_mute, targetSub);
    }

    if (S.th_equipment)
    {
        static const Icon eq[5] = { IC_HELMET, IC_CHESTPLATE, IC_LEGGINGS, IC_BOOTS, IC_SWORD_SMALL };

        static const ImU32 bar[5] = {
            IM_COL32(82, 200, 92, 255), IM_COL32(82, 200, 92, 255),
            IM_COL32(82, 200, 92, 255), IM_COL32(196, 190, 74, 255),
            IM_COL32(150, 110, 240, 255)
        };
        for (int i = 0; i < 5; i++)
        {
            float ix = tx + 6.0f + i * 25.0f;
            DrawIcon(dl, eq[i], ImVec2(ix, y0 + 44.0f), 14.0f, C.text_dim, 1.3f);
            if (S.th_healthbar)
                RectFilled(dl, ImVec2(ix - 8.0f, y0 + 53.0f), ImVec2(ix + 8.0f, y0 + 55.0f), bar[i], 1.0f);
        }
    }

    if (S.th_hp_style == 1)
    {
        DrawHpRing(dl, ImVec2(x0 + w - 36.0f, y0 + h * 0.5f), 21.0f, S.target_hp);
    }
    else
    {
        const float cyy = y0 + h * 0.5f;
        float bx1 = x0 + w - 16.0f;
        float bx0 = bx1 - 70.0f;

        char buf[16]; snprintf(buf, sizeof(buf), pstra("%d%%"), (int)(S.target_hp * 100.0f));
        ImVec2 ts = Measure(F_Small, buf);

        TextAt(dl, F_Small, ImVec2((bx0 + bx1) * 0.5f - ts.x * 0.5f, cyy - ts.y - 1.0f),
               C.text, buf);

        float by = cyy + 7.0f;
        RectFilled(dl, ImVec2(bx0, by - 3.0f), ImVec2(bx1, by + 3.0f), IM_COL32(44, 44, 52, 255), 3.0f);
        float rr, gg, bb;
        ImGui::ColorConvertHSVtoRGB(S.target_hp * 0.33f, 0.85f, 0.95f, rr, gg, bb);
        RectFilled(dl, ImVec2(bx0, by - 3.0f), ImVec2(bx0 + (bx1 - bx0) * S.target_hp, by + 3.0f),
                          ImGui::GetColorU32(ImVec4(rr, gg, bb, 1.0f)), 3.0f);
    }

    HudEnd(pstra("hud_target"), min, max);
}

void Blade::DrawTargetHudSettings(ImDrawList* dl)
{

    if (!S.th_panel_open) return;

    const float w = 256.0f;
    const float head_h = 52.0f;
    const float row_h = 33.0f;
    const float h = head_h + 2 * row_h + 14.0f + 3 * row_h + 10.0f;

    ImVec2 ds = Screen();
    ImVec2 hp = HudBegin(pstra("hud_thset"), ImVec2(ds.x - 20.0f - w, ds.y * 0.5f - 12.0f));
    float x0 = hp.x, y0 = hp.y;

    ImVec2 min(x0, y0), max(x0 + w, y0 + h);
    Panel(dl, min, max, 14.0f);
    PanelHeader(dl, min, w, pstra("TargetHUD settings"),
                pstra("Additional display settings for details."), pstra("##th_close"), &S.th_panel_open);

    for (int i = 0; i < kTargetHudStyles.Count(); i++)
    {
        float ry = y0 + head_h + i * row_h;
        ImVec2 rmin(x0 + 10.0f, ry + 1.0f), rmax(x0 + w - 10.0f, ry + row_h - 2.0f);

        char id[40];
        snprintf(id, sizeof(id), pstra("##th_style%d"), i);
        bool hov = false;
        bool clicked = Hitbox(id, rmin, rmax, &hov);
        if (clicked) S.th_hp_style = i;

        bool sel = (S.th_hp_style == i);
        if (sel || hov)
            RectFilled(dl, rmin, rmax, sel ? C.row : IM_COL32(255, 255, 255, 10), 8.0f);

        float cy = (rmin.y + rmax.y) * 0.5f;
        ImVec2 ts = Measure(F_Body, kTargetHudStyles[i]);
        TextAt(dl, F_Body, ImVec2(rmin.x + 12.0f, cy - ts.y * 0.5f), C.text, kTargetHudStyles[i]);

        snprintf(id, sizeof(id), pstra("##th_radio%d"), i);
        if (Radio(id, ImVec2(rmax.x - 14.0f, cy), sel, 8.0f))
            S.th_hp_style = i;
    }

    float sep_y = y0 + head_h + 2 * row_h + 7.0f;
    dl->AddLine(ImVec2(x0 + 14.0f, sep_y), ImVec2(x0 + w - 14.0f, sep_y), IM_COL32(48, 48, 58, 200), 1.0f);

    struct Opt { ProtectedText label; bool* v; };
    Opt opts[3] = {
        { pstra("Display nickname"), &S.th_nickname  },
        { pstra("Show equipment"),   &S.th_equipment },
        { pstra("Show health bar"),  &S.th_healthbar },
    };
    for (int i = 0; i < 3; i++)
    {
        float cy = sep_y + 14.0f + i * row_h + row_h * 0.5f - 8.0f;
        ImVec2 ts = Measure(F_Body, opts[i].label);
        TextAt(dl, F_Body, ImVec2(x0 + 22.0f, cy - ts.y * 0.5f), C.text, opts[i].label);

        char id[40];
        snprintf(id, sizeof(id), pstra("##th_tg%d"), i);
        Toggle(id, ImVec2(x0 + w - 22.0f, cy), opts[i].v, 32.0f, 17.0f);
    }

    HudEnd(pstra("hud_thset"), min, ImVec2(max.x, y0 + head_h));
}

struct Notif { char text[96]; int type; float t0; bool active; };
static const int   NOTIF_MAX  = 6;
static const float NOTIF_LIFE = 4.6f;

static Notif g_notif[NOTIF_MAX];
static int   g_notif_next = 0;

static ImU32 NotifColor(int type)
{
    Settings::sInterface& ui = Settings::Get().Interface;
    switch (type)
    {
    case NT_SUCCESS: return ColorU32(ui.SuccessColor);
    case NT_WARNING: return ColorU32(ui.WarningColor);
    case NT_ERROR:   return ColorU32(ui.ErrorColor);
    default:         return ColorU32(ui.PrimaryColor);
    }
}
static Icon NotifIcon(int type)
{
    switch (type)
    {
    case NT_SUCCESS: return IC_CHECK;
    case NT_WARNING: return IC_WARNING;
    case NT_ERROR:   return IC_CLOSE;
    default:         return IC_BOLT;
    }
}

void Blade::PushNotification(const char* text, int type)
{
    Notif& nf = g_notif[g_notif_next];
    snprintf(nf.text, sizeof(nf.text), pstra("%s"), text ? text : pstra(""));
    nf.type   = type;
    nf.t0     = (float)ImGui::GetTime();
    nf.active = true;
    g_notif_next = (g_notif_next + 1) % NOTIF_MAX;
}

void Blade::ClearNotifications()
{
    for (int i = 0; i < NOTIF_MAX; i++) g_notif[i].active = false;
}

void Blade::DrawNotifications(ImDrawList* dl)
{
    if (!S.hud_notify) return;

    static bool seeded = false;
    if (!seeded) { seeded = true; PushNotification(pstra("Bem-vindo ao Project Oficial"), NT_INFO); }

    const float now = (float)ImGui::GetTime();

    struct Vis { int idx; float age; };
    Vis vis[NOTIF_MAX]; int nv = 0;
    for (int k = 0; k < NOTIF_MAX; k++)
    {
        int i = (g_notif_next + k) % NOTIF_MAX;
        if (!g_notif[i].active) continue;
        float age = now - g_notif[i].t0;
        if (age >= NOTIF_LIFE) { g_notif[i].active = false; continue; }
        vis[nv].idx = i; vis[nv].age = age; nv++;
    }
    if (nv == 0) return;

    const float w = 400.0f, h = 62.0f, gap = 12.0f;
    const float reserve = h * 3.0f + gap * 2.0f;

    ImVec2 ds = Screen();
    ImVec2 hp = HudBegin(pstra("hud_notify"), ImVec2(24.0f, ds.y - 24.0f - reserve));
    float x0 = hp.x;
    float anchor_bottom = hp.y + reserve;

    auto easeOut = [](float t) { float i = 1.0f - t; return 1.0f - i * i * i; };

    for (int v = 0; v < nv; v++)
    {
        const Notif& nf = g_notif[vis[v].idx];
        float age = vis[v].age;

        int   slot = (nv - 1) - v;
        float y0   = anchor_bottom - (slot + 1) * h - slot * gap;

        float tin  = easeOut(ImClamp(age / 0.30f, 0.0f, 1.0f));
        float tout = easeOut(ImClamp((NOTIF_LIFE - age) / 0.40f, 0.0f, 1.0f));
        float ap   = tin * tout;
        float dx   = (1.0f - tin) * -34.0f + (1.0f - tout) * -34.0f;

        PushAlpha(ap);
        ImVec2 mn(x0 + dx, y0), mx(x0 + dx + w, y0 + h);
        Panel(dl, mn, mx, 12.0f);

        ImU32 col = NotifColor(nf.type);
        float mid = (mn.y + mx.y) * 0.5f;

        DrawIcon(dl, NotifIcon(nf.type), ImVec2(mn.x + 30.0f, mid), 20.0f, col, 1.7f);

        dl->AddLine(ImVec2(mn.x + 56.0f, mn.y + 16.0f), ImVec2(mn.x + 56.0f, mx.y - 16.0f),
                    A(IM_COL32(255, 255, 255, 45)), 1.5f);

        ImVec2 ts = Measure(F_Med, nf.text);
        TextAt(dl, F_Med, ImVec2(mn.x + 72.0f, mid - ts.y * 0.5f), C.text, nf.text);

        float remain = ImClamp(1.0f - age / NOTIF_LIFE, 0.0f, 1.0f);
        RectFilled(dl, ImVec2(mn.x + 2.0f, mx.y - 5.0f),
                   ImVec2(mn.x + 2.0f + (w - 4.0f) * remain, mx.y - 2.0f), col, 1.5f);
        PopAlpha();
    }

    HudEnd(pstra("hud_notify"), ImVec2(x0, anchor_bottom - reserve), ImVec2(x0 + w, anchor_bottom));
}

void Blade::DrawWatermark(ImDrawList* dl)
{
    if (!S.hud_watermark) return;

    const char* brand = WhiteLabelName();
    if (!brand || !brand[0])
        brand = "Shaiya Assistant";

    char textBuf[128];
    snprintf(textBuf, sizeof(textBuf), "%s | FPS: %.0f | %s",
        brand, ImGui::GetIO().Framerate, IsWhiteLabel() ? "VIP" : "Release");

    ImVec2 ts = Measure(F_Body, textBuf);
    float padX = 14.0f;
    float w = ts.x + padX * 2.0f + 12.0f;
    float h = 30.0f;

    ImVec2 pos = HudBegin(pstra("hud_watermark"), ImVec2(20.0f, 20.0f));
    ImVec2 min(pos.x, pos.y);
    ImVec2 max(pos.x + w, pos.y + h);

    Panel(dl, min, max, 8.0f);
    dl->AddCircleFilled(ImVec2(min.x + 12.0f, min.y + h * 0.5f), 3.5f, Accent(1.0f));
    TextAt(dl, F_Body, ImVec2(min.x + 22.0f, min.y + (h - ts.y) * 0.5f), C.text, textBuf);

    HudEnd(pstra("hud_watermark"), min, max);
}

void Blade::DrawCoords(ImDrawList* dl)
{
    if (!S.hud_coords) return;

    const auto& player = ShaiyaOverlay::EntityManager::GetLocalPlayer();
    if (!player.Valid) return;

    char coordsBuf[96];
    snprintf(coordsBuf, sizeof(coordsBuf), "Pos: (%.1f, %.1f, %.1f)", player.Position.X, player.Position.Y, player.Position.Z);

    ImVec2 ts = Measure(F_Small, coordsBuf);
    float padX = 12.0f;
    float w = ts.x + padX * 2.0f;
    float h = 26.0f;

    ImVec2 screen = Screen();
    ImVec2 pos = HudBegin(pstra("hud_coords"), ImVec2(20.0f, screen.y - 40.0f));
    ImVec2 min(pos.x, pos.y);
    ImVec2 max(pos.x + w, pos.y + h);

    Panel(dl, min, max, 6.0f);
    TextAt(dl, F_Small, ImVec2(min.x + padX, min.y + (h - ts.y) * 0.5f), C.text_dim, coordsBuf);

    HudEnd(pstra("hud_coords"), min, max);
}
