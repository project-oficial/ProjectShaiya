#pragma once

#include "Core/Types.h"
#include <d3d11.h>
#include <dxgi.h>

namespace ShaiyaOverlay
{
    using PresentFn = HRESULT(__fastcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(__fastcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    class D3D11Hook
    {
    public:
        static bool Initialize();
        static void Uninitialize();

        static ID3D11Device* GetDevice() { return Device; }
        static ID3D11DeviceContext* GetContext() { return Context; }
        static ID3D11RenderTargetView* GetRenderTargetView() { return RenderTargetView; }

    private:
        static HRESULT __fastcall HookedPresent(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT __fastcall HookedResizeBuffers(IDXGISwapChain* SwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

        static bool CreateRenderTarget(IDXGISwapChain* SwapChain);
        static void CleanupRenderTarget();

        static void** SwapChainVTable;
        static PresentFn OriginalPresent;
        static ResizeBuffersFn OriginalResizeBuffers;

        static ID3D11Device* Device;
        static ID3D11DeviceContext* Context;
        static ID3D11RenderTargetView* RenderTargetView;
        static bool IsInitialized;
        static bool IsHooked;
        static volatile LONG ActivePresentCalls;
    };
}
