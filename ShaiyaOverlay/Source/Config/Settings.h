#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "ThirdParty/ImGui/imgui.h"

namespace ConfigColors
{
    inline ImColor Accent()      { return ImColor(0.545f, 0.361f, 0.965f, 1.0f); }
    inline ImColor Text()        { return ImColor(0.96f, 0.96f, 0.98f, 1.0f); }
    inline ImColor Panel()       { return ImColor(0.078f, 0.082f, 0.106f, 1.0f); }
    inline ImColor Primary()     { return ImColor(0.545f, 0.361f, 0.965f, 1.0f); }
    inline ImColor GradientEnd() { return ImColor(0.745f, 0.42f, 0.98f, 1.0f); }
    inline ImColor Success()     { return ImColor(0.24f, 0.82f, 0.44f, 1.0f); }
    inline ImColor Warning()     { return ImColor(0.96f, 0.64f, 0.18f, 1.0f); }
    inline ImColor Error()       { return ImColor(0.92f, 0.28f, 0.28f, 1.0f); }

    inline ImColor MixColor(const ImColor& a, const ImColor& b, float t)
    {
        return ImColor(
            a.Value.x + (b.Value.x - a.Value.x) * t,
            a.Value.y + (b.Value.y - a.Value.y) * t,
            a.Value.z + (b.Value.z - a.Value.z) * t,
            a.Value.w + (b.Value.w - a.Value.w) * t
        );
    }
}

namespace Shaiya::Config
{
    struct sWhiteLabelTheme
    {
        ImColor TextColor = ConfigColors::Text();
        ImColor BackgroundColor = ConfigColors::Panel();
        ImColor PrimaryColor = ConfigColors::Primary();
        ImColor SecondaryColor = ConfigColors::GradientEnd();
        char Name[50] = {};
        int Id = 0;
        bool Valid = false;
    };

    struct sInterface
    {
        int PanicKey = VK_END;
        int MenuLang = 0;

        bool AutoScale = false;
        bool Glass = true;
        bool Shadows = true;
        bool GradientText = false;
        bool IntroAnimation = true;
        bool LogoGlitch = true;
        bool Wallpaper = true;
        bool Particles = true;

        float UiScale = 1.2f;
        float AnimationSpeed = 1.0f;
        float SliderLoad = 1.0f;
        float ToggleLoad = 1.0f;
        float IntroCascade = 1.0f;
        float EspBuild = 1.0f;
        float CornerRounding = 1.0f;
        float SplashOpen = 1.2f;
        float LogoGlitchSpeed = 1.0f;

        ImColor AccentColor = ConfigColors::Accent();
        ImColor TextColor = ConfigColors::Text();
        ImColor PanelColor = ConfigColors::Panel();
        ImColor GradientStart = ConfigColors::Primary();
        ImColor GradientEnd = ConfigColors::GradientEnd();
        ImColor SliderStart = ConfigColors::Primary();
        ImColor SliderEnd = ConfigColors::GradientEnd();
        ImColor ToggleStart = ConfigColors::Primary();
        ImColor ToggleEnd = ConfigColors::GradientEnd();
        ImColor CheckboxStart = ConfigColors::Primary();
        ImColor CheckboxEnd = ConfigColors::GradientEnd();
        ImColor PrimaryColor = ConfigColors::Primary();
        ImColor SuccessColor = ConfigColors::Success();
        ImColor WarningColor = ConfigColors::Warning();
        ImColor ErrorColor = ConfigColors::Error();

        void Reset()
        {
            PanicKey = VK_END;
            MenuLang = 0;

            AutoScale = false;
            Glass = true;
            Shadows = true;
            GradientText = false;
            IntroAnimation = true;
            LogoGlitch = true;
            Wallpaper = true;
            Particles = true;

            UiScale = 1.5f;
            AnimationSpeed = 1.0f;
            SliderLoad = 1.0f;
            ToggleLoad = 1.0f;
            IntroCascade = 1.0f;
            EspBuild = 1.0f;
            CornerRounding = 1.0f;
            SplashOpen = 1.2f;
            LogoGlitchSpeed = 1.0f;

            AccentColor = ConfigColors::Accent();
            TextColor = ConfigColors::Text();
            PanelColor = ConfigColors::Panel();
            GradientStart = ConfigColors::Primary();
            GradientEnd = ConfigColors::GradientEnd();
            SliderStart = ConfigColors::Primary();
            SliderEnd = ConfigColors::GradientEnd();
            ToggleStart = ConfigColors::Primary();
            ToggleEnd = ConfigColors::GradientEnd();
            CheckboxStart = ConfigColors::Primary();
            CheckboxEnd = ConfigColors::GradientEnd();
            PrimaryColor = ConfigColors::Primary();
            SuccessColor = ConfigColors::Success();
            WarningColor = ConfigColors::Warning();
            ErrorColor = ConfigColors::Error();
        }
    };

    struct VisualsConfig
    {
        bool Enable = true;
        bool MonsterEsp = true;
        bool MonsterBox = true;
        bool MonsterName = true;
        bool MonsterHp = true;
        bool MonsterDist = true;
        ImVec4 MonsterColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);

        bool NpcEsp = true;
        ImVec4 NpcColor = ImVec4(0.2f, 0.8f, 1.0f, 1.0f);

        bool Loot = true;
        bool LootSnaplines = true;
        ImVec4 LootColor = ImVec4(0.0f, 1.0f, 0.3f, 1.0f);

        bool QuestWaypoints = true;
        ImVec4 QuestColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);

        void Reset()
        {
            Enable = true;
            MonsterEsp = true;
            MonsterBox = true;
            MonsterName = true;
            MonsterHp = true;
            MonsterDist = true;
            MonsterColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            NpcEsp = true;
            NpcColor = ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
            Loot = true;
            LootSnaplines = true;
            LootColor = ImVec4(0.0f, 1.0f, 0.3f, 1.0f);
            QuestWaypoints = true;
            QuestColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
        }
    };

    struct Settings
    {
        using sInterface = Shaiya::Config::sInterface;
        using sWhiteLabelTheme = Shaiya::Config::sWhiteLabelTheme;

        bool ShowInterface = true;
        int ShowMenuKey = VK_INSERT;

        bool WhiteLabel = false;
        sWhiteLabelTheme WhiteLabelTheme;
        sInterface Interface;
        VisualsConfig Visuals;

        void ResetSettings()
        {
            ShowInterface = true;
            ShowMenuKey = VK_INSERT;
            Visuals.Reset();
        }

        void InitializeRuntimeDefaults()
        {
            ResetSettings();
            Interface.Reset();
            WhiteLabel = false;
            WhiteLabelTheme = sWhiteLabelTheme{};
        }

        void ApplyWhiteLabelTheme(std::uint32_t text, std::uint32_t background,
                                  std::uint32_t primary, std::uint32_t secondary,
                                  const char* name, int id)
        {
            auto to_color = [](std::uint32_t c) {
                const float r = ((c >> 16) & 0xFF) / 255.0f;
                const float g = ((c >> 8) & 0xFF) / 255.0f;
                const float b = (c & 0xFF) / 255.0f;
                const float a = ((c >> 24) & 0xFF) / 255.0f;
                return ImColor(r, g, b, a > 0.0f ? a : 1.0f);
            };

            WhiteLabelTheme.TextColor = to_color(text);
            WhiteLabelTheme.BackgroundColor = to_color(background);
            WhiteLabelTheme.PrimaryColor = to_color(primary);
            WhiteLabelTheme.SecondaryColor = to_color(secondary);
            WhiteLabelTheme.Id = id;
            WhiteLabelTheme.Valid = true;
            strncpy_s(WhiteLabelTheme.Name, name ? name : "", _TRUNCATE);

            Interface.TextColor = WhiteLabelTheme.TextColor;
            Interface.PanelColor = WhiteLabelTheme.BackgroundColor;
            Interface.AccentColor = WhiteLabelTheme.PrimaryColor;
            Interface.GradientStart = WhiteLabelTheme.PrimaryColor;
            Interface.GradientEnd = WhiteLabelTheme.SecondaryColor;
            Interface.SliderStart = WhiteLabelTheme.PrimaryColor;
            Interface.SliderEnd = WhiteLabelTheme.SecondaryColor;
            Interface.ToggleStart = WhiteLabelTheme.PrimaryColor;
            Interface.ToggleEnd = WhiteLabelTheme.SecondaryColor;
            Interface.CheckboxStart = WhiteLabelTheme.PrimaryColor;
            Interface.CheckboxEnd = WhiteLabelTheme.SecondaryColor;
            Interface.PrimaryColor = WhiteLabelTheme.PrimaryColor;
            Interface.ErrorColor = ConfigColors::MixColor(WhiteLabelTheme.SecondaryColor, WhiteLabelTheme.BackgroundColor, 0.18f);

            WhiteLabel = true;
        }

        void EnsureInterfaceDefaults()
        {
            const bool invalid_scale = !(Interface.UiScale >= 0.5f && Interface.UiScale <= 2.5f);
            const bool invalid_colors = Interface.PanelColor.Value.w <= 0.001f ||
                Interface.TextColor.Value.w <= 0.001f || Interface.AccentColor.Value.w <= 0.001f;

            if (invalid_scale || invalid_colors)
            {
                const bool restore_white_label = WhiteLabel && WhiteLabelTheme.Valid;
                const sWhiteLabelTheme saved_theme = WhiteLabelTheme;
                Interface.Reset();

                if (restore_white_label)
                {
                    ApplyWhiteLabelTheme(
                        saved_theme.TextColor.operator ImU32(),
                        saved_theme.BackgroundColor.operator ImU32(),
                        saved_theme.PrimaryColor.operator ImU32(),
                        saved_theme.SecondaryColor.operator ImU32(),
                        saved_theme.Name,
                        saved_theme.Id
                    );
                }
            }
        }

        static Settings& Get()
        {
            static Settings instance;
            return instance;
        }
    };
}

using Settings = Shaiya::Config::Settings;
