#include "Renderer.h"
#include "Core/Logger.h"
#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_impl_win32.h"
#include "ThirdParty/ImGui/imgui_impl_dx9.h"
#include "ThirdParty/ImGui/imgui_impl_dx11.h"

namespace ShaiyaOverlay
{
    RenderApi Renderer::CurrentApi = RenderApi::None;

    void Renderer::SetupImGuiStyle()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& Io = ImGui::GetIO();
        Io.IniFilename = nullptr;
        Io.LogFilename = nullptr;

        ImGui::StyleColorsDark();
        ImGuiStyle& Style = ImGui::GetStyle();
        Style.WindowRounding = 6.0f;
        Style.FrameRounding = 4.0f;
        Style.PopupRounding = 4.0f;
        Style.ScrollbarRounding = 4.0f;
        Style.GrabRounding = 4.0f;
        Style.TabRounding = 4.0f;
        Style.WindowBorderSize = 1.0f;
    }

    bool Renderer::InitializeD3D9(HWND WindowHandle, IDirect3DDevice9* Device)
    {
        if (CurrentApi != RenderApi::None)
            return true;

        if (!WindowHandle || !Device)
            return false;

        SetupImGuiStyle();

        if (!ImGui_ImplWin32_Init(WindowHandle))
        {
            ImGui::DestroyContext();
            return false;
        }

        if (!ImGui_ImplDX9_Init(Device))
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        CurrentApi = RenderApi::D3D9;
        Logger::Info("Renderer: Initialized with DirectX 9 backend.");
        return true;
    }

    bool Renderer::InitializeD3D11(HWND WindowHandle, ID3D11Device* Device, ID3D11DeviceContext* Context)
    {
        if (CurrentApi != RenderApi::None)
            return true;

        if (!WindowHandle || !Device || !Context)
            return false;

        SetupImGuiStyle();

        if (!ImGui_ImplWin32_Init(WindowHandle))
        {
            ImGui::DestroyContext();
            return false;
        }

        if (!ImGui_ImplDX11_Init(Device, Context))
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        CurrentApi = RenderApi::D3D11;
        Logger::Info("Renderer: Initialized with DirectX 11 backend.");
        return true;
    }

    void Renderer::Uninitialize()
    {
        if (CurrentApi == RenderApi::None)
            return;

        if (CurrentApi == RenderApi::D3D9)
            ImGui_ImplDX9_Shutdown();
        else if (CurrentApi == RenderApi::D3D11)
            ImGui_ImplDX11_Shutdown();

        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CurrentApi = RenderApi::None;
        Logger::Info("Renderer: Uninitialized successfully.");
    }

    void Renderer::NewFrame()
    {
        if (CurrentApi == RenderApi::D3D9)
            ImGui_ImplDX9_NewFrame();
        else if (CurrentApi == RenderApi::D3D11)
            ImGui_ImplDX11_NewFrame();
        else
            return;

        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::RenderDrawData()
    {
        if (CurrentApi == RenderApi::None)
            return;

        ImGui::Render();

        if (CurrentApi == RenderApi::D3D9)
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        else if (CurrentApi == RenderApi::D3D11)
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Renderer::PreResetD3D9()
    {
        if (CurrentApi == RenderApi::D3D9)
            ImGui_ImplDX9_InvalidateDeviceObjects();
    }

    void Renderer::PostResetD3D9()
    {
        if (CurrentApi == RenderApi::D3D9)
            ImGui_ImplDX9_CreateDeviceObjects();
    }
}
