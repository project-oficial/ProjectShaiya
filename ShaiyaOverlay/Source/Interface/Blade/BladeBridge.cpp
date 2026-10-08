#include "BladeBridge.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <algorithm>

#include "Config/Settings.h"
#include "Security/ProtectMacro.h"
#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_internal.h"
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/ImGui/stb_image.h"

#include "blade_ui.hpp"

#include "../Assets/ui_asset_background_jpg.h"
#include "../Assets/ui_asset_dashboard_png.h"
#include "../Assets/ui_asset_dpi_png.h"
#include "../Assets/ui_asset_language_png.h"
#include "../Assets/ui_asset_misc_png.h"
#include "../Assets/ui_asset_movement_png.h"
#include "../Assets/ui_asset_overlays_png.h"
#include "../Assets/ui_asset_render_png.h"
#include "../Assets/ui_asset_store_png.h"
#include "../Assets/ui_asset_styles_png.h"
#include "../Assets/ui_asset_teste_png.h"
#include "../Assets/ui_asset_theme_png.h"
#include "../Assets/ui_asset_aimbot_png.h"
#include "../Assets/ui_asset_alert_png.h"
#include "../Assets/ui_asset_character_png.h"
#include "../Assets/ui_asset_hotkeys_png.h"
#include "../Assets/ui_asset_items_png.h"
#include "../Assets/ui_asset_logo_stack_png.h"
#include "../Assets/ui_asset_logo_wide_png.h"
#include "../Assets/ui_asset_profile_jpg.h"
#include "../Assets/ui_asset_settings_png.h"
#include "../Assets/ui_asset_visuals_png.h"

#include "../Fonts/Montserrat-ExtraBold.h"
#include "../Fonts/Montserrat-Medium.h"
#include "../Fonts/Montserrat-Regular.h"

using namespace BladeBridge;

namespace
{
    bool LoadTextureFromMemoryProcessed(ID3D11Device* device, ID3D11ShaderResourceView** out_srv,
        const void* data, size_t data_size, int* out_width, int* out_height,
        ImVec2* out_uv0, ImVec2* out_uv1, bool force_white, bool chroma_key, bool trim_transparent)
    {
        if (!device || !out_srv || !data || data_size == 0)
            return false;

        int image_width = 0;
        int image_height = 0;
        unsigned char* image_data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data), static_cast<int>(data_size),
            &image_width, &image_height, nullptr, 4);
        if (!image_data || image_width <= 0 || image_height <= 0)
            return false;

        const size_t pixel_count = static_cast<size_t>(image_width) * static_cast<size_t>(image_height);
        std::vector<unsigned char> pixels(image_data, image_data + pixel_count * 4);
        stbi_image_free(image_data);

        auto clamp01 = [](float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); };
        for (size_t index = 0; index < pixels.size(); index += 4)
        {
            if (force_white)
            {
                pixels[index + 0] = 255;
                pixels[index + 1] = 255;
                pixels[index + 2] = 255;
            }

            if (chroma_key)
            {
                const int r = pixels[index + 0];
                const int g = pixels[index + 1];
                const int b = pixels[index + 2];
                const float initial_alpha = pixels[index + 3] / 255.0f;
                const float magenta_strength = (static_cast<float>(r + b) * 0.5f) - static_cast<float>(g);
                const float alpha = initial_alpha * (magenta_strength > 45.0f ? 1.0f - clamp01((magenta_strength - 45.0f) / 55.0f) : 1.0f);
                if (alpha < initial_alpha)
                {
                    const int cap = g + 45;
                    if (pixels[index + 0] > cap) pixels[index + 0] = static_cast<unsigned char>(cap);
                    if (pixels[index + 2] > cap) pixels[index + 2] = static_cast<unsigned char>(cap);
                }
                pixels[index + 3] = static_cast<unsigned char>(alpha * 255.0f);
            }
        }

        ImVec2 uv0(0.0f, 0.0f);
        ImVec2 uv1(1.0f, 1.0f);
        if (chroma_key || trim_transparent)
        {
            int x0 = image_width, y0 = image_height, x1 = -1, y1 = -1;
            for (int y = 0; y < image_height; ++y)
            {
                for (int x = 0; x < image_width; ++x)
                {
                    if (pixels[(static_cast<size_t>(y) * image_width + x) * 4 + 3] <= 8)
                        continue;
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
            }
            if (x1 >= x0 && y1 >= y0)
            {
                uv0 = ImVec2(static_cast<float>(x0) / image_width, static_cast<float>(y0) / image_height);
                uv1 = ImVec2(static_cast<float>(x1 + 1) / image_width, static_cast<float>(y1 + 1) / image_height);
            }
        }

        D3D11_TEXTURE2D_DESC description{};
        description.Width = static_cast<UINT>(image_width);
        description.Height = static_cast<UINT>(image_height);
        description.MipLevels = 1;
        description.ArraySize = 1;
        description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        description.SampleDesc.Count = 1;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA resource{};
        resource.pSysMem = pixels.data();
        resource.SysMemPitch = static_cast<UINT>(image_width * 4);

        ID3D11Texture2D* texture = nullptr;
        if (FAILED(device->CreateTexture2D(&description, &resource, &texture)) || !texture)
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC view_description{};
        view_description.Format = description.Format;
        view_description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        view_description.Texture2D.MipLevels = 1;
        HRESULT result = device->CreateShaderResourceView(texture, &view_description, out_srv);
        texture->Release();
        if (FAILED(result) || !*out_srv)
            return false;

        if (out_width) *out_width = image_width;
        if (out_height) *out_height = image_height;
        if (out_uv0) *out_uv0 = uv0;
        if (out_uv1) *out_uv1 = uv1;
        return true;
    }

    enum class AssetSlot { LogoWide, LogoStack, WhiteLabelLogo, Character, Background, Profile, Dashboard, Aimbot, Movement, Visuals, Items, Render, Misc, Theme, Store, Test, Overlays, Settings, Alert, Hotkeys, Language, Dpi, Styles, Count };

    struct Asset
    {
        ID3D11ShaderResourceView* view = nullptr;
        ImVec2 size = ImVec2(0.0f, 0.0f);
        ImVec2 uv0 = ImVec2(0.0f, 0.0f);
        ImVec2 uv1 = ImVec2(1.0f, 1.0f);
    };
    std::array<Asset, static_cast<size_t>(AssetSlot::Count)> g_assets{};
    std::vector<std::uint8_t> g_white_label_logo{};
    ID3D11Device* g_device = nullptr;
    bool g_initialized = false;

    Asset& AssetAt(AssetSlot slot)
    {
        return g_assets[static_cast<size_t>(slot)];
    }

    bool LoadAsset(AssetSlot slot, const void* data, size_t data_size,
        bool force_white = false, bool chroma_key = false, bool trim_transparent = false)
    {
        Asset& asset = AssetAt(slot);
        if (asset.view || !g_device || !data || data_size == 0)
            return asset.view != nullptr;

        int width = 0;
        int height = 0;
        if (!LoadTextureFromMemoryProcessed(g_device, &asset.view, data, data_size, &width, &height,
            &asset.uv0, &asset.uv1, force_white, chroma_key, trim_transparent) || !asset.view)
            return false;

        asset.size = ImVec2(static_cast<float>(width), static_cast<float>(height));
        if (trim_transparent || chroma_key)
        {
            const ImVec2 extent(asset.uv1.x - asset.uv0.x, asset.uv1.y - asset.uv0.y);
            asset.size = ImVec2(asset.size.x * extent.x, asset.size.y * extent.y);
        }
        return true;
    }

    ImTextureID Texture(AssetSlot slot)
    {
        return reinterpret_cast<ImTextureID>(AssetAt(slot).view);
    }

    ImVec2 TextureSize(AssetSlot slot)
    {
        return AssetAt(slot).size;
    }

    void BindAssets()
    {
        auto texture = [](AssetSlot slot) -> const Asset& { return AssetAt(slot); };
        const Asset& wide = texture(AssetSlot::LogoWide);
        const Asset& stack = texture(AssetSlot::LogoStack);
        Blade::SetLogos(reinterpret_cast<ImTextureID>(wide.view), wide.size, reinterpret_cast<ImTextureID>(stack.view), stack.size);

        const Asset& white_label = texture(AssetSlot::WhiteLabelLogo);
        Blade::SetWhiteLabelLogo(reinterpret_cast<ImTextureID>(white_label.view), white_label.size, white_label.uv0, white_label.uv1);
        const Asset& character = texture(AssetSlot::Character);
        Blade::SetCharacter(reinterpret_cast<ImTextureID>(character.view), character.size, character.uv0, character.uv1);
        const Asset& profile = texture(AssetSlot::Profile);
        Blade::SetProfileTex(reinterpret_cast<ImTextureID>(profile.view), profile.size);

        auto set_tab = [&](int tab, AssetSlot slot) {
            const Asset& asset = texture(slot);
            Blade::SetTabTex(tab, reinterpret_cast<ImTextureID>(asset.view), asset.size);
        };
        set_tab(0, AssetSlot::Aimbot);
        set_tab(1, AssetSlot::Visuals);
        set_tab(2, AssetSlot::Items);
        Blade::SetTabTex(3, nullptr, ImVec2(0.0f, 0.0f));
        set_tab(4, AssetSlot::Settings);

        auto set_icon = [&](Blade::Icon icon, AssetSlot slot) {
            const Asset& asset = texture(slot);
            Blade::SetIconTex(static_cast<int>(icon), reinterpret_cast<ImTextureID>(asset.view), asset.size);
        };
        set_icon(Blade::IC_WARNING, AssetSlot::Alert);
        set_icon(Blade::IC_HOTKEY, AssetSlot::Hotkeys);
        set_icon(Blade::IC_LANG, AssetSlot::Language);
        set_icon(Blade::IC_DPI, AssetSlot::Dpi);
        set_icon(Blade::IC_STYLE, AssetSlot::Styles);
    }
}

bool BladeBridge::Initialize()
{
    if (g_initialized)
        return true;

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config{};
    config.FontDataOwnedByAtlas = false;
    const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
    io.Fonts->AddFontFromMemoryTTF((void*)MontserratRegular, sizeof(MontserratRegular), 15.0f, &config, ranges);
    Blade::LoadFonts(io, MontserratRegular, sizeof(MontserratRegular), MontserratMedium, sizeof(MontserratMedium), MontserratExtraBold, sizeof(MontserratExtraBold));
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    Blade::S.Reset();
    Blade::ArmSplash();

    g_initialized = true;
    return true;
}

bool BladeBridge::CreateDeviceObjects(ID3D11Device* device)
{
    if (!device)
        return false;

    ReleaseDeviceObjects();
    g_device = device;
    bool loaded = false;
    loaded |= LoadAsset(AssetSlot::LogoWide, ui_asset_logo_wide_png, sizeof(ui_asset_logo_wide_png));
    loaded |= LoadAsset(AssetSlot::LogoStack, ui_asset_logo_stack_png, sizeof(ui_asset_logo_stack_png));
    if (!g_white_label_logo.empty())
        loaded |= LoadAsset(AssetSlot::WhiteLabelLogo, g_white_label_logo.data(), g_white_label_logo.size(), false, false, true);
    loaded |= LoadAsset(AssetSlot::Character, ui_asset_character_png, sizeof(ui_asset_character_png), false, false, true);
    loaded |= LoadAsset(AssetSlot::Background, ui_asset_background_jpg, sizeof(ui_asset_background_jpg));
    loaded |= LoadAsset(AssetSlot::Profile, ui_asset_profile_jpg, sizeof(ui_asset_profile_jpg));
    loaded |= LoadAsset(AssetSlot::Dashboard, ui_asset_dashboard_png, sizeof(ui_asset_dashboard_png), true);
    loaded |= LoadAsset(AssetSlot::Aimbot, ui_asset_aimbot_png, sizeof(ui_asset_aimbot_png), true);
    loaded |= LoadAsset(AssetSlot::Movement, ui_asset_movement_png, sizeof(ui_asset_movement_png), true);
    loaded |= LoadAsset(AssetSlot::Visuals, ui_asset_visuals_png, sizeof(ui_asset_visuals_png), true);
    loaded |= LoadAsset(AssetSlot::Items, ui_asset_items_png, sizeof(ui_asset_items_png), true);
    loaded |= LoadAsset(AssetSlot::Render, ui_asset_render_png, sizeof(ui_asset_render_png), true);
    loaded |= LoadAsset(AssetSlot::Misc, ui_asset_misc_png, sizeof(ui_asset_misc_png), true);
    loaded |= LoadAsset(AssetSlot::Theme, ui_asset_theme_png, sizeof(ui_asset_theme_png), true);
    loaded |= LoadAsset(AssetSlot::Store, ui_asset_store_png, sizeof(ui_asset_store_png), true);
    loaded |= LoadAsset(AssetSlot::Test, ui_asset_teste_png, sizeof(ui_asset_teste_png), true);
    loaded |= LoadAsset(AssetSlot::Overlays, ui_asset_overlays_png, sizeof(ui_asset_overlays_png), true);
    loaded |= LoadAsset(AssetSlot::Settings, ui_asset_settings_png, sizeof(ui_asset_settings_png), true);
    loaded |= LoadAsset(AssetSlot::Alert, ui_asset_alert_png, sizeof(ui_asset_alert_png), true);
    loaded |= LoadAsset(AssetSlot::Hotkeys, ui_asset_hotkeys_png, sizeof(ui_asset_hotkeys_png), true);
    loaded |= LoadAsset(AssetSlot::Language, ui_asset_language_png, sizeof(ui_asset_language_png), true);
    loaded |= LoadAsset(AssetSlot::Dpi, ui_asset_dpi_png, sizeof(ui_asset_dpi_png), true);
    loaded |= LoadAsset(AssetSlot::Styles, ui_asset_styles_png, sizeof(ui_asset_styles_png), true);
    BindAssets();
    return loaded;
}
void BladeBridge::ReleaseDeviceObjects()
{
    for (Asset& asset : g_assets)
    {
        if (asset.view)
            asset.view->Release();
        asset = {};
    }
    g_device = nullptr;
}

void BladeBridge::ApplyWhiteLabel(const SharedTheme& theme)
{
    Settings::Get().ApplyWhiteLabelTheme(theme.Colors.uText, theme.Colors.uBg, theme.Colors.uElementsPrimary,
        theme.Colors.uElementsSecondary, theme.m_strName, theme.nId);
    g_white_label_logo.clear();
    if (theme.pImageData && theme.szImageSize)
        g_white_label_logo.assign(theme.pImageData, theme.pImageData + theme.szImageSize);
}

void BladeBridge::SetSession(const char* user, const char* time_left)
{
    // pstra monta na stack - snprintf copia na mesma expressao, safe
    const char* safe_user = user && user[0] ? user : pstra("Project Oficial" );
    const char* safe_time = time_left && time_left[0] ? time_left : "-";
    std::snprintf(Blade::S.session_user, sizeof(Blade::S.session_user), "%s", safe_user);
    std::snprintf(Blade::S.session_until, sizeof(Blade::S.session_until), pstra("Till: %s"), safe_time);
}

void BladeBridge::Render()
{
    // Fallback for mappers that bypassed CRT initialization before InitMain ran.
    Settings::Get().EnsureInterfaceDefaults();
    if (Blade::S.wm_order[1] == 0 && Blade::S.wm_order[2] == 0 && Blade::S.wm_order[3] == 0)
    {
        Blade::S.Reset();
    }

    if (!Initialize())
        return;


    Blade::UpdatePalette();
    Blade::BeginScale();
    ImGuiIO& io = ImGui::GetIO();
    const float scale = Blade::Scale();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x / ImMin(scale, 1.0f), io.DisplaySize.y / ImMin(scale, 1.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(pstra("##arcr_blade_overlay"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* foreground = ImGui::GetForegroundDrawList();
    const int draw_vtx = draw_list->VtxBuffer.Size;
    const int draw_cmd = draw_list->CmdBuffer.Size;
    const int foreground_vtx = foreground->VtxBuffer.Size;
    const int foreground_cmd = foreground->CmdBuffer.Size;
    draw_list->PushClipRect(ImVec2(-100000.0f, -100000.0f), ImVec2(100000.0f, 100000.0f), false);
    foreground->PushClipRect(ImVec2(-100000.0f, -100000.0f), ImVec2(100000.0f, 100000.0f), false);
    Blade::RenderPopups();
    Blade::BeginShadowLayer(draw_list);
    if (Blade::S.menu_open)
    {
        Blade::DrawMainMenu(draw_list, 1.0f);
    }
    Blade::DrawWatermark(draw_list);
    Blade::DrawTargetHud(draw_list);
    Blade::DrawNotifications(draw_list);
    Blade::DrawCoords(draw_list);
    Blade::EndShadowLayer(draw_list);
    Blade::RenderTooltip(foreground);
    draw_list->PopClipRect();
    foreground->PopClipRect();
    Blade::ScaleDrawList(draw_list, draw_vtx, draw_cmd);
    Blade::ScaleDrawList(foreground, foreground_vtx, foreground_cmd);
    ImGui::End();
    ImGui::PopStyleVar();
    Blade::EndScale();
}
