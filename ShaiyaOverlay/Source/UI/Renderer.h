#pragma once

#include "Core/Types.h"
#include <d3d9.h>
#include <d3d11.h>

namespace ShaiyaOverlay
{
    enum class RenderApi : U8
    {
        None = 0,
        D3D9,
        D3D11
    };

    class Renderer
    {
    public:
        static bool InitializeD3D9(HWND WindowHandle, IDirect3DDevice9* Device);
        static bool InitializeD3D11(HWND WindowHandle, ID3D11Device* Device, ID3D11DeviceContext* Context);
        static void Uninitialize();

        static void NewFrame();
        static void RenderDrawData();

        static void PreResetD3D9();
        static void PostResetD3D9();

        static bool IsReady() { return CurrentApi != RenderApi::None; }
        static RenderApi GetActiveApi() { return CurrentApi; }

    private:
        static void SetupImGuiStyle();

        static RenderApi CurrentApi;
    };
}
