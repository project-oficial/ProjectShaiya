#include "D3D11Hook.h"
#include "WndProcHook.h"
#include "Core/Memory.h"
#include "Core/Logger.h"
#include "UI/Renderer.h"
#include "UI/Menu.h"
#include "Interface/Blade/BladeBridge.h"
#include "Security/ProtectMacro.h"

namespace ShaiyaOverlay
{
    void** D3D11Hook::SwapChainVTable = nullptr;
    PresentFn D3D11Hook::OriginalPresent = nullptr;
    ResizeBuffersFn D3D11Hook::OriginalResizeBuffers = nullptr;

    ID3D11Device* D3D11Hook::Device = nullptr;
    ID3D11DeviceContext* D3D11Hook::Context = nullptr;
    ID3D11RenderTargetView* D3D11Hook::RenderTargetView = nullptr;
    bool D3D11Hook::IsInitialized = false;
    bool D3D11Hook::IsHooked = false;
    volatile LONG D3D11Hook::ActivePresentCalls = 0;

    bool D3D11Hook::CreateRenderTarget(IDXGISwapChain* SwapChain)
    {
        if (!SwapChain || !Device)
            return false;

        ID3D11Texture2D* BackBuffer = nullptr;
        HRESULT Hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer));
        if (FAILED(Hr) || !BackBuffer)
            return false;

        Hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
        BackBuffer->Release();

        return SUCCEEDED(Hr);
    }

    void D3D11Hook::CleanupRenderTarget()
    {
        if (RenderTargetView)
        {
            RenderTargetView->Release();
            RenderTargetView = nullptr;
        }
    }

    bool D3D11Hook::Initialize()
    {
        if (IsHooked)
            return true;

        Logger::Info("D3D11Hook: Initializing...");

        WNDCLASSA Wc = { 0 };
        Wc.lpfnWndProc = DefWindowProcA;
        Wc.hInstance = GetModuleHandleA(nullptr);
        Wc.lpszClassName = "ShaiyaDummyD3D11Window";

        RegisterClassA(&Wc);

        HWND DummyHwnd = CreateWindowA(
            "ShaiyaDummyD3D11Window",
            "Dummy",
            WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100,
            nullptr, nullptr,
            Wc.hInstance,
            nullptr
        );

        if (!DummyHwnd)
        {
            Logger::Error("D3D11Hook: Failed to create dummy window.");
            return false;
        }

        DXGI_SWAP_CHAIN_DESC ScDesc = { 0 };
        ScDesc.BufferCount = 1;
        ScDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        ScDesc.BufferDesc.Width = 100;
        ScDesc.BufferDesc.Height = 100;
        ScDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        ScDesc.OutputWindow = DummyHwnd;
        ScDesc.SampleDesc.Count = 1;
        ScDesc.Windowed = TRUE;
        ScDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL FeatureLevel;
        const D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

        IDXGISwapChain* DummySwapChain = nullptr;
        ID3D11Device* DummyDevice = nullptr;
        ID3D11DeviceContext* DummyContext = nullptr;

        HRESULT Hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            FeatureLevels,
            2,
            D3D11_SDK_VERSION,
            &ScDesc,
            &DummySwapChain,
            &DummyDevice,
            &FeatureLevel,
            &DummyContext
        );

        if (FAILED(Hr))
        {
            Hr = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                0,
                FeatureLevels,
                2,
                D3D11_SDK_VERSION,
                &ScDesc,
                &DummySwapChain,
                &DummyDevice,
                &FeatureLevel,
                &DummyContext
            );
        }

        if (FAILED(Hr) || !DummySwapChain)
        {
            Logger::Error("D3D11Hook: Failed to create dummy DX11 device & swapchain.");
            DestroyWindow(DummyHwnd);
            UnregisterClassA("ShaiyaDummyD3D11Window", Wc.hInstance);
            return false;
        }

        SwapChainVTable = *reinterpret_cast<void***>(DummySwapChain);
        Logger::Info("D3D11Hook: Retrieved SwapChain VTable: 0x%p", SwapChainVTable);

        // Hook Present (index 8) and ResizeBuffers (index 13)
        bool Hook1 = Memory::HookVTableFunction(SwapChainVTable, 8, reinterpret_cast<void*>(HookedPresent), reinterpret_cast<void**>(&OriginalPresent));
        bool Hook2 = Memory::HookVTableFunction(SwapChainVTable, 13, reinterpret_cast<void*>(HookedResizeBuffers), reinterpret_cast<void**>(&OriginalResizeBuffers));

        DummySwapChain->Release();
        DummyContext->Release();
        DummyDevice->Release();
        DestroyWindow(DummyHwnd);
        UnregisterClassA("ShaiyaDummyD3D11Window", Wc.hInstance);

        IsHooked = Hook1 && Hook2;
        Logger::Info("D3D11Hook: Hook applied! Present: %d, ResizeBuffers: %d", Hook1, Hook2);
        return IsHooked;
    }

    void D3D11Hook::Uninitialize()
    {
        Logger::Info("D3D11Hook: Uninitializing...");

        if (IsHooked && SwapChainVTable)
        {
            if (OriginalPresent)
            {
                Memory::RestoreVTableFunction(SwapChainVTable, 8, reinterpret_cast<void*>(OriginalPresent));
            }

            if (OriginalResizeBuffers)
            {
                Memory::RestoreVTableFunction(SwapChainVTable, 13, reinterpret_cast<void*>(OriginalResizeBuffers));
            }

            IsHooked = false;
        }

        // Wait for any active HookedPresent frames to complete
        DWORD start = GetTickCount();
        while (ActivePresentCalls > 0 && (GetTickCount() - start) < 1000)
        {
            Sleep(10);
        }

        // Wait several video frames for engine thread to execute cleanly on restored VTable
        Sleep(150);

        Renderer::Uninitialize();
        CleanupRenderTarget();

        if (Context)
        {
            Context->Release();
            Context = nullptr;
        }

        if (Device)
        {
            Device->Release();
            Device = nullptr;
        }

        IsInitialized = false;
        Logger::Info("D3D11Hook: Uninitialized successfully.");
    }

    HRESULT __fastcall D3D11Hook::HookedPresent(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags)
    {
        InterlockedIncrement(&ActivePresentCalls);
        PresentFn Orig = OriginalPresent;

        if (!WndProcHook::ShouldUnload())
        {
            if (!IsInitialized && SwapChain)
            {
                if (SUCCEEDED(SwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&Device))))
                {
                    Device->GetImmediateContext(&Context);

                    DXGI_SWAP_CHAIN_DESC Desc;
                    SwapChain->GetDesc(&Desc);

                    CreateRenderTarget(SwapChain);
                    Logger::Info("D3D11 HookedPresent: First frame! OutputWindow: 0x%p", Desc.OutputWindow);
                    WndProcHook::AttachToWindow(Desc.OutputWindow);

                    if (Renderer::InitializeD3D11(Desc.OutputWindow, Device, Context))
                    {
                        IsInitialized = true;
                        BladeBridge::Initialize();
                        BladeBridge::CreateDeviceObjects(Device);
                        if (s_info.m_theme_ptr)
                        {
                            const auto* theme = reinterpret_cast<const SharedTheme*>(s_info.m_theme_ptr);
                            if (theme && !IsBadReadPtr(theme, sizeof(SharedTheme)) && theme->nId != 0)
                            {
                                BladeBridge::ApplyWhiteLabel(*theme);
                            }
                        }
                        Logger::Info("D3D11 HookedPresent: Renderer initialized successfully.");
                    }
                    else
                    {
                        Logger::Error("D3D11 HookedPresent: Failed to initialize renderer.");
                    }
                }
            }

            if (IsInitialized && Context && RenderTargetView)
            {
                Renderer::NewFrame();
                Menu::Render();
                Context->OMSetRenderTargets(1, &RenderTargetView, nullptr);
                Renderer::RenderDrawData();
            }
        }

        HRESULT hr = Orig ? Orig(SwapChain, SyncInterval, Flags) : S_OK;
        InterlockedDecrement(&ActivePresentCalls);
        return hr;
    }

    HRESULT __fastcall D3D11Hook::HookedResizeBuffers(IDXGISwapChain* SwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
    {
        Logger::Info("D3D11 HookedResizeBuffers invoked.");
        CleanupRenderTarget();
        HRESULT Result = OriginalResizeBuffers(SwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        CreateRenderTarget(SwapChain);
        return Result;
    }
}
