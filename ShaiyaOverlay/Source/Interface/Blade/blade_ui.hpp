#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_internal.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Security/ProtectMacro.h"
#include "Config/Settings.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

namespace Blade
{

    class ProtectedText
    {
    public:
        ProtectedText() = default;

        ProtectedText(const char* value)
        {
            if (value)
                value_ = value;
        }

        const char* c_str() const
        {
            return value_.c_str();
        }

        operator const char*() const
        {
            return c_str();
        }

    private:
        std::string value_;
    };

    class ProtectedStringList
    {
    public:
        ProtectedStringList(std::initializer_list<const char*> values)
        {
            values_.reserve(values.size());
            for (const char* value : values)
            {
                if (value)
                    values_.emplace_back(value);
                else
                    values_.emplace_back();
            }

            pointers_.reserve(values_.size());
            for (const std::string& value : values_)
                pointers_.push_back(value.c_str());
        }

        const char* const* Data() const
        {
            return pointers_.data();
        }

        int Count() const
        {
            return static_cast<int>(pointers_.size());
        }

        const char* operator[](int index) const
        {
            return pointers_[index];
        }

    private:
        std::vector<std::string> values_;
        std::vector<const char*> pointers_;
    };

    extern ImFont* F_Tiny;
    extern ImFont* F_Small;
    extern ImFont* F_Body;
    extern ImFont* F_Med;
    extern ImFont* F_Label;
    extern ImFont* F_Title;
    extern ImFont* F_Head;
    extern ImFont* F_Logo;

    void LoadFonts(ImGuiIO& io, const void* regular, int reg_size,
                   const void* medium, int med_size,
                   const void* bold, int bold_size);
    bool IsWhiteLabel();
    const char* WhiteLabelName();

    float  Scale();
    ImVec2 Screen();
    void   BeginScale();
    void   ScaleDrawList(ImDrawList* dl, int vtx_start, int cmd_start);
    void   EndScale();

    extern ImTextureID TexLogoWide;
    extern ImVec2      TexLogoWideSz;
    extern ImTextureID TexLogoStack;
    extern ImVec2      TexLogoStackSz;

    void SetLogos(ImTextureID wide, ImVec2 wide_sz, ImTextureID stack, ImVec2 stack_sz);

    extern ImTextureID TexWhiteLabelLogo;
    extern ImVec2      TexWhiteLabelLogoSz;
    extern ImVec2      TexWhiteLabelLogoUV0;
    extern ImVec2      TexWhiteLabelLogoUV1;
    void SetWhiteLabelLogo(ImTextureID texture, ImVec2 size, ImVec2 uv0, ImVec2 uv1);

    extern ImTextureID TexChar;
    extern ImVec2      TexCharSz;
    extern ImVec2      TexCharUV0, TexCharUV1;
    void SetCharacter(ImTextureID tex, ImVec2 size, ImVec2 uv0, ImVec2 uv1);

    struct Palette
    {
        ImU32 panel;
        ImU32 panel_solid;
        ImU32 sidebar;
        ImU32 border;
        ImU32 separator;
        ImU32 divider;
        ImU32 row;
        ImU32 row_hover;
        ImU32 pill;
        ImU32 pill_hover;
        ImU32 sel;
        ImU32 track;
        ImU32 scroll;
        ImU32 text;
        ImU32 text_dim;
        ImU32 text_mute;
        ImU32 accent;
        ImU32 accent_soft;
        float alpha;
    };
    extern Palette C;
    void UpdatePalette();
    ImU32 Accent(float a = 1.0f);
    ImU32 GradA(float a = 1.0f);
    ImU32 GradB(float a = 1.0f);

    enum Icon
    {
        IC_NONE = 0,
        IC_BOLT,
        IC_USER,
        IC_WIFI,
        IC_GAUGE,
        IC_MAPPIN,
        IC_CLOSE,
        IC_DRAG,
        IC_PIN,
        IC_DOTS,
        IC_CHEVRON,
        IC_ARROWS_LR,
        IC_SEARCH,
        IC_FOLDER,
        IC_SWORD,
        IC_RUN,
        IC_EYE,
        IC_CUBE,
        IC_SLIDERS,
        IC_PALETTE,
        IC_CART,
        IC_FLAME,
        IC_POTION,
        IC_SPEED,
        IC_FIST,
        IC_PREV,
        IC_PAUSE,
        IC_NEXT,
        IC_SUN,
        IC_HELMET,
        IC_CHESTPLATE,
        IC_LEGGINGS,
        IC_BOOTS,
        IC_SWORD_SMALL,
        IC_OWL,
        IC_KEYBIND,
        IC_GEAR,
        IC_SORT,
        IC_FILTER,
        IC_CHEVRON_R,
        IC_CHEVRON_L,
        IC_CHECK,
        IC_COPY,
        IC_HOTKEY,
        IC_LANG,
        IC_DPI,
        IC_STYLE,
        IC_SUNDIM,
        IC_WARNING,
        IC_FRAME,
        IC_EYE_OFF,
        IC_TRASH,
        IC_BOOKMARK,
        IC_RESET,
        IC_PLUS,
        IC_MISC,
        IC_COUNT
    };

    void DrawIcon(ImDrawList* dl, Icon id, ImVec2 center, float size, ImU32 col, float thickness = 1.6f);

    void SetIconTex(int icon_id, ImTextureID tex, ImVec2 sz, bool colored = false);
    void SetTabTex(int tab, ImTextureID tex, ImVec2 sz);
    void SetProfileTex(ImTextureID tex, ImVec2 sz);
    void DrawTabIcon(ImDrawList* dl, int tab, Icon fallback, ImVec2 c, float size, ImU32 col);
    void DrawAvatar(ImDrawList* dl, ImVec2 center, float radius);

    ImDrawList* CurDL();
    void  PushDrawList(ImDrawList* dl);
    void  PopDrawList();

    void  RectFilled(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float rounding = 0.0f,
                     int corners = ImDrawCornerFlags_All, int max_seg = 32);
    void  RectStroke(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float rounding = 0.0f,
                     int corners = ImDrawCornerFlags_All, float thickness = 1.0f, int max_seg = 32);

    void  FadeTopRect(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float rounding);
    void  GradientBarV(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding,
                       const ImU32* stops, int stop_count);

    void  GradientBarH(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding,
                       ImU32 ca, ImU32 cb, int corners = ImDrawCornerFlags_All);

    void  Panel(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding = 12.0f);
    void  Shadow(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding = 12.0f, float spread = 14.0f);

    void  BeginShadowLayer(ImDrawList* dl);
    void  EndShadowLayer(ImDrawList* dl);
    void  TextAt(ImDrawList* dl, ImFont* f, ImVec2 pos, ImU32 col, const char* txt);
    void  TextRight(ImDrawList* dl, ImFont* f, ImVec2 right_pos, ImU32 col, const char* txt);
    void  TextCentered(ImDrawList* dl, ImFont* f, ImVec2 center, ImU32 col, const char* txt);
    ImVec2 Measure(ImFont* f, const char* txt);

    const char* FitEllipsis(ImFont* f, const char* text, float max_w, char* buf, int bufsz);

    void  SetTooltip(const char* title, const char* desc = nullptr);
    void  RenderTooltip(ImDrawList* fg);

    void  GlitchLogoAmbient(ImDrawList* dl, ImTextureID tex, ImVec2 texsz,
                            ImVec2 mn, ImVec2 mx, float alpha = 1.0f);

    void  ArmSplash();

    void  FadeDrawListAlpha(ImDrawList* dl, int vtx_start, float a);
    float Anim(ImGuiID id, bool target, float speed = 12.0f);
    ImU32 Fade(ImU32 col, float a);

    void  PushAlpha(float a);
    void  PopAlpha();
    ImU32 A(ImU32 col);

    void  TriggerIntro();
    float IntroT(float delay = 0.0f, float duration = 0.42f);

    bool Toggle(const char* id, ImVec2 center_right, bool* v, float w = 34.0f, float h = 18.0f);
    bool Radio(const char* id, ImVec2 center, bool selected, float r = 8.0f);
    bool Checkbox(const char* id, ImVec2 center, bool* v, float sz = 20.0f);

    bool SliderF(const char* id, ImVec2 min, ImVec2 max, float* v, float v_min, float v_max,
                 float intro = 1.0f);
    bool RangeSlider(const char* id, ImVec2 min, ImVec2 max, float* lo, float* hi,
                     float v_min, float v_max, float intro = 1.0f);
    bool IconButton(const char* id, Icon ic, ImVec2 center, float size, ImU32 col, float hit = 11.0f);
    bool Hitbox(const char* id, ImVec2 min, ImVec2 max, bool* hovered = nullptr);
    bool Button(const char* id, ImVec2 min, ImVec2 max, const char* label, bool accent = false);

    enum HotkeyMouseFlags : unsigned int
    {
        HK_MOUSE1 = 1u << 0,
        HK_MOUSE2 = 1u << 1,
        HK_MOUSE3 = 1u << 2,
        HK_MOUSE4 = 1u << 3,
        HK_MOUSE5 = 1u << 4,
        HK_MOUSE_ALL = HK_MOUSE1 | HK_MOUSE2 | HK_MOUSE3 | HK_MOUSE4 | HK_MOUSE5,
    };

    // Shared keybind control. Mouse1 can be disabled for bindings that would
    // otherwise react to the click used to configure them (e.g. open menu).
    bool HotkeyButton(const char* id, ImVec2 min, ImVec2 max, int* key,
                      unsigned int allowed_mouse = HK_MOUSE_ALL);

    bool Dropdown(const char* id, ImVec2 min, ImVec2 max, const char* const* opts, int count, int* value);

    void DrawBlock(ImDrawList* dl, ImVec2 center, float size, ImU32 base);

    ImVec2 SettingRow(ImDrawList* dl, ImVec2 min, float w, float h, const char* label, bool dim = false);

    ImVec2 HudBegin(const char* id, ImVec2 default_pos);

    void   HudEnd(const char* id, ImVec2 min, ImVec2 max, float full_h = -1.0f);

    enum PopupKind
    {
        PK_NONE = 0,
        PK_DROPDOWN,
        PK_PROFILE,
        PK_MODULES,
        PK_ITEMPICK,
        PK_SETTINGS,
        PK_HOTKEY,
    };

    struct PopupCtx
    {
        PopupKind          kind  = PK_NONE;
        ImGuiID            id    = 0;
        ImVec2             amin, amax;
        void*              data  = nullptr;
        const char* const* opts  = nullptr;
        std::vector<std::string> option_storage;
        std::vector<const char*> option_ptrs;
        int                count = 0;
        float              anim  = 0.0f;
        int                sub   = -1;
        ImVec2             pos;
        bool               placed = false;
    };
    extern PopupCtx P;

    struct ItemDef { ProtectedText name; ImU32 color; };
    extern const ItemDef kItems[];
    extern const int     kItemCount;

    struct Module { ProtectedText name; const char* const* subs; int sub_count; };
    const Module* GetModules(int tab, int* out_count);

    void OpenPopup(PopupKind k, ImGuiID id, ImVec2 amin, ImVec2 amax,
                   void* data = nullptr, const char* const* opts = nullptr, int count = 0);
    void ClosePopup();
    bool IsPopupOpen(ImGuiID id);

    struct ColorPickerCtx
    {
        bool    open   = false;
        ImGuiID id     = 0;
        float*  col    = nullptr;
        char    label[40]{};
        ImVec2  anchor_min, anchor_max;
        ImVec2  pos;
        bool    placed = false;
        float   anim   = 0.0f;
    };
    extern ColorPickerCtx CP;

    void OpenColorPicker(ImGuiID id, ImVec2 anchor_min, ImVec2 anchor_max, float* col,
                         const char* label = nullptr);
    void CloseColorPicker();
    bool ColorSwatch(const char* id, ImVec2 center, float size, float* col,
                     const char* label = nullptr);

    void RenderPopups();

    struct State
    {

        bool  wm_brand      = true;
        bool  wm_fps        = true;
        bool  wm_server     = true;
        bool  wm_latency    = true;
        bool  wm_time       = false;
        int   wm_order[5]   = { 0, 1, 2, 3, 4 };
        bool  wm_panel_open = true;

        bool  hud_watermark = true;
        bool  hud_coords    = true;
        bool  hud_bpstps    = true;
        bool  hud_potions   = true;
        bool  hud_keybinds  = true;
        bool  hud_media     = true;
        bool  hud_target    = true;
        bool  hud_hotbar    = true;
        bool  hud_inventory = true;
        bool  hud_notify    = true;

        bool  potions_pinned  = false;
        bool  keybinds_pinned = false;

        bool  th_panel_open   = true;
        int   th_hp_style     = 1;
        bool  th_nickname     = true;
        bool  th_equipment    = true;
        bool  th_healthbar    = true;

        bool  menu_open  = true;

        int   hk_capturing = -1;
        bool  panic        = false;
        int   menu_tab   = 0;
        int   module_sel[10] = { 0 };
        char  search[64]{};
        bool  search_focus = false;
        char  session_user[64]{};
        char  session_until[64]{};

        bool  test_toggle   = true;
        bool  test_check     = true;
        bool  test_check2    = false;
        float test_slider    = 62.0f;
        float test_range_lo  = 1.0f;
        float test_range_hi  = 4.0f;
        int   test_dropdown  = 0;
        float test_color[4]  = { 0.545f, 0.361f, 0.965f, 1.0f };

        ImVec2 aim_curve[4]  = { ImVec2(0.06f, 0.14f), ImVec2(0.33f, 0.62f),
                                 ImVec2(0.66f, 0.28f), ImVec2(0.93f, 0.86f) };

        struct Hotkey { char key[24]; bool visible; };
        Hotkey aim_keys[6]{};
        int    aim_key_count = 2;
        int    aim_key_mode  = 0;
        bool   aim_favorites = true;
        int    aim_bind_edit = -1;
        bool   aim_capturing = false;

        int   ab_sort      = 0;
        int   ab_slot      = -1;
        int   ab_pick      = 0;
        int   ab_items[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        float ab_scroll    = 0.0f;
        char  ab_search[64]{};

        int   rotation      = 0;
        bool  look          = true;
        int   aimbot        = 0;
        float rot_dist_lo   = 3.0f;
        float rot_dist_hi   = 4.5f;
        bool  ray_trace     = false;

        int   shield_break  = 0;
        bool  stop_sprint   = true;
        bool  teleport      = false;

        float tgt_dist_lo   = 3.0f;
        float tgt_dist_hi   = 4.5f;
        int   tgt_priority  = 0;
        int   tgt_mode      = 0;
        float fov           = 180.0f;

        bool  auto_criticals = true;
        float hit_delay      = 90.0f;
        bool  cooldown       = true;

        float media_pos   = 45.0f;
        float media_len   = 320.0f;
        bool  media_play  = true;

        float ping = 64.0f, fps = 144.0f, bps = 6.1f, tps = 20.0f;
        float pos_x = 1200.0f, pos_y = 270.0f, pos_z = 65.0f;
        float target_hp = 0.75f;

        void Reset()
        {
            wm_brand      = true;
            wm_fps        = true;
            wm_server     = true;
            wm_latency    = true;
            wm_time       = false;
            wm_order[0]   = 0; wm_order[1] = 1; wm_order[2] = 2; wm_order[3] = 3; wm_order[4] = 4;
            wm_panel_open = true;

            hud_watermark = true;
            hud_coords    = true;
            hud_bpstps    = true;
            hud_potions   = true;
            hud_keybinds  = true;
            hud_media     = true;
            hud_target    = true;
            hud_hotbar    = true;
            hud_inventory = true;
            hud_notify    = true;

            potions_pinned  = false;
            keybinds_pinned = false;

            th_panel_open   = true;
            th_hp_style     = 1;
            th_nickname     = true;
            th_equipment    = true;
            th_healthbar    = true;

            menu_open  = true;

            hk_capturing = -1;
            panic        = false;
            menu_tab   = 0;
            for (int i = 0; i < 10; ++i) module_sel[i] = 0;
            search[0] = '\0';
            search_focus = false;
            std::snprintf(session_user, sizeof(session_user), "%s", "Project Oficial");
            std::snprintf(session_until, sizeof(session_until), "%s", "Till: 1 Jan 2025");

            test_toggle   = true;
            test_check     = true;
            test_check2    = false;
            test_slider    = 62.0f;
            test_range_lo  = 1.0f;
            test_range_hi  = 4.0f;
            test_dropdown  = 0;
            test_color[0]  = 0.545f; test_color[1] = 0.361f; test_color[2] = 0.965f; test_color[3] = 1.0f;

            aim_curve[0] = ImVec2(0.06f, 0.14f);
            aim_curve[1] = ImVec2(0.33f, 0.62f);
            aim_curve[2] = ImVec2(0.66f, 0.28f);
            aim_curve[3] = ImVec2(0.93f, 0.86f);

            for (int i = 0; i < 6; ++i) { aim_keys[i].key[0] = '\0'; aim_keys[i].visible = false; }
            std::snprintf(aim_keys[0].key, sizeof(aim_keys[0].key), "%s", "Ctrl + F1");
            std::snprintf(aim_keys[1].key, sizeof(aim_keys[1].key), "%s", "Mouse5");
            aim_keys[0].visible = true;
            aim_keys[1].visible = true;
            aim_key_count = 2;
            aim_key_mode  = 0;
            aim_favorites = true;
            aim_bind_edit = -1;
            aim_capturing = false;

            ab_sort      = 0;
            ab_slot      = -1;
            ab_pick      = 0;
            for (int i = 0; i < 12; ++i) ab_items[i] = i;
            ab_scroll    = 0.0f;
            ab_search[0] = '\0';

            rotation      = 0;
            look          = true;
            aimbot        = 0;
            rot_dist_lo   = 3.0f;
            rot_dist_hi   = 4.5f;
            ray_trace     = false;

            shield_break  = 0;
            stop_sprint   = true;
            teleport      = false;

            tgt_dist_lo   = 3.0f;
            tgt_dist_hi   = 4.5f;
            tgt_priority  = 0;
            tgt_mode      = 0;
            fov           = 180.0f;

            auto_criticals = true;
            hit_delay      = 90.0f;
            cooldown       = true;

            media_pos   = 45.0f;
            media_len   = 320.0f;
            media_play  = true;

            ping = 64.0f; fps = 144.0f; bps = 6.1f; tps = 20.0f;
            pos_x = 1200.0f; pos_y = 270.0f; pos_z = 65.0f;
            target_hp = 0.75f;
        }

        State()
        {
            Reset();
        }
    };
    extern State S;

    void DrawWatermark(ImDrawList* dl);
    void DrawCoords(ImDrawList* dl);
    void DrawBpsTps(ImDrawList* dl);
    void DrawPotions(ImDrawList* dl);
    void DrawHotbar(ImDrawList* dl);
    void DrawInventory(ImDrawList* dl);
    void DrawWatermarkSettings(ImDrawList* dl);
    void DrawKeybinds(ImDrawList* dl);
    void DrawMediaPlayer(ImDrawList* dl);
    void DrawNotifications(ImDrawList* dl);

    enum NotifyType { NT_INFO = 0, NT_SUCCESS, NT_WARNING, NT_ERROR };
    void PushNotification(const char* text, int type = NT_INFO);
    void ClearNotifications();

    void DrawTargetHud(ImDrawList* dl);
    void DrawTargetHudSettings(ImDrawList* dl);
    void DrawMainMenu(ImDrawList* dl, float menu_a = 1.0f);

    void DrawVisualsContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                            float cx0, float cy0, float cw, float pad);
    void DrawLootItemEspContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                                float cx0, float cy0, float cw);
    void DrawDayZWorldContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                              float cx0, float cy0, float cw);
    void DrawDayZMiniMapContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                                float cx0, float cy0, float cw);

    void DrawAimbotContent(ImDrawList* dl, ImVec2 min, ImVec2 max,
                           float cx0, float cy0, float cw, float pad);

    void Render();
}
