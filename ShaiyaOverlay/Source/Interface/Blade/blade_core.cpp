#include "blade_ui.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Config/Settings.h"
#include "../Fonts/Louis_George_Cafe_Bold_ttf.h"

#include <map>
#include <math.h>
#include <stdio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

using namespace Blade;


ImFont* Blade::F_Tiny = nullptr;
ImFont* Blade::F_Small = nullptr;
ImFont* Blade::F_Body = nullptr;
ImFont* Blade::F_Med = nullptr;
ImFont* Blade::F_Label = nullptr;
ImFont* Blade::F_Title = nullptr;
ImFont* Blade::F_Head = nullptr;
ImFont* Blade::F_Logo = nullptr;

static ImFont* g_standard_fonts[8] = {};
static ImFont* g_white_label_fonts[8] = {};

Palette Blade::C ;
State   Blade::S ;

static std::map<ImGuiID, float> g_blade_anim_values;
static std::map<ImGuiID, float> g_blade_slider_load_values;

static Settings::sInterface& Ui()
{
    return Settings::Get().Interface;
}

bool Blade::IsWhiteLabel()
{
    const Settings& settings = Settings::Get();
    return settings.WhiteLabel && settings.WhiteLabelTheme.Valid;
}

const char* Blade::WhiteLabelName()
{
    return Settings::Get().WhiteLabelTheme.Name;
}

static void UpdateFontSet()
{
    ImFont* const* fonts = IsWhiteLabel() ? g_white_label_fonts : g_standard_fonts;
    if (!fonts[0])
        return;

    F_Tiny = fonts[0];
    F_Small = fonts[1];
    F_Body = fonts[2];
    F_Med = fonts[3];
    F_Label = fonts[4];
    F_Title = fonts[5];
    F_Head = fonts[6];
    F_Logo = fonts[7];
}

static ImU32 ColorU32(const ImColor& color, float alpha = 1.0f)
{
    ImVec4 value = color.Value;
    value.w = alpha;
    return ImGui::GetColorU32(value);
}

void Blade::LoadFonts(ImGuiIO& io, const void* regular, int reg_size,
               const void* medium, int med_size,
               const void* bold, int bold_size)
{
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();

    const float OS = 2.0f;

    F_Tiny  = io.Fonts->AddFontFromMemoryTTF((void*)regular, reg_size, 11.0f * OS, &cfg, ranges);
    F_Small = io.Fonts->AddFontFromMemoryTTF((void*)regular, reg_size, 12.0f * OS, &cfg, ranges);
    F_Body  = io.Fonts->AddFontFromMemoryTTF((void*)medium,  med_size, 13.0f * OS, &cfg, ranges);
    F_Med   = io.Fonts->AddFontFromMemoryTTF((void*)medium,  med_size, 14.0f * OS, &cfg, ranges);
    F_Label = io.Fonts->AddFontFromMemoryTTF((void*)medium,  med_size, 15.0f * OS, &cfg, ranges);
    F_Title = io.Fonts->AddFontFromMemoryTTF((void*)bold,    bold_size, 15.0f * OS, &cfg, ranges);
    F_Head  = io.Fonts->AddFontFromMemoryTTF((void*)bold,    bold_size, 18.0f * OS, &cfg, ranges);
    F_Logo  = io.Fonts->AddFontFromMemoryTTF((void*)bold,    bold_size, 22.0f * OS, &cfg, ranges);

    g_standard_fonts[0] = F_Tiny;
    g_standard_fonts[1] = F_Small;
    g_standard_fonts[2] = F_Body;
    g_standard_fonts[3] = F_Med;
    g_standard_fonts[4] = F_Label;
    g_standard_fonts[5] = F_Title;
    g_standard_fonts[6] = F_Head;
    g_standard_fonts[7] = F_Logo;

    g_white_label_fonts[0] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 11.0f * OS, &cfg, ranges);
    g_white_label_fonts[1] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 12.0f * OS, &cfg, ranges);
    g_white_label_fonts[2] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 13.0f * OS, &cfg, ranges);
    g_white_label_fonts[3] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 14.0f * OS, &cfg, ranges);
    g_white_label_fonts[4] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 15.0f * OS, &cfg, ranges);
    g_white_label_fonts[5] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 16.0f * OS, &cfg, ranges);
    g_white_label_fonts[6] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 19.0f * OS, &cfg, ranges);
    g_white_label_fonts[7] = io.Fonts->AddFontFromMemoryTTF(Louis_George_Cafe_Bold_ttf, sizeof(Louis_George_Cafe_Bold_ttf), 22.0f * OS, &cfg, ranges);
    UpdateFontSet();
}

static inline float FontPx(ImFont* f) { return f->FontSize * 0.5f; }

ImTextureID Blade::TexLogoWide = nullptr;
ImVec2      Blade::TexLogoWideSz(0, 0);
ImTextureID Blade::TexLogoStack = nullptr;
ImVec2      Blade::TexLogoStackSz(0, 0);

ImTextureID Blade::TexWhiteLabelLogo = nullptr;
ImVec2      Blade::TexWhiteLabelLogoSz(0, 0);
ImVec2      Blade::TexWhiteLabelLogoUV0(0, 0);
ImVec2      Blade::TexWhiteLabelLogoUV1(1, 1);

void Blade::SetLogos(ImTextureID wide, ImVec2 wide_sz, ImTextureID stack, ImVec2 stack_sz)
{
    TexLogoWide = wide;   TexLogoWideSz = wide_sz;
    TexLogoStack = stack; TexLogoStackSz = stack_sz;
}

void Blade::SetWhiteLabelLogo(ImTextureID texture, ImVec2 size, ImVec2 uv0, ImVec2 uv1)
{
    TexWhiteLabelLogo = texture;
    TexWhiteLabelLogoSz = size;
    TexWhiteLabelLogoUV0 = uv0;
    TexWhiteLabelLogoUV1 = uv1;
}

ImTextureID Blade::TexChar = nullptr;
ImVec2      Blade::TexCharSz(0, 0);
ImVec2      Blade::TexCharUV0(0, 0), Blade::TexCharUV1(1, 1);

void Blade::SetCharacter(ImTextureID tex, ImVec2 size, ImVec2 uv0, ImVec2 uv1)
{
    TexChar = tex; TexCharSz = size; TexCharUV0 = uv0; TexCharUV1 = uv1;
}

static ImTextureID g_icon_tex[IC_COUNT] = {};
static ImVec2      g_icon_sz [IC_COUNT] = {};
static bool        g_icon_colored[IC_COUNT] = {};
static ImTextureID g_tab_tex[16] = {};
static ImVec2      g_tab_sz [16] = {};
static ImTextureID g_profile_tex = nullptr;
static ImVec2      g_profile_sz(0, 0);

void Blade::SetIconTex(int id, ImTextureID tex, ImVec2 sz, bool colored)
{
    if (id > 0 && id < IC_COUNT) { g_icon_tex[id] = tex; g_icon_sz[id] = sz; g_icon_colored[id] = colored; }
}
void Blade::SetTabTex(int tab, ImTextureID tex, ImVec2 sz)
{
    if (tab >= 0 && tab < 16) { g_tab_tex[tab] = tex; g_tab_sz[tab] = sz; }
}
void Blade::SetProfileTex(ImTextureID tex, ImVec2 sz) { g_profile_tex = tex; g_profile_sz = sz; }

static void BlitIcon(ImDrawList* dl, ImTextureID tex, ImVec2 texsz, ImVec2 c, float size, ImU32 col, bool colored)
{
    if (!tex || texsz.y <= 0.0f) return;
    float ar = texsz.x / texsz.y, w = size, hh = size;
    if (ar >= 1.0f) hh = size / ar; else w = size * ar;
    ImVec2 mn(c.x - w * 0.5f, c.y - hh * 0.5f), mx(c.x + w * 0.5f, c.y + hh * 0.5f);
    ImU32 tc = colored ? ((col & IM_COL32_A_MASK) | 0x00FFFFFF) : col;
    dl->AddImage(tex, mn, mx, ImVec2(0, 0), ImVec2(1, 1), tc);
}

void Blade::DrawTabIcon(ImDrawList* dl, int tab, Icon fallback, ImVec2 c, float size, ImU32 col)
{
    if (tab >= 0 && tab < 16 && g_tab_tex[tab]) BlitIcon(dl, g_tab_tex[tab], g_tab_sz[tab], c, size, A(col), false);
    else DrawIcon(dl, fallback, c, size, col);
}

void Blade::DrawAvatar(ImDrawList* dl, ImVec2 center, float radius)
{
    if (g_profile_tex && g_profile_sz.y > 0.0f)
    {
        ImVec2 mn(center.x - radius, center.y - radius), mx(center.x + radius, center.y + radius);
        dl->AddImageRounded(g_profile_tex, mn, mx, ImVec2(0, 0), ImVec2(1, 1),
                            A(IM_COL32(255, 255, 255, 255)), radius, ImDrawCornerFlags_All);
    }
    else
    {
        dl->AddCircleFilled(center, radius, A(IM_COL32(255, 255, 255, 255)), 32);
        DrawIcon(dl, IC_OWL, center, radius * 2.2f, A(IM_COL32(20, 20, 24, 255)), 1.5f);
    }
}

static float  g_scale = 1.0f;
static ImVec2 g_saved_mouse(0, 0);
static ImVec2 g_saved_delta(0, 0);

float Blade::Scale() { return g_scale; }

ImVec2 Blade::Screen()
{
    ImVec2 d = ImGui::GetIO().DisplaySize;
    return ImVec2(d.x / g_scale, d.y / g_scale);
}

void Blade::BeginScale()
{

    Settings::sInterface& ui = Ui();
    if (ui.AutoScale && !ImGui::IsMouseDown(0))
    {
        float h = ImGui::GetIO().DisplaySize.y;
        if (h > 1.0f) ui.UiScale = ImClamp(h / 1080.0f, 0.75f, 2.0f);
    }

    static float committed = -1.0f;
    if (committed < 0.0f || !ImGui::IsMouseDown(0))
        committed = ImClamp(ui.UiScale, 0.6f, 2.5f);

    g_scale = committed;

    ImGuiIO& io = ImGui::GetIO();
    g_saved_mouse = io.MousePos;
    g_saved_delta = io.MouseDelta;

    if (g_scale != 1.0f)
    {

        io.MousePos   = ImVec2(io.MousePos.x / g_scale, io.MousePos.y / g_scale);
        io.MouseDelta = ImVec2(io.MouseDelta.x / g_scale, io.MouseDelta.y / g_scale);
    }
}

void Blade::ScaleDrawList(ImDrawList* dl, int vtx_start, int cmd_start)
{
    if (g_scale == 1.0f || !dl) return;

    for (int i = vtx_start; i < dl->VtxBuffer.Size; i++)
    {
        dl->VtxBuffer[i].pos.x *= g_scale;
        dl->VtxBuffer[i].pos.y *= g_scale;
    }
    for (int i = cmd_start; i < dl->CmdBuffer.Size; i++)
    {
        ImVec4& c = dl->CmdBuffer[i].ClipRect;
        c.x *= g_scale; c.y *= g_scale; c.z *= g_scale; c.w *= g_scale;
    }
}

void Blade::EndScale()
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos   = g_saved_mouse;
    io.MouseDelta = g_saved_delta;
    g_scale = 1.0f;
}

ImU32 Blade::Accent(float a)
{
    return ColorU32(Ui().AccentColor, a);
}

ImU32 Blade::GradA(float a) { return ColorU32(Ui().GradientStart, a); }
ImU32 Blade::GradB(float a) { return ColorU32(Ui().GradientEnd, a); }

void Blade::UpdatePalette()
{
    Settings::Get().EnsureInterfaceDefaults();
    UpdateFontSet();
    C.alpha = 1.0f;

    Settings::sInterface& ui = Ui();
    ImVec4 pn = ui.PanelColor.Value;
    pn.w = 1.0f;

    auto Surf = [&](float d, float a) {
        return ImGui::GetColorU32(ImVec4(ImClamp(pn.x + d, 0.0f, 1.0f),
                                         ImClamp(pn.y + d, 0.0f, 1.0f),
                                         ImClamp(pn.z + d * 1.18f, 0.0f, 1.0f), a));
    };

    ImVec4 text_value = ui.TextColor.Value;
    C.divider     = ImGui::GetColorU32(ImVec4(text_value.x, text_value.y, text_value.z, 0.10f));
    C.panel       = Surf(0.000f, 1.0f);
    C.panel_solid = Surf(0.000f, 1.0f);
    C.sidebar     = Surf(-0.018f, 1.0f);
    C.separator   = Surf(0.035f, 1.0f);
    C.row         = Surf(0.047f, 1.0f);
    C.sel         = Surf(0.062f, 1.0f);
    C.border      = Surf(0.070f, 1.0f);
    C.row_hover   = Surf(0.078f, 1.0f);
    C.pill        = Surf(0.078f, 1.0f);
    C.pill_hover  = Surf(0.110f, 1.0f);
    C.track       = Surf(0.125f, 1.0f);
    C.scroll      = Surf(0.240f, 1.0f);

    ImVec4 tx(text_value.x, text_value.y, text_value.z, 1.0f);
    auto TxMix = [&](float t) {
        return ImGui::GetColorU32(ImVec4(ImLerp(tx.x, pn.x, t),
                                         ImLerp(tx.y, pn.y, t),
                                         ImLerp(tx.z, pn.z, t), 1.0f));
    };
    C.text      = ImGui::GetColorU32(tx);
    C.text_dim  = TxMix(0.42f);
    C.text_mute = TxMix(0.62f);

    C.accent      = Accent(1.0f);
    C.accent_soft = Accent(0.28f);
}

ImU32 Blade::Fade(ImU32 col, float a)
{
    a = ImClamp(a, 0.0f, 1.0f);
    ImU32 alpha = (ImU32)(((col >> IM_COL32_A_SHIFT) & 0xFF) * a);
    return (col & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

static float g_alpha = 1.0f;

void  Blade::PushAlpha(float a) { g_alpha = ImClamp(a, 0.0f, 1.0f); }
void  Blade::PopAlpha()         { g_alpha = 1.0f; }
ImU32 Blade::A(ImU32 col)       { return g_alpha >= 1.0f ? col : Fade(col, g_alpha); }

static float g_intro_start = -100.0f;

void Blade::TriggerIntro()
{
    g_intro_start = (float)ImGui::GetTime();
}

float Blade::IntroT(float delay, float duration)
{
    Settings::sInterface& ui = Ui();
    if (IsWhiteLabel() || !ui.IntroAnimation) return 1.0f;

    float speed = ImMax(ui.AnimationSpeed, 0.05f) * ImMax(ui.IntroCascade, 0.05f);
    float el = ((float)ImGui::GetTime() - g_intro_start) * speed - delay;
    if (el <= 0.0f) return 0.0f;

    float t = ImClamp(el / ImMax(duration, 0.01f), 0.0f, 1.0f);
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static ImDrawList* g_dl_override = nullptr;

ImDrawList* Blade::CurDL() { return g_dl_override ? g_dl_override : ImGui::GetWindowDrawList(); }
void Blade::PushDrawList(ImDrawList* dl) { g_dl_override = dl; }
void Blade::PopDrawList() { g_dl_override = nullptr; }

ImVec2 Blade::Measure(ImFont* f, const char* txt)
{
    if (!f) f = ImGui::GetFont();
    return f->CalcTextSizeA(FontPx(f), FLT_MAX, 0.0f, txt);
}

const char* Blade::FitEllipsis(ImFont* f, const char* text, float max_w, char* buf, int bufsz)
{
    static const char empty_text[1]{};
    if (!text) return empty_text;
    if (max_w <= 1.0f || Measure(f, text).x <= max_w) return text;
    for (int n = (int)strlen(text); n > 0; n--)
    {
        snprintf(buf, bufsz, pstra("%.*s..."), n, text);
        if (Measure(f, buf).x <= max_w) return buf;
    }
    snprintf(buf, bufsz, pstra("..."));
    return buf;
}

static bool        g_tip_on = false;
static char        g_tip_title[96]{};
static std::string g_tip_desc;
static ImVec2      g_tip_mouse(0, 0);

void Blade::SetTooltip(const char* title, const char* desc)
{
    g_tip_on = true;
    snprintf(g_tip_title, sizeof(g_tip_title), pstra("%s"), title ? title : pstra(""));
    if (desc)
        g_tip_desc = desc;
    else
        g_tip_desc.clear();
    g_tip_mouse = ImGui::GetIO().MousePos;
}

void Blade::RenderTooltip(ImDrawList* fg)
{
    if (!g_tip_on) return;
    g_tip_on = false;

    const float pad = 9.0f, maxw = 250.0f, lh = 15.0f;
    ImVec2 tsz = Measure(F_Label, g_tip_title);
    float  longest = tsz.x;

    char lines[6][96]; int nlines = 0;
    if (!g_tip_desc.empty())
    {
        char cur[96]{}; const char* p = g_tip_desc.c_str();
        while (*p && nlines < 6)
        {
            char word[64]; int wi = 0;
            while (*p == ' ') p++;
            while (*p && *p != ' ' && wi < 63) word[wi++] = *p++;
            word[wi] = 0;
            if (wi == 0) break;
            char test[96];
            if (cur[0]) snprintf(test, sizeof(test), pstra("%s %s"), cur, word);
            else        snprintf(test, sizeof(test), pstra("%s"), word);
            if (Measure(F_Small, test).x > maxw && cur[0])
            {
                snprintf(lines[nlines], sizeof(lines[0]), pstra("%s"), cur);
                longest = ImMax(longest, Measure(F_Small, cur).x); nlines++;
                snprintf(cur, sizeof(cur), pstra("%s"), word);
            }
            else snprintf(cur, sizeof(cur), pstra("%s"), test);
        }
        if (cur[0] && nlines < 6)
        {
            snprintf(lines[nlines], sizeof(lines[0]), pstra("%s"), cur);
            longest = ImMax(longest, Measure(F_Small, cur).x); nlines++;
        }
    }

    float w = longest + pad * 2.0f;
    float h = pad * 2.0f + tsz.y + (nlines > 0 ? 4.0f + nlines * lh : 0.0f);

    ImVec2 ds = Screen();
    ImVec2 pos(g_tip_mouse.x + 16.0f, g_tip_mouse.y + 20.0f);
    pos.x = ImClamp(pos.x, 6.0f, ImMax(6.0f, ds.x - w - 6.0f));
    pos.y = ImClamp(pos.y, 6.0f, ImMax(6.0f, ds.y - h - 6.0f));
    ImVec2 mx(pos.x + w, pos.y + h);

    fg->AddRectFilled(pos, mx, IM_COL32(24, 24, 30, 250), 8.0f);
    fg->AddRect(pos, mx, IM_COL32(74, 74, 90, 255), 8.0f, ImDrawCornerFlags_All, 1.0f);
    fg->AddText(F_Label, FontPx(F_Label), ImVec2(pos.x + pad, pos.y + pad),
                IM_COL32(236, 236, 244, 255), g_tip_title);
    for (int i = 0; i < nlines; i++)
        fg->AddText(F_Small, FontPx(F_Small),
                    ImVec2(pos.x + pad, pos.y + pad + tsz.y + 4.0f + i * lh),
                    IM_COL32(152, 152, 168, 255), lines[i]);
}

static float GlitchRnd2(int frame, int seed)
{
    float v = sinf((float)frame * 12.9898f + (float)seed * 78.233f) * 43758.5453f;
    return v - floorf(v);
}

void Blade::FadeDrawListAlpha(ImDrawList* dl, int vtx_start, float a)
{
    if (!dl || a >= 1.0f) return;
    ImU32 mul = (ImU32)(ImClamp(a, 0.0f, 1.0f) * 255.0f);
    for (int i = vtx_start; i < dl->VtxBuffer.Size; i++)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        ImU32 ca = (v.col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
        ca = (ca * mul) / 255;
        v.col = (v.col & ~IM_COL32_A_MASK) | (ca << IM_COL32_A_SHIFT);
    }
}

void Blade::GlitchLogoAmbient(ImDrawList* dl, ImTextureID tex, ImVec2 texsz,
                       ImVec2 mn, ImVec2 mx, float alpha)
{
    if (!tex || texsz.y <= 0.0f) return;
    int A = (int)(255.0f * ImClamp(alpha, 0.0f, 1.0f));
    if (A <= 0) return;

    Settings::sInterface& ui = Ui();
    if (!ui.LogoGlitch)
    {
        dl->AddImage(tex, mn, mx, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, A));
        return;
    }

    const float now = (float)ImGui::GetTime();
    float speed  = ImClamp(ui.LogoGlitchSpeed, 0.2f, 4.0f);
    float period = 3.4f / speed;
    float burst  = 0.30f;
    bool  on     = (fmodf(now, period) < burst);

    float lw = mx.x - mn.x, lh = mx.y - mn.y;
    if (!on)
    {
        dl->AddImage(tex, mn, mx, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, A));
        return;
    }

    int frame = (int)(now * (16.0f * speed));
    float dx = (GlitchRnd2(frame, 1) - 0.5f) * (lw * 0.07f);

    dl->PushClipRect(mn, mx, true);
    dl->AddImage(tex, ImVec2(mn.x + dx, mn.y), ImVec2(mx.x + dx, mx.y),
                 ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 60, 60, (int)(A * 0.5f)));
    dl->AddImage(tex, ImVec2(mn.x - dx, mn.y), ImVec2(mx.x - dx, mx.y),
                 ImVec2(0, 0), ImVec2(1, 1), IM_COL32(60, 200, 255, (int)(A * 0.5f)));
    dl->AddImage(tex, mn, mx, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, A));

    for (int i = 0; i < 2; i++)
    {
        if (GlitchRnd2(frame, 40 + i) < 0.5f) continue;
        float band = GlitchRnd2(frame, 10 + i);
        float bh   = 0.06f + 0.10f * GlitchRnd2(frame, 20 + i);
        float sx   = (GlitchRnd2(frame, 30 + i) - 0.5f) * (lw * 0.14f);
        float y0 = mn.y + band * lh, y1 = ImMin(y0 + bh * lh, mx.y);
        dl->PushClipRect(ImVec2(mn.x - lw * 0.25f, y0), ImVec2(mx.x + lw * 0.25f, y1), true);
        dl->AddImage(tex, ImVec2(mn.x + sx, mn.y), ImVec2(mx.x + sx, mx.y),
                     ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, A));
        dl->PopClipRect();
    }
    dl->PopClipRect();
}

void Blade::TextAt(ImDrawList* dl, ImFont* f, ImVec2 pos, ImU32 col, const char* txt)
{
    if (!f) f = ImGui::GetFont();

    if (Ui().GradientText && (col & ~IM_COL32_A_MASK) == (C.text & ~IM_COL32_A_MASK))
    {
        int v0 = dl->VtxBuffer.Size;
        dl->AddText(f, FontPx(f), pos, A(col), txt);
        float tw = f->CalcTextSizeA(FontPx(f), FLT_MAX, 0.0f, txt).x;
        if (tw > 0.5f)
        {
            ImVec4 fa = ImGui::ColorConvertU32ToFloat4(A(GradA(1.0f)));
            ImVec4 fb = ImGui::ColorConvertU32ToFloat4(A(GradB(1.0f)));
            for (int i = v0; i < dl->VtxBuffer.Size; i++)
            {
                ImDrawVert& v = dl->VtxBuffer[i];
                float t = ImClamp((v.pos.x - pos.x) / tw, 0.0f, 1.0f);
                ImU32 tgt = ImGui::GetColorU32(ImLerp(fa, fb, t));
                v.col = (tgt & ~IM_COL32_A_MASK) | (v.col & IM_COL32_A_MASK);
            }
        }
        return;
    }
    dl->AddText(f, FontPx(f), pos, A(col), txt);
}

void Blade::TextRight(ImDrawList* dl, ImFont* f, ImVec2 right_pos, ImU32 col, const char* txt)
{
    ImVec2 sz = Measure(f, txt);
    TextAt(dl, f, ImVec2(right_pos.x - sz.x, right_pos.y), col, txt);
}

void Blade::TextCentered(ImDrawList* dl, ImFont* f, ImVec2 center, ImU32 col, const char* txt)
{
    ImVec2 sz = Measure(f, txt);
    TextAt(dl, f, ImVec2(center.x - sz.x * 0.5f, center.y - sz.y * 0.5f), col, txt);
}

float Blade::Anim(ImGuiID id, bool target, float speed)
{
    if (IsWhiteLabel())
        return target ? 1.0f : 0.0f;

    auto it = g_blade_anim_values.find(id);
    if (it == g_blade_anim_values.end())
        it = g_blade_anim_values.insert({ id, target ? 1.0f : 0.0f }).first;

    float dt = ImGui::GetIO().DeltaTime * speed * ImMax(Ui().AnimationSpeed, 0.05f);
    it->second = ImClamp(it->second + dt * (target ? 1.0f : -1.0f), 0.0f, 1.0f);
    return it->second;
}

static ImDrawList* g_shadow_dl = nullptr;

void Blade::BeginShadowLayer(ImDrawList* dl)
{
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);
    g_shadow_dl = dl;
}

void Blade::EndShadowLayer(ImDrawList* dl)
{
    g_shadow_dl = nullptr;
    dl->ChannelsMerge();
}

void Blade::Shadow(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding, float spread)
{

    const bool layered = (dl == g_shadow_dl);
    if (layered) dl->ChannelsSetCurrent(0);

    const int   steps   = 6;
    const int   max_seg = 4;
    const float step    = spread / (float)steps;
    const float th      = step * 2.0f;
    const float drop    = spread * 0.14f;

    for (int i = steps; i >= 1; i--)
    {
        float t = (float)i / (float)steps;
        float grow = spread * t;

        float f = 1.0f - t;
        int a = (int)(22.0f * f * f + 1.5f);
        if (a <= 0) continue;

        RectStroke(dl, ImVec2(min.x - grow, min.y - grow + drop),
                   ImVec2(max.x + grow, max.y + grow + drop),
                   IM_COL32(0, 0, 0, a), rounding + grow,
                   ImDrawCornerFlags_All, th, max_seg);
    }

    if (layered) dl->ChannelsSetCurrent(1);
}

static void PathRoundedRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, int corners,
                            int max_seg = 32)
{
    if (IsWhiteLabel())
        rounding = ImMin(rounding, 2.0f);

    rounding = ImMin(rounding, ImFabs(b.x - a.x) * 0.5f - 1.0f);
    rounding = ImMin(rounding, ImFabs(b.y - a.y) * 0.5f - 1.0f);

    if (rounding <= 0.0f || corners == 0)
    {
        dl->PathLineTo(a);
        dl->PathLineTo(ImVec2(b.x, a.y));
        dl->PathLineTo(b);
        dl->PathLineTo(ImVec2(a.x, b.y));
        return;
    }

    const float rtl = (corners & ImDrawCornerFlags_TopLeft)  ? rounding : 0.0f;
    const float rtr = (corners & ImDrawCornerFlags_TopRight) ? rounding : 0.0f;
    const float rbr = (corners & ImDrawCornerFlags_BotRight) ? rounding : 0.0f;
    const float rbl = (corners & ImDrawCornerFlags_BotLeft)  ? rounding : 0.0f;

    auto Seg = [max_seg](float r) {
        return ImClamp((int)(r * Scale() * 0.35f) + 2, 2, max_seg);
    };

    dl->PathArcTo(ImVec2(a.x + rtl, a.y + rtl), rtl, IM_PI,        IM_PI * 1.5f, Seg(rtl));
    dl->PathArcTo(ImVec2(b.x - rtr, a.y + rtr), rtr, IM_PI * 1.5f, IM_PI * 2.0f, Seg(rtr));
    dl->PathArcTo(ImVec2(b.x - rbr, b.y - rbr), rbr, 0.0f,         IM_PI * 0.5f, Seg(rbr));
    dl->PathArcTo(ImVec2(a.x + rbl, b.y - rbl), rbl, IM_PI * 0.5f, IM_PI,        Seg(rbl));
}

void Blade::RectFilled(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float rounding,
                int corners, int max_seg)
{
    col = A(col);
    if ((col & IM_COL32_A_MASK) == 0) return;
    PathRoundedRect(dl, min, max, rounding, corners, max_seg);
    dl->PathFillConvex(col);
}

void Blade::RectStroke(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float rounding,
                int corners, float thickness, int max_seg)
{
    col = A(col);
    if ((col & IM_COL32_A_MASK) == 0) return;
    PathRoundedRect(dl, min, max, rounding, corners, max_seg);
    dl->PathStroke(col, true, thickness);
}

void Blade::FadeTopRect(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float rounding)
{
    const float h = max.y - min.y;
    if (h <= 0.0f) return;

    const int v0 = dl->VtxBuffer.Size;
    RectFilled(dl, min, max, col, rounding, ImDrawCornerFlags_Top);

    for (int i = v0; i < dl->VtxBuffer.Size; i++)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        float t = ImClamp((v.pos.y - min.y) / h, 0.0f, 1.0f);
        ImU32 a = (ImU32)(((v.col >> IM_COL32_A_SHIFT) & 0xFF) * (1.0f - t));
        v.col = (v.col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
    }
}

void Blade::GradientBarV(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding,
                  const ImU32* stops, int stop_count)
{
    if (stop_count < 2) return;

    const int N = 64;
    for (int i = 0; i < N; i++)
    {
        float t0 = (float)i / N, t1 = (float)(i + 1) / N;
        float y0 = ImLerp(min.y, max.y, t0);
        float y1 = ImLerp(min.y, max.y, t1) + (i < N - 1 ? 0.6f : 0.0f);

        float f = t0 * (stop_count - 1);
        int   k = ImClamp((int)f, 0, stop_count - 2);
        ImU32 c = ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(stops[k]),
                                            ImGui::ColorConvertU32ToFloat4(stops[k + 1]),
                                            f - (float)k));

        int flags = 0;
        if (i == 0)     flags |= ImDrawCornerFlags_Top;
        if (i == N - 1) flags |= ImDrawCornerFlags_Bot;

        RectFilled(dl, ImVec2(min.x, y0), ImVec2(max.x, y1), c,
                          flags ? rounding : 0.0f, flags);
    }
}

void Blade::GradientBarH(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding,
                  ImU32 ca, ImU32 cb, int corners)
{
    const float w = max.x - min.x;
    if (w <= 0.0f) return;

    const int v0 = dl->VtxBuffer.Size;
    RectFilled(dl, min, max, ca, rounding, corners);

    ImU32 a_ca = A(ca), a_cb = A(cb);
    ImVec4 fa = ImGui::ColorConvertU32ToFloat4(a_ca);
    ImVec4 fb = ImGui::ColorConvertU32ToFloat4(a_cb);
    for (int i = v0; i < dl->VtxBuffer.Size; i++)
    {
        ImDrawVert& vv = dl->VtxBuffer[i];
        float t = ImClamp((vv.pos.x - min.x) / w, 0.0f, 1.0f);
        ImU32 tgt = ImGui::GetColorU32(ImLerp(fa, fb, t));
        vv.col = (tgt & ~IM_COL32_A_MASK) | (vv.col & IM_COL32_A_MASK);
    }
}

void Blade::Panel(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding)
{
    Settings::sInterface& ui = Ui();
    float r = rounding * ImClamp(ui.CornerRounding, 0.0f, 1.8f);

    if (ui.Shadows)
        Shadow(dl, min, max, r, 12.0f);

    RectFilled(dl, min, max, C.panel, r);

    if (ui.Glass)
        FadeTopRect(dl, min, ImVec2(max.x, min.y + 28.0f), IM_COL32(255, 255, 255, 8), r);

    RectStroke(dl, min, max, C.border, r, ImDrawCornerFlags_All, 1.0f);
}

static void PolyStroke(ImDrawList* dl, const ImVec2* pts, int n, ImU32 col, float th, bool closed = false)
{
    dl->PathClear();
    for (int i = 0; i < n; i++) dl->PathLineTo(pts[i]);
    dl->PathStroke(col, closed, th);
}

static void PolyFill(ImDrawList* dl, const ImVec2* pts, int n, ImU32 col)
{
    dl->AddConvexPolyFilled(pts, n, col);
}

void Blade::DrawIcon(ImDrawList* dl, Icon id, ImVec2 c, float size, ImU32 col, float th)
{
    col = A(col);
    if (id > 0 && id < IC_COUNT && g_icon_tex[id])
    {
        BlitIcon(dl, g_icon_tex[id], g_icon_sz[id], c, size, col, g_icon_colored[id]);
        return;
    }
    const float h = size * 0.5f;
    const float u = size / 16.0f;
    auto P = [&](float x, float y) { return ImVec2(c.x + (x - 8.0f) * u, c.y + (y - 8.0f) * u); };

    switch (id)
    {
    case IC_BOLT:
    {
        ImVec2 p[6] = { P(9.6f,1.5f), P(4.2f,8.9f), P(7.6f,8.9f), P(6.4f,14.5f), P(11.8f,7.1f), P(8.4f,7.1f) };
        PolyFill(dl, p, 6, col);
        break;
    }
    case IC_USER:
        dl->AddCircle(P(8, 5.6f), 2.6f * u, col, 20, th);
        dl->PathClear();
        dl->PathArcTo(P(8, 14.2f), 4.4f * u, IM_PI + 0.25f, 2.0f * IM_PI - 0.25f, 20);
        dl->PathStroke(col, false, th);
        break;
    case IC_WIFI:
        for (int i = 0; i < 3; i++)
        {
            float r = (2.2f + i * 2.4f) * u;
            dl->PathClear();
            dl->PathArcTo(P(8, 12.6f), r, IM_PI + 0.62f, 2.0f * IM_PI - 0.62f, 24);
            dl->PathStroke(col, false, th);
        }
        dl->AddCircleFilled(P(8, 12.4f), 1.15f * u, col, 10);
        break;
    case IC_GAUGE:
        dl->PathClear();
        dl->PathArcTo(P(8, 10.0f), 5.6f * u, IM_PI, 2.0f * IM_PI, 28);
        dl->PathStroke(col, false, th);
        dl->AddLine(P(8, 10.0f), P(11.1f, 6.4f), col, th);
        dl->AddCircleFilled(P(8, 10.0f), 1.1f * u, col, 10);
        break;
    case IC_MAPPIN:
        dl->PathClear();
        dl->PathArcTo(P(8, 6.6f), 4.3f * u, IM_PI * 0.82f, IM_PI * 2.18f, 26);
        dl->PathLineTo(P(8, 14.4f));
        dl->PathStroke(col, true, th);
        dl->AddCircle(P(8, 6.5f), 1.7f * u, col, 14, th);
        break;
    case IC_CLOSE:
        dl->AddLine(P(4.6f, 4.6f), P(11.4f, 11.4f), col, th);
        dl->AddLine(P(11.4f, 4.6f), P(4.6f, 11.4f), col, th);
        break;
    case IC_DRAG:
        for (int gy = 0; gy < 3; gy++)
            for (int gx = 0; gx < 2; gx++)
                dl->AddCircleFilled(P(6.0f + gx * 4.0f, 4.0f + gy * 4.0f), 1.05f * u, col, 8);
        break;
    case IC_PIN:
    {

        ImVec2 head[4] = { P(6.0f,2.6f), P(13.4f,6.4f), P(11.6f,9.0f), P(4.2f,5.2f) };
        PolyStroke(dl, head, 4, col, th, true);
        dl->AddLine(P(6.8f, 8.5f), P(3.0f, 13.6f), col, th);
        break;
    }
    case IC_DOTS:
        for (int i = -1; i <= 1; i++)
            dl->AddCircleFilled(ImVec2(c.x + i * 3.4f * u, c.y), 1.15f * u, col, 8);
        break;
    case IC_CHEVRON:
    {
        ImVec2 p[3] = { P(4.6f, 6.4f), P(8.0f, 10.0f), P(11.4f, 6.4f) };
        PolyStroke(dl, p, 3, col, th);
        break;
    }
    case IC_ARROWS_LR:
        dl->AddLine(P(3.6f, 8.0f), P(12.4f, 8.0f), col, th);
        { ImVec2 a[3] = { P(5.8f,5.6f), P(3.2f,8.0f), P(5.8f,10.4f) }; PolyStroke(dl, a, 3, col, th); }
        { ImVec2 b[3] = { P(10.2f,5.6f), P(12.8f,8.0f), P(10.2f,10.4f) }; PolyStroke(dl, b, 3, col, th); }
        break;
    case IC_SEARCH:
        dl->AddCircle(P(7.0f, 7.0f), 4.1f * u, col, 22, th);
        dl->AddLine(P(10.1f, 10.1f), P(13.4f, 13.4f), col, th);
        break;
    case IC_FOLDER:
    {
        ImVec2 p[6] = { P(2.4f,12.6f), P(2.4f,4.2f), P(6.4f,4.2f), P(7.8f,6.0f), P(13.6f,6.0f), P(13.6f,12.6f) };
        PolyStroke(dl, p, 6, col, th, true);
        break;
    }
    case IC_SWORD:
        dl->AddLine(P(13.2f, 2.8f), P(6.4f, 9.6f), col, th * 1.6f);
        dl->AddLine(P(4.4f, 9.2f), P(7.0f, 11.8f), col, th);
        dl->AddLine(P(3.0f, 13.2f), P(5.6f, 10.6f), col, th);
        dl->AddLine(P(10.8f, 2.6f), P(13.4f, 5.2f), col, th);
        break;
    case IC_RUN:
        dl->AddCircleFilled(P(9.6f, 3.6f), 1.5f * u, col, 12);
        { ImVec2 p[4] = { P(11.6f,7.4f), P(8.2f,6.2f), P(6.0f,8.6f), P(6.6f,11.4f) }; PolyStroke(dl, p, 4, col, th); }
        dl->AddLine(P(8.2f, 9.0f), P(10.4f, 12.8f), col, th);
        dl->AddLine(P(6.6f, 11.4f), P(4.2f, 13.4f), col, th);
        break;
    case IC_EYE:
        dl->PathClear();
        dl->PathArcTo(P(8, 12.4f), 6.2f * u, IM_PI * 1.22f, IM_PI * 1.78f, 24);
        dl->PathArcTo(P(8, 3.6f), 6.2f * u, IM_PI * 0.22f, IM_PI * 0.78f, 24);
        dl->PathStroke(col, true, th);
        dl->AddCircleFilled(P(8, 8), 1.7f * u, col, 14);
        break;
    case IC_CUBE:
    {
        ImVec2 top[4] = { P(8,2.6f), P(13.6f,5.6f), P(8,8.6f), P(2.4f,5.6f) };
        PolyStroke(dl, top, 4, col, th, true);
        dl->AddLine(P(2.4f, 5.6f), P(2.4f, 10.6f), col, th);
        dl->AddLine(P(13.6f, 5.6f), P(13.6f, 10.6f), col, th);
        dl->AddLine(P(8, 8.6f), P(8, 13.6f), col, th);
        dl->AddLine(P(2.4f, 10.6f), P(8, 13.6f), col, th);
        dl->AddLine(P(13.6f, 10.6f), P(8, 13.6f), col, th);
        break;
    }
    case IC_SLIDERS:
        for (int i = 0; i < 3; i++)
        {
            float y = 4.0f + i * 4.0f;
            dl->AddLine(P(2.8f, y), P(13.2f, y), col, th);
            dl->AddCircleFilled(P(i == 1 ? 10.4f : 5.8f, y), 1.6f * u, col, 12);
        }
        break;
    case IC_PALETTE:
        dl->PathClear();
        dl->PathArcTo(P(8, 8), 5.8f * u, 0.0f, IM_PI * 1.72f, 32);
        dl->PathLineTo(P(9.4f, 10.6f));
        dl->PathStroke(col, true, th);
        dl->AddCircleFilled(P(5.4f, 6.0f), 1.1f * u, col, 8);
        dl->AddCircleFilled(P(9.0f, 4.8f), 1.1f * u, col, 8);
        dl->AddCircleFilled(P(4.6f, 9.6f), 1.1f * u, col, 8);
        break;
    case IC_CART:
        dl->AddLine(P(2.2f, 3.2f), P(4.2f, 3.2f), col, th);
        { ImVec2 p[4] = { P(4.2f,3.2f), P(5.6f,10.2f), P(12.6f,10.2f), P(13.8f,5.4f) }; PolyStroke(dl, p, 4, col, th); }
        dl->AddLine(P(4.9f, 5.4f), P(13.8f, 5.4f), col, th);
        dl->AddCircleFilled(P(6.6f, 12.8f), 1.3f * u, col, 10);
        dl->AddCircleFilled(P(11.6f, 12.8f), 1.3f * u, col, 10);
        break;
    case IC_FLAME:
        dl->PathClear();
        dl->PathLineTo(P(8.0f, 1.8f));
        dl->PathBezierCurveTo(P(12.4f, 5.4f), P(13.2f, 9.4f), P(10.6f, 12.4f), 16);
        dl->PathBezierCurveTo(P(8.6f, 14.6f), P(5.0f, 14.2f), P(4.0f, 11.2f), 16);
        dl->PathBezierCurveTo(P(3.2f, 8.6f), P(5.4f, 6.6f), P(8.0f, 1.8f), 16);
        dl->PathStroke(col, true, th);
        dl->AddLine(P(8.0f, 8.2f), P(6.4f, 11.2f), col, th * 0.85f);
        break;
    case IC_POTION:
        dl->AddCircle(P(5.4f, 5.0f), 2.8f * u, col, 18, th);
        dl->AddCircle(P(10.8f, 10.6f), 3.6f * u, col, 20, th);
        dl->AddLine(P(4.0f, 11.4f), P(7.0f, 8.4f), col, th);
        dl->AddLine(P(4.0f, 8.4f), P(7.0f, 11.4f), col, th);
        break;
    case IC_SPEED:
    {
        ImVec2 a[4] = { P(7.6f,1.8f), P(3.4f,8.4f), P(6.2f,8.4f), P(5.4f,14.2f) };
        PolyStroke(dl, a, 4, col, th);
        dl->AddLine(P(5.4f, 14.2f), P(9.8f, 7.4f), col, th);
        dl->AddLine(P(9.8f, 7.4f), P(7.0f, 7.4f), col, th);
        dl->AddLine(P(7.0f, 7.4f), P(7.6f, 1.8f), col, th);
        dl->AddLine(P(11.4f, 4.4f), P(14.0f, 4.4f), col, th * 0.8f);
        dl->AddLine(P(11.0f, 8.0f), P(14.4f, 8.0f), col, th * 0.8f);
        break;
    }
    case IC_FIST:
    {
        ImVec2 p[6] = { P(3.8f,7.4f), P(3.8f,11.6f), P(6.4f,13.6f), P(11.2f,13.6f), P(12.6f,10.4f), P(12.6f,6.2f) };
        PolyStroke(dl, p, 6, col, th, true);
        for (int i = 0; i < 3; i++)
            dl->AddLine(P(6.0f + i * 2.2f, 6.2f), P(6.0f + i * 2.2f, 4.0f), col, th * 0.9f);
        break;
    }
    case IC_PREV:
        dl->AddLine(P(4.6f, 4.0f), P(4.6f, 12.0f), col, th * 1.15f);
        { ImVec2 p[3] = { P(12.2f,4.0f), P(12.2f,12.0f), P(6.2f,8.0f) }; PolyFill(dl, p, 3, col); }
        break;
    case IC_PAUSE:
        RectFilled(dl, P(5.4f, 4.0f), P(7.2f, 12.0f), col, 0.6f * u);
        RectFilled(dl, P(8.8f, 4.0f), P(10.6f, 12.0f), col, 0.6f * u);
        break;
    case IC_NEXT:
        dl->AddLine(P(11.4f, 4.0f), P(11.4f, 12.0f), col, th * 1.15f);
        { ImVec2 p[3] = { P(3.8f,4.0f), P(3.8f,12.0f), P(9.8f,8.0f) }; PolyFill(dl, p, 3, col); }
        break;
    case IC_SUN:
        dl->AddCircleFilled(P(8, 8), 2.6f * u, col, 16);
        for (int i = 0; i < 8; i++)
        {
            float a = i * IM_PI * 0.25f;
            dl->AddLine(ImVec2(c.x + cosf(a) * 4.2f * u, c.y + sinf(a) * 4.2f * u),
                        ImVec2(c.x + cosf(a) * 6.4f * u, c.y + sinf(a) * 6.4f * u), col, th);
        }
        break;
    case IC_HELMET:
    {
        ImVec2 p[6] = { P(3.0f,12.4f), P(3.0f,7.0f), P(5.4f,3.6f), P(10.6f,3.6f), P(13.0f,7.0f), P(13.0f,12.4f) };
        PolyStroke(dl, p, 6, col, th, true);
        dl->AddLine(P(3.0f, 8.6f), P(13.0f, 8.6f), col, th * 0.85f);
        break;
    }
    case IC_CHESTPLATE:
    {
        ImVec2 p[7] = { P(3.2f,4.0f), P(6.4f,4.0f), P(8.0f,6.0f), P(9.6f,4.0f), P(12.8f,4.0f), P(12.0f,12.6f), P(4.0f,12.6f) };
        PolyStroke(dl, p, 7, col, th, true);
        break;
    }
    case IC_LEGGINGS:
    {
        ImVec2 p[6] = { P(4.0f,3.4f), P(12.0f,3.4f), P(11.2f,13.0f), P(9.2f,13.0f), P(8.0f,7.8f), P(6.8f,13.0f) };
        PolyStroke(dl, p, 6, col, th, true);
        dl->AddLine(P(4.8f, 13.0f), P(6.8f, 13.0f), col, th);
        break;
    }
    case IC_BOOTS:
    {
        ImVec2 p[5] = { P(4.4f,3.2f), P(8.4f,3.2f), P(8.4f,9.0f), P(13.0f,11.4f), P(13.0f,13.2f) };
        PolyStroke(dl, p, 5, col, th);
        dl->AddLine(P(13.0f, 13.2f), P(4.4f, 13.2f), col, th);
        dl->AddLine(P(4.4f, 13.2f), P(4.4f, 3.2f), col, th);
        break;
    }
    case IC_SWORD_SMALL:
        dl->AddLine(P(12.6f, 3.4f), P(6.8f, 9.2f), col, th * 1.3f);
        dl->AddLine(P(4.8f, 9.0f), P(7.2f, 11.4f), col, th);
        dl->AddLine(P(3.4f, 12.8f), P(5.8f, 10.4f), col, th);
        break;
    case IC_OWL:
        dl->AddCircle(P(8, 8), 6.0f * u, col, 26, th);
        dl->AddCircleFilled(P(5.8f, 6.8f), 1.5f * u, col, 12);
        dl->AddCircleFilled(P(10.2f, 6.8f), 1.5f * u, col, 12);
        { ImVec2 p[3] = { P(6.9f,9.4f), P(9.1f,9.4f), P(8.0f,11.4f) }; PolyFill(dl, p, 3, col); }
        dl->AddLine(P(3.4f, 3.6f), P(5.6f, 5.0f), col, th);
        dl->AddLine(P(12.6f, 3.6f), P(10.4f, 5.0f), col, th);
        break;
    case IC_KEYBIND:
        RectStroke(dl, P(2.2f, 4.4f), P(13.8f, 11.6f), col, 1.6f * u, ImDrawCornerFlags_All, th);
        for (int i = 0; i < 4; i++)
            dl->AddLine(P(4.2f + i * 2.4f, 6.8f), P(4.9f + i * 2.4f, 6.8f), col, th);
        dl->AddLine(P(5.4f, 9.4f), P(10.6f, 9.4f), col, th);
        break;
    case IC_GEAR:
    {
        dl->AddCircle(P(8, 8), 2.5f * u, col, 18, th);
        for (int i = 0; i < 8; i++)
        {
            float a = i * IM_PI * 0.25f;
            dl->AddLine(ImVec2(c.x + cosf(a) * 4.0f * u, c.y + sinf(a) * 4.0f * u),
                        ImVec2(c.x + cosf(a) * 6.2f * u, c.y + sinf(a) * 6.2f * u), col, th * 1.35f);
        }
        dl->AddCircle(P(8, 8), 5.1f * u, col, 26, th);
        break;
    }
    case IC_SORT:
        for (int i = 0; i < 3; i++)
            dl->AddLine(P(2.4f, 4.6f + i * 3.2f), P(8.6f - i * 1.8f, 4.6f + i * 3.2f), col, th);
        dl->AddLine(P(12.2f, 3.4f), P(12.2f, 12.6f), col, th);
        { ImVec2 a[3] = { P(10.2f,10.4f), P(12.2f,12.8f), P(14.2f,10.4f) }; PolyStroke(dl, a, 3, col, th); }
        break;
    case IC_FILTER:
        for (int i = 0; i < 3; i++)
        {
            float y = 4.2f + i * 3.8f;
            dl->AddLine(P(2.6f, y), P(13.4f, y), col, th);
            dl->AddCircleFilled(P(i == 1 ? 10.6f : 5.6f, y), 1.55f * u, col, 12);
        }
        break;
    case IC_CHEVRON_R:
    {
        ImVec2 p[3] = { P(6.4f, 4.4f), P(10.0f, 8.0f), P(6.4f, 11.6f) };
        PolyStroke(dl, p, 3, col, th);
        break;
    }
    case IC_CHEVRON_L:
    {
        ImVec2 p[3] = { P(9.6f, 4.4f), P(6.0f, 8.0f), P(9.6f, 11.6f) };
        PolyStroke(dl, p, 3, col, th);
        break;
    }
    case IC_CHECK:
    {
        ImVec2 p[3] = { P(3.6f, 8.2f), P(6.7f, 11.4f), P(12.4f, 4.8f) };
        PolyStroke(dl, p, 3, col, th * 1.35f);
        break;
    }
    case IC_COPY:
        RectStroke(dl, P(5.6f, 2.6f), P(13.4f, 10.4f), col, 1.6f * u, ImDrawCornerFlags_All, th);
        dl->PathClear();
        dl->PathLineTo(P(10.4f, 13.4f)); dl->PathLineTo(P(2.6f, 13.4f)); dl->PathLineTo(P(2.6f, 5.6f));
        dl->PathStroke(col, false, th);
        break;
    case IC_HOTKEY:
        dl->AddCircle(P(8, 8), 5.8f * u, col, 26, th);
        dl->AddLine(P(8, 4.6f), P(8, 11.4f), col, th);
        dl->AddLine(P(4.6f, 8), P(11.4f, 8), col, th);
        break;
    case IC_LANG:
        dl->AddLine(P(2.2f, 12.4f), P(5.6f, 3.4f), col, th);
        dl->AddLine(P(5.6f, 3.4f), P(9.0f, 12.4f), col, th);
        dl->AddLine(P(3.4f, 9.4f), P(7.8f, 9.4f), col, th);
        dl->AddLine(P(10.4f, 13.6f), P(13.8f, 6.4f), col, th * 0.9f);
        dl->AddLine(P(13.8f, 6.4f), P(15.4f, 13.6f), col, th * 0.9f);
        break;
    case IC_DPI:
        { ImVec2 a[3] = { P(3.0f,6.0f), P(5.4f,3.2f), P(7.8f,6.0f) }; PolyStroke(dl, a, 3, col, th); }
        dl->AddLine(P(5.4f, 3.2f), P(5.4f, 12.8f), col, th);
        { ImVec2 b[3] = { P(8.2f,10.0f), P(10.6f,12.8f), P(13.0f,10.0f) }; PolyStroke(dl, b, 3, col, th); }
        dl->AddLine(P(10.6f, 12.8f), P(10.6f, 3.2f), col, th);
        break;
    case IC_STYLE:
        dl->AddCircle(P(5.8f, 6.2f), 3.3f * u, col, 20, th);
        dl->AddCircle(P(10.2f, 6.2f), 3.3f * u, col, 20, th);
        dl->AddCircle(P(8.0f, 10.4f), 3.3f * u, col, 20, th);
        break;
    case IC_WARNING:
    {
        ImVec2 p[3] = { P(8.0f, 2.4f), P(14.4f, 13.4f), P(1.6f, 13.4f) };
        dl->PathClear();
        for (int i = 0; i < 3; i++) dl->PathLineTo(p[i]);
        dl->PathStroke(col, true, th * 1.1f);
        dl->AddLine(P(8.0f, 6.6f), P(8.0f, 10.0f), col, th * 1.2f);
        dl->AddCircleFilled(P(8.0f, 11.8f), 0.8f * u, col, 8);
        break;
    }
    case IC_EYE_OFF:
        dl->PathClear();
        dl->PathArcTo(P(8, 12.4f), 6.2f * u, IM_PI * 1.22f, IM_PI * 1.78f, 24);
        dl->PathArcTo(P(8, 3.6f), 6.2f * u, IM_PI * 0.22f, IM_PI * 0.78f, 24);
        dl->PathStroke(col, true, th);
        dl->AddCircle(P(8, 8), 1.7f * u, col, 14, th);
        dl->AddLine(P(2.6f, 2.6f), P(13.4f, 13.4f), col, th * 1.2f);
        break;
    case IC_TRASH:
        dl->AddLine(P(3.2f, 4.4f), P(12.8f, 4.4f), col, th);
        dl->AddRect(P(4.4f, 4.4f), P(11.6f, 13.4f), col, 1.2f * u, ImDrawCornerFlags_Bot, th);
        dl->AddLine(P(6.2f, 3.0f), P(9.8f, 3.0f), col, th);
        dl->AddLine(P(6.4f, 6.4f), P(6.4f, 11.6f), col, th * 0.85f);
        dl->AddLine(P(9.6f, 6.4f), P(9.6f, 11.6f), col, th * 0.85f);
        break;
    case IC_BOOKMARK:
    {
        ImVec2 p[5] = { P(4.4f,2.6f), P(11.6f,2.6f), P(11.6f,13.4f), P(8.0f,10.2f), P(4.4f,13.4f) };
        PolyStroke(dl, p, 5, col, th, true);
        break;
    }
    case IC_RESET:
        dl->PathClear();
        dl->PathArcTo(P(8, 8), 5.2f * u, IM_PI * 0.35f, IM_PI * 1.9f, 24);
        dl->PathStroke(col, false, th);
        { ImVec2 a[3] = { P(3.2f,4.4f), P(3.0f,8.4f), P(6.8f,7.2f) }; PolyStroke(dl, a, 3, col, th); }
        break;
    case IC_PLUS:
        dl->AddLine(P(8, 3.6f), P(8, 12.4f), col, th * 1.2f);
        dl->AddLine(P(3.6f, 8), P(12.4f, 8), col, th * 1.2f);
        break;
    case IC_FRAME:
    {
        const float e = 2.6f, s = 4.6f;
        dl->AddLine(P(e, e), P(s, e), col, th);          dl->AddLine(P(e, e), P(e, s), col, th);
        dl->AddLine(P(16 - e, e), P(16 - s, e), col, th); dl->AddLine(P(16 - e, e), P(16 - e, s), col, th);
        dl->AddLine(P(e, 16 - e), P(s, 16 - e), col, th); dl->AddLine(P(e, 16 - e), P(e, 16 - s), col, th);
        dl->AddLine(P(16 - e, 16 - e), P(16 - s, 16 - e), col, th);
        dl->AddLine(P(16 - e, 16 - e), P(16 - e, 16 - s), col, th);
        break;
    }
    case IC_SUNDIM:
        dl->AddCircleFilled(P(8, 8), 2.2f * u, col, 14);
        for (int i = 0; i < 8; i++)
        {
            float a = i * IM_PI * 0.25f;
            dl->AddLine(ImVec2(c.x + cosf(a) * 3.6f * u, c.y + sinf(a) * 3.6f * u),
                        ImVec2(c.x + cosf(a) * 5.4f * u, c.y + sinf(a) * 5.4f * u), col, th * 0.9f);
        }
        break;
    case IC_MISC:
    {
        RectFilled(dl, P(2.8f, 2.8f), P(6.8f, 6.8f), col, 1.2f * u);
        RectFilled(dl, P(9.2f, 2.8f), P(13.2f, 6.8f), col, 1.2f * u);
        RectFilled(dl, P(2.8f, 9.2f), P(6.8f, 13.2f), col, 1.2f * u);
        RectFilled(dl, P(9.2f, 9.2f), P(13.2f, 13.2f), col, 1.2f * u);
        break;
    }
    default:
        break;
    }
    (void)h;
}

bool Blade::Hitbox(const char* id_str, ImVec2 min, ImVec2 max, bool* hovered_out)
{
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) { if (hovered_out) *hovered_out = false; return false; }

    const ImGuiID id = w->GetID(id_str);
    ImRect bb(min, max);
    ImGui::ItemAdd(bb, id);

    bool hovered = false, held = false;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (hovered_out) *hovered_out = hovered;
    return pressed;
}

namespace
{
    struct HotkeyCaptureState
    {
        ImGuiID capture = 0;
        ImGuiID suppress_bound_lbutton_release = 0;
        bool wait_for_initial_click_release = false;
    };

    HotkeyCaptureState g_hotkey_capture;

    static bool IsAllowedMouseHotkey(int button, unsigned int allowed_mouse)
    {
        static const unsigned int mouse_flags[] = { HK_MOUSE1, HK_MOUSE2, HK_MOUSE3, HK_MOUSE4, HK_MOUSE5 };
        return button >= 0 && button < IM_ARRAYSIZE(mouse_flags) &&
               (allowed_mouse & mouse_flags[button]) != 0;
    }

    static std::string HotkeyName(int vk)
    {
        char buffer[16]{};
        if (vk == 0) return pstra("None");
        if (vk == VK_LBUTTON) return pstra("Mouse1");
        if (vk == VK_RBUTTON) return pstra("Mouse2");
        if (vk == VK_MBUTTON) return pstra("Mouse3");
        if (vk == VK_XBUTTON1) return pstra("Mouse4");
        if (vk == VK_XBUTTON2) return pstra("Mouse5");
        if (vk == VK_HOME) return pstra("Home");
        if (vk == VK_INSERT) return pstra("Insert");
        if (vk == VK_END) return pstra("End");
        if (vk == VK_DELETE) return pstra("Delete");
        if (vk == VK_SHIFT) return pstra("Shift");
        if (vk == VK_CONTROL) return pstra("Ctrl");
        if (vk == VK_MENU) return pstra("Alt");
        if (vk == VK_SPACE) return pstra("Space");
        if (vk >= 'A' && vk <= 'Z') return std::string(1, static_cast<char>(vk));
        if (vk >= '0' && vk <= '9') return std::string(1, static_cast<char>(vk));
        if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buffer, sizeof(buffer), pstra("F%d"), vk - VK_F1 + 1); return buffer; }

        snprintf(buffer, sizeof(buffer), pstra("0x%02X"), vk);
        return buffer;
    }
}

bool Blade::HotkeyButton(const char* id_str, ImVec2 min, ImVec2 max, int* key, unsigned int allowed_mouse)
{
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID(id_str);
    bool hovered = false;
    bool clicked = Hitbox(id_str, min, max, &hovered);

    if (g_hotkey_capture.suppress_bound_lbutton_release == id &&
        !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        // ButtonBehavior reports Mouse1 on release. Consume the release
        // after binding it so the control is not armed again.
        g_hotkey_capture.suppress_bound_lbutton_release = 0;
        clicked = false;
    }

    bool active = g_hotkey_capture.capture == id;
    if (clicked && !active)
    {
        g_hotkey_capture.capture = id;
        g_hotkey_capture.wait_for_initial_click_release = true;
        active = true;
    }

    if (active)
    {
        if (g_hotkey_capture.wait_for_initial_click_release)
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                g_hotkey_capture.wait_for_initial_click_release = false;
        }
        else
        {
            const ImGuiIO& io = ImGui::GetIO();
            static const int mouse_keys[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
            bool captured = false;

            for (int button = 0; button < IM_ARRAYSIZE(mouse_keys); ++button)
            {
                // Mouse1 must be clicked inside the armed field. Other
                // mouse buttons may be captured without moving the cursor.
                const bool imgui_clicked = io.MouseClicked[button];
                // Some overlay input paths do not forward XBUTTON1/2 to
                // ImGui. Fall back to the native transition for Mouse2-5.
                const bool native_clicked = button != ImGuiMouseButton_Left &&
                    (GetAsyncKeyState(mouse_keys[button]) & 1) != 0;
                if ((!imgui_clicked && !native_clicked) ||
                    !IsAllowedMouseHotkey(button, allowed_mouse) ||
                    (button == ImGuiMouseButton_Left && !hovered))
                    continue;

                *key = mouse_keys[button];
                if (button == ImGuiMouseButton_Left)
                    g_hotkey_capture.suppress_bound_lbutton_release = id;
                captured = true;
                break;
            }

            if (!captured)
            {
                for (int vk = 1; vk < 256; ++vk)
                {
                    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
                        vk == VK_XBUTTON1 || vk == VK_XBUTTON2 ||
                        (GetAsyncKeyState(vk) & 1) == 0)
                        continue;

                    *key = (vk == VK_ESCAPE) ? 0 : vk;
                    captured = true;
                    break;
                }
            }

            if (captured)
            {
                g_hotkey_capture.capture = 0;
                g_hotkey_capture.wait_for_initial_click_release = false;
                active = false;
            }
        }
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    RectFilled(dl, min, max, active ? Accent(0.32f) : (hovered ? C.pill_hover : C.pill), 8.0f);
    RectStroke(dl, min, max, active ? Accent(0.65f) : C.border, 8.0f);
    const std::string key_name = HotkeyName(*key);
    TextCentered(dl, F_Body, ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f),
                 active ? C.text : C.text_dim, active ? pstra("Press key") : key_name.c_str());
    return clicked;
}

bool Blade::Toggle(const char* id_str, ImVec2 center_right, bool* v, float w, float h)
{
    ImDrawList* dl = CurDL();
    ImVec2 min(center_right.x - w, center_right.y - h * 0.5f);
    ImVec2 max(center_right.x, center_right.y + h * 0.5f);

    bool hovered = false;
    bool pressed = Hitbox(id_str, min, max, &hovered);
    if (pressed) *v = !*v;

    if (IsWhiteLabel())
    {
        const float rounding = 2.0f;
        const ImU32 fill = *v ? Accent(1.0f) : C.track;
        RectFilled(dl, min, max, fill, rounding);
        RectStroke(dl, min, max, *v ? Accent(1.0f) : C.border, rounding);

        const float knob_w = h - 6.0f;
        const float knob_x = *v ? max.x - knob_w - 3.0f : min.x + 3.0f;
        RectFilled(dl, ImVec2(knob_x, min.y + 3.0f),
                   ImVec2(knob_x + knob_w, max.y - 3.0f), C.text, 2.0f);
        return pressed;
    }

    ImGuiID id = ImGui::GetCurrentWindow()->GetID(id_str);
    Settings::sInterface& ui = Ui();
    float t = Anim(id, *v, 14.0f * ImMax(ui.ToggleLoad, 0.05f));

    RectFilled(dl, min, max, C.track, h * 0.5f);
    if (t > 0.01f)
        GradientBarH(dl, min, max, h * 0.5f, ColorU32(ui.ToggleStart, t), ColorU32(ui.ToggleEnd, t));
    if (hovered)
        RectStroke(dl, min, max, IM_COL32(255, 255, 255, 30), h * 0.5f);

    float kr = h * 0.5f - 3.0f;
    float kx = ImLerp(min.x + kr + 3.0f, max.x - kr - 3.0f, t);
    ImU32 knob = ImGui::GetColorU32(ImLerp(ImVec4(0.68f, 0.68f, 0.73f, 1.0f), ImVec4(1, 1, 1, 1), t));
    dl->AddCircleFilled(ImVec2(kx, min.y + h * 0.5f), kr, A(knob), 20);

    return pressed;
}

bool Blade::Radio(const char* id_str, ImVec2 center, bool selected, float r)
{
    ImDrawList* dl = CurDL();
    bool hovered = false;
    bool pressed = Hitbox(id_str, ImVec2(center.x - r - 4, center.y - r - 4),
                          ImVec2(center.x + r + 4, center.y + r + 4), &hovered);

    ImGuiID id = ImGui::GetCurrentWindow()->GetID(id_str);
    float t = Anim(id, selected, 16.0f);

    dl->AddCircle(center, r, A(Accent(hovered ? 1.0f : 0.85f)), 24, 1.8f);
    if (t > 0.01f)
        dl->AddCircleFilled(center, (r - 3.0f) * t, A(Accent(1.0f)), 20);

    return pressed;
}

bool Blade::Checkbox(const char* id_str, ImVec2 center, bool* v, float sz)
{
    ImDrawList* dl = CurDL();
    ImVec2 min(center.x - sz * 0.5f, center.y - sz * 0.5f);
    ImVec2 max(center.x + sz * 0.5f, center.y + sz * 0.5f);

    bool hovered = false;
    bool pressed = Hitbox(id_str, min, max, &hovered);
    if (pressed) *v = !*v;

    if (IsWhiteLabel())
    {
        RectFilled(dl, min, max, *v ? Accent(1.0f) : C.pill, 3.0f);
        RectStroke(dl, min, max, *v ? Accent(1.0f) : (hovered ? C.text_dim : C.border), 3.0f);
        if (*v)
            DrawIcon(dl, IC_CHECK, center, sz * 0.74f, C.panel, 1.6f);
        return pressed;
    }

    ImGuiID id = ImGui::GetCurrentWindow()->GetID(id_str);
    Settings::sInterface& ui = Ui();
    float t = Anim(id, *v, 16.0f * ImMax(ui.ToggleLoad, 0.05f));

    ImU32 off = hovered ? C.pill_hover : C.pill;
    RectFilled(dl, min, max, off, sz * 0.26f);
    if (t > 0.01f)
        GradientBarH(dl, min, max, sz * 0.26f, ColorU32(ui.CheckboxStart, t), ColorU32(ui.CheckboxEnd, t));
    if (t < 0.99f)
        RectStroke(dl, min, max, IM_COL32(255, 255, 255, (int)(26 * (1.0f - t))), sz * 0.26f);
    if (t > 0.05f)
        DrawIcon(dl, IC_CHECK, center, sz * 0.78f * t, IM_COL32(255, 255, 255, (int)(255 * t)), 1.5f);

    return pressed;
}

bool Blade::Button(const char* id_str, ImVec2 min, ImVec2 max, const char* label, bool accent)
{
    ImDrawList* dl = CurDL();
    bool hovered = false;
    bool pressed = Hitbox(id_str, min, max, &hovered);

    float rounding = IsWhiteLabel() ? 2.0f : 9.0f;
    if (accent)
        RectFilled(dl, min, max, ColorU32(Ui().PrimaryColor, hovered ? 1.0f : 0.88f), rounding);
    else
    {
        RectFilled(dl, min, max, hovered ? C.pill_hover : C.pill, rounding);
        RectStroke(dl, min, max, C.border, rounding);
    }
    TextCentered(dl, F_Med, ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f),
                 C.text, label);
    return pressed;
}

void Blade::DrawBlock(ImDrawList* dl, ImVec2 c, float size, ImU32 base)
{
    ImVec4 b = ImGui::ColorConvertU32ToFloat4(base);
    auto Shade = [&](float f) {
        return ImGui::GetColorU32(ImVec4(ImMin(b.x * f, 1.0f), ImMin(b.y * f, 1.0f), ImMin(b.z * f, 1.0f), 1.0f));
    };

    const float w = size, h = size * 1.06f;
    ImVec2 top[4] = {
        ImVec2(c.x,           c.y - h * 0.50f), ImVec2(c.x + w * 0.5f, c.y - h * 0.25f),
        ImVec2(c.x,           c.y),             ImVec2(c.x - w * 0.5f, c.y - h * 0.25f)
    };
    ImVec2 left[4] = {
        ImVec2(c.x - w * 0.5f, c.y - h * 0.25f), ImVec2(c.x, c.y),
        ImVec2(c.x,            c.y + h * 0.50f), ImVec2(c.x - w * 0.5f, c.y + h * 0.25f)
    };
    ImVec2 right[4] = {
        ImVec2(c.x,            c.y),             ImVec2(c.x + w * 0.5f, c.y - h * 0.25f),
        ImVec2(c.x + w * 0.5f, c.y + h * 0.25f), ImVec2(c.x, c.y + h * 0.50f)
    };

    dl->AddConvexPolyFilled(top,   4, Shade(1.18f));
    dl->AddConvexPolyFilled(left,  4, Shade(0.62f));
    dl->AddConvexPolyFilled(right, 4, Shade(0.86f));

    ImU32 edge = Shade(0.45f);
    dl->AddLine(top[0], top[1], edge, 1.0f);
    dl->AddLine(top[1], top[2], edge, 1.0f);
    dl->AddLine(top[2], top[3], edge, 1.0f);
    dl->AddLine(top[3], top[0], edge, 1.0f);
    dl->AddLine(left[1], left[2], edge, 1.0f);
    dl->AddLine(left[2], left[3], edge, 1.0f);
    dl->AddLine(right[2], right[3], edge, 1.0f);
    dl->AddLine(right[1], right[2], edge, 1.0f);
}

struct HudSlot { ImVec2 pos; bool placed; };
std::map<ImGuiID, HudSlot> g_blade_hud_slots;

ImVec2 Blade::HudBegin(const char* id_str, ImVec2 def_pos)
{
    ImGuiID id = ImHashStr(id_str);
    auto it = g_blade_hud_slots.find(id);
    if (it == g_blade_hud_slots.end())
        it = g_blade_hud_slots.insert({ id, HudSlot{ def_pos, false } }).first;

    if (!it->second.placed)
        it->second.pos = def_pos;

    return it->second.pos;
}

void Blade::HudEnd(const char* id_str, ImVec2 min, ImVec2 max, float full_h)
{
    ImGuiID id = ImHashStr(id_str);
    auto it = g_blade_hud_slots.find(id);
    if (it == g_blade_hud_slots.end()) return;

    char hid[96];
    snprintf(hid, sizeof(hid), pstra("##huddrag_%s"), id_str);

    bool hovered = false;
    Hitbox(hid, min, max, &hovered);
    ImGuiID hit_id = ImGui::GetCurrentWindow()->GetID(hid);

    if (GImGui->ActiveId == hit_id && ImGui::IsMouseDown(0))
    {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        if (d.x != 0.0f || d.y != 0.0f)
        {
            it->second.pos.x += d.x;
            it->second.pos.y += d.y;
            it->second.placed = true;
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    ImVec2 ds = Screen();
    if (ds.x > 120.0f && ds.y > 60.0f)
    {
        ImVec2 sz(max.x - min.x, max.y - min.y);
        if (full_h > 0.0f)
        {

            const float m = 8.0f;
            it->second.pos.x = ImClamp(it->second.pos.x, m, ImMax(m, ds.x - sz.x - m));
            it->second.pos.y = ImClamp(it->second.pos.y, m, ImMax(m, ds.y - full_h - m));
        }
        else
        {

            it->second.pos.x = ImClamp(it->second.pos.x, -sz.x + 60.0f, ds.x - 60.0f);
            it->second.pos.y = ImClamp(it->second.pos.y, 0.0f, ds.y - 30.0f);
        }
    }
}

PopupCtx Blade::P ;

void Blade::OpenPopup(PopupKind k, ImGuiID id, ImVec2 amin, ImVec2 amax,
               void* data, const char* const* opts, int count)
{
    P.kind = k;
    P.id = id;
    P.amin = amin;
    P.amax = amax;
    P.data = data;
    P.option_storage.clear();
    P.option_ptrs.clear();

    if (opts && count > 0)
    {
        P.option_storage.reserve(count);
        for (int index = 0; index < count; ++index)
        {
            if (opts[index])
                P.option_storage.emplace_back(opts[index]);
            else
                P.option_storage.emplace_back();
        }

        P.option_ptrs.reserve(P.option_storage.size());
        for (const std::string& option : P.option_storage)
            P.option_ptrs.push_back(option.c_str());
    }

    P.opts = P.option_ptrs.empty() ? nullptr : P.option_ptrs.data();
    P.count = static_cast<int>(P.option_ptrs.size());
    P.anim = 0.0f;
    P.sub = -1;
    P.placed = false;
}

void Blade::ClosePopup()
{
    P.kind = PK_NONE;
    P.id = 0;
    P.data = nullptr;
    P.opts = nullptr;
    P.option_storage.clear();
    P.option_ptrs.clear();
    P.count = 0;
    P.sub = -1;
    S.aim_bind_edit = -1;
    S.aim_capturing = false;
}

bool Blade::IsPopupOpen(ImGuiID id) { return P.kind != PK_NONE && P.id == id; }

ColorPickerCtx Blade::CP ;

void Blade::OpenColorPicker(ImGuiID id, ImVec2 anchor_min, ImVec2 anchor_max, float* col,
                     const char* label)
{
    CP.open = true;
    CP.id = id;
    CP.col = col;
    snprintf(CP.label, sizeof(CP.label), pstra("%s"), (label && label[0]) ? label : pstra("Color selection"));
    CP.anchor_min = anchor_min;
    CP.anchor_max = anchor_max;
    CP.placed = false;
    CP.anim = 0.0f;
}

void Blade::CloseColorPicker()
{
    CP.open = false;
    CP.id = 0;
    CP.col = nullptr;
}

bool Blade::ColorSwatch(const char* id_str, ImVec2 center, float size, float* col, const char* label)
{
    if (!col)
        return false;

    ImDrawList* dl = CurDL();
    const float half = size * 0.5f;
    const ImVec2 min(center.x - half, center.y - half);
    const ImVec2 max(center.x + half, center.y + half);
    const float hit_half = half + (IsWhiteLabel() ? 0.0f : 2.0f);
    bool hovered = false;
    const bool pressed = Hitbox(id_str, ImVec2(center.x - hit_half, center.y - hit_half),
                                ImVec2(center.x + hit_half, center.y + hit_half), &hovered);
    if (pressed)
        OpenColorPicker(ImGui::GetCurrentWindow()->GetID(id_str), min, max, col, label);

    const ImU32 color = ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], 1.0f));
    if (IsWhiteLabel())
    {
        RectFilled(dl, min, max, color, 2.0f);
        RectStroke(dl, min, max, hovered ? C.text : C.border, 2.0f, ImDrawCornerFlags_All, hovered ? 1.5f : 1.0f);
    }
    else
    {
        dl->AddCircleFilled(center, half, A(Fade(color, 0.22f)), 20);
        dl->AddCircleFilled(center, half * 0.36f, A(color), 12);
        if (hovered)
            dl->AddCircle(center, half + 2.0f, A(Fade(color, 0.55f)), 20);
    }

    return pressed;
}

bool Blade::Dropdown(const char* id_str, ImVec2 min, ImVec2 max,
              const char* const* opts, int count, int* value)
{
    ImDrawList* dl = CurDL();
    bool hovered = false;
    bool pressed = Hitbox(id_str, min, max, &hovered);

    ImGuiID id = ImGui::GetCurrentWindow()->GetID(id_str);
    bool open = IsPopupOpen(id);

    if (pressed)
    {
        if (open) ClosePopup();
        else      OpenPopup(PK_DROPDOWN, id, min, max, (void*)value, opts, count);
    }

    const bool white_label = IsWhiteLabel();
    float rounding = white_label ? 3.0f : (max.y - min.y) * 0.32f;
    RectFilled(dl, min, max,
               (hovered || open) ? C.pill_hover : C.pill, rounding);
    RectStroke(dl, min, max, open ? Accent(1.0f) : (white_label ? C.border : Fade(C.border, 0.0f)), rounding);

    float cy = (min.y + max.y) * 0.5f;
    const std::string value_txt = (*value >= 0 && *value < count) ? std::string(opts[*value]) : std::string();
    TextAt(dl, F_Body, ImVec2(min.x + 9.0f, cy - Measure(F_Body, value_txt.c_str()).y * 0.5f), C.text, value_txt.c_str());

    float t = Anim(id + 991, open, 16.0f);
    ImVec2 cc(max.x - 11.0f, cy);
    if (white_label)
    {
        const ImU32 arrow = open ? Accent(1.0f) : C.text_dim;
        dl->AddLine(ImVec2(cc.x - 3.5f, cc.y - 1.5f), ImVec2(cc.x, cc.y + 2.0f), arrow, 1.5f);
        dl->AddLine(ImVec2(cc.x, cc.y + 2.0f), ImVec2(cc.x + 3.5f, cc.y - 1.5f), arrow, 1.5f);
    }
    else if (t > 0.5f) DrawIcon(dl, IC_CHEVRON, ImVec2(cc.x, cc.y + 1.0f), 11.0f, Accent(1.0f), 1.4f);
    else               DrawIcon(dl, IC_CHEVRON, cc, 11.0f, C.text_dim, 1.4f);

    return pressed;
}

static float SliderLoad(ImGuiID id, float intro)
{
    Settings::sInterface& ui = Ui();
    if (IsWhiteLabel() || !ui.IntroAnimation) return 1.0f;
    float& ld = g_blade_slider_load_values[id];
    if (intro <= 0.001f) ld = 0.0f;
    float dt = ImGui::GetIO().DeltaTime * 1.3f * ImMax(ui.AnimationSpeed, 0.05f)
             * ImMax(ui.SliderLoad, 0.05f);
    ld = ImClamp(ld + dt, 0.0f, 1.0f);
    float inv = 1.0f - ld;
    return 1.0f - inv * inv * inv;
}

bool Blade::SliderF(const char* id_str, ImVec2 min, ImVec2 max, float* v, float v_min, float v_max,
             float intro)
{
    ImDrawList* dl = CurDL();
    float cy = (min.y + max.y) * 0.5f;
    const bool white_label = IsWhiteLabel();
    float track_h = white_label ? 4.0f : 9.0f;

    bool hovered = false;
    Hitbox(id_str, ImVec2(min.x, cy - 11.0f), ImVec2(max.x, cy + 11.0f), &hovered);

    ImGuiID id = ImGui::GetCurrentWindow()->GetID(id_str);
    bool active = (GImGui->ActiveId == id);
    bool changed = false;
    if (active && ImGui::IsMouseDown(0))
    {
        float t = ImClamp((ImGui::GetIO().MousePos.x - min.x) / ImMax(max.x - min.x, 1.0f), 0.0f, 1.0f);
        float nv = v_min + t * (v_max - v_min);
        if (nv != *v) { *v = nv; changed = true; }
    }

    float ia = SliderLoad(id, intro);
    float t = ImClamp((*v - v_min) / ImMax(v_max - v_min, 0.0001f), 0.0f, 1.0f) * ia;
    float kx = ImLerp(min.x + 6.0f, max.x - 6.0f, t);

    RectFilled(dl, ImVec2(min.x, cy - track_h * 0.5f), ImVec2(max.x, cy + track_h * 0.5f),
                      C.track, track_h * 0.5f);

    if (kx > min.x + 0.5f && !white_label)
        GradientBarH(dl, ImVec2(min.x, cy - track_h * 0.5f), ImVec2(kx, cy + track_h * 0.5f),
                     track_h * 0.5f, ColorU32(Ui().SliderStart), ColorU32(Ui().SliderEnd));
    else if (kx > min.x + 0.5f)
        RectFilled(dl, ImVec2(min.x, cy - track_h * 0.5f), ImVec2(kx, cy + track_h * 0.5f), Accent(1.0f), 2.0f);

    if (white_label)
    {
        const float knob_h = (hovered || active) ? 15.0f : 13.0f;
        RectFilled(dl, ImVec2(kx - 3.0f, cy - knob_h * 0.5f), ImVec2(kx + 3.0f, cy + knob_h * 0.5f), C.text, 1.0f);
    }
    else
    {
        dl->AddCircleFilled(ImVec2(kx, cy), (hovered || active) ? 9.5f : 8.5f,
                            A(IM_COL32(255, 255, 255, 255)), 24);
        dl->AddCircle(ImVec2(kx, cy), (hovered || active) ? 9.5f : 8.5f,
                      A(ColorU32(Ui().SliderEnd, 0.95f)), 24, 1.7f);
    }

    return changed;
}

bool Blade::RangeSlider(const char* id_str, ImVec2 min, ImVec2 max, float* lo, float* hi,
                 float v_min, float v_max, float intro)
{
    ImDrawList* dl = CurDL();
    float cy = (min.y + max.y) * 0.5f;
    const bool white_label = IsWhiteLabel();
    float track_h = white_label ? 4.0f : 9.0f;
    float span = ImMax(max.x - min.x - 12.0f, 1.0f);
    float ia = SliderLoad(ImGui::GetCurrentWindow()->GetID(id_str), intro);

    auto ValToX = [&](float v) { return min.x + 6.0f + ImClamp((v - v_min) / ImMax(v_max - v_min, 0.0001f), 0.0f, 1.0f) * span; };
    auto XToVal = [&](float x) { return v_min + ImClamp((x - min.x - 6.0f) / span, 0.0f, 1.0f) * (v_max - v_min); };

    auto Animated = [&](float x) {
        float mid = ValToX((*lo + *hi) * 0.5f);
        return ImLerp(mid, x, ia);
    };

    float x_lo = Animated(ValToX(*lo)), x_hi = Animated(ValToX(*hi));

    RectFilled(dl, ImVec2(min.x, cy - track_h * 0.5f), ImVec2(max.x, cy + track_h * 0.5f),
                      C.track, track_h * 0.5f);
    if (x_hi > x_lo + 0.5f && !white_label)
        GradientBarH(dl, ImVec2(x_lo, cy - track_h * 0.5f), ImVec2(x_hi, cy + track_h * 0.5f),
                     track_h * 0.5f, ColorU32(Ui().SliderStart), ColorU32(Ui().SliderEnd));
    else if (x_hi > x_lo + 0.5f)
        RectFilled(dl, ImVec2(x_lo, cy - track_h * 0.5f), ImVec2(x_hi, cy + track_h * 0.5f), Accent(1.0f), 2.0f);

    bool changed = false;
    char buf[64];

    snprintf(buf, sizeof(buf), pstra("%s_lo"), id_str);
    bool h_lo = false;
    Hitbox(buf, ImVec2(x_lo - 8, cy - 9), ImVec2(x_lo + 8, cy + 9), &h_lo);
    ImGuiID id_lo = ImGui::GetCurrentWindow()->GetID(buf);
    if (GImGui->ActiveId == id_lo && ImGui::IsMouseDown(0))
    {
        *lo = ImMin(XToVal(ImGui::GetIO().MousePos.x), *hi);
        changed = true;
    }

    snprintf(buf, sizeof(buf), pstra("%s_hi"), id_str);
    bool h_hi = false;
    Hitbox(buf, ImVec2(x_hi - 8, cy - 9), ImVec2(x_hi + 8, cy + 9), &h_hi);
    ImGuiID id_hi = ImGui::GetCurrentWindow()->GetID(buf);
    if (GImGui->ActiveId == id_hi && ImGui::IsMouseDown(0))
    {
        *hi = ImMax(XToVal(ImGui::GetIO().MousePos.x), *lo);
        changed = true;
    }

    x_lo = Animated(ValToX(*lo)); x_hi = Animated(ValToX(*hi));
    if (white_label)
    {
        const float lo_h = h_lo ? 15.0f : 13.0f;
        const float hi_h = h_hi ? 15.0f : 13.0f;
        RectFilled(dl, ImVec2(x_lo - 3.0f, cy - lo_h * 0.5f), ImVec2(x_lo + 3.0f, cy + lo_h * 0.5f), C.text, 1.0f);
        RectFilled(dl, ImVec2(x_hi - 3.0f, cy - hi_h * 0.5f), ImVec2(x_hi + 3.0f, cy + hi_h * 0.5f), C.text, 1.0f);
    }
    else
    {
        dl->AddCircleFilled(ImVec2(x_lo, cy), h_lo ? 9.5f : 8.5f, A(IM_COL32(255, 255, 255, 255)), 24);
        dl->AddCircle(ImVec2(x_lo, cy), h_lo ? 9.5f : 8.5f, A(ColorU32(Ui().SliderEnd, 0.95f)), 24, 1.7f);
        dl->AddCircleFilled(ImVec2(x_hi, cy), h_hi ? 9.5f : 8.5f, A(IM_COL32(255, 255, 255, 255)), 24);
        dl->AddCircle(ImVec2(x_hi, cy), h_hi ? 9.5f : 8.5f, A(ColorU32(Ui().SliderEnd, 0.95f)), 24, 1.7f);
    }

    return changed;
}

bool Blade::IconButton(const char* id_str, Icon ic, ImVec2 center, float size, ImU32 col, float hit)
{
    ImDrawList* dl = CurDL();
    bool hovered = false;
    bool pressed = Hitbox(id_str, ImVec2(center.x - hit, center.y - hit),
                          ImVec2(center.x + hit, center.y + hit), &hovered);
    if (hovered)
        dl->AddCircleFilled(center, hit, IM_COL32(255, 255, 255, 16), 20);
    DrawIcon(dl, ic, center, size, hovered ? C.text : col, 1.6f);
    return pressed;
}

ImVec2 Blade::SettingRow(ImDrawList* dl, ImVec2 min, float w, float h, const char* label, bool dim)
{
    ImVec2 max(min.x + w, min.y + h);
    RectFilled(dl, min, max, C.row, 8.0f);
    ImVec2 ts = Measure(F_Body, label);
    TextAt(dl, F_Body, ImVec2(min.x + 12.0f, min.y + (h - ts.y) * 0.5f),
           dim ? C.text_mute : C.text, label);
    return max;
}
