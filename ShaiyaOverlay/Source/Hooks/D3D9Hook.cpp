#include "D3D9Hook.h"
#include "WndProcHook.h"
#include "Core/Memory.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include "UI/Renderer.h"
#include "UI/Menu.h"

namespace ShaiyaOverlay
{
    void** D3D9Hook::DeviceVTable = nullptr;
    EndSceneFn D3D9Hook::OriginalEndScene = nullptr;
    ResetFn D3D9Hook::OriginalReset = nullptr;

    IDirect3DDevice9* D3D9Hook::Device = nullptr;
    bool D3D9Hook::IsInitialized = false;
    bool D3D9Hook::IsHooked = false;

    bool D3D9Hook::Initialize()
    {
        if (IsHooked)
            return true;

        U64 ImageBase = Memory::GetModuleBase(nullptr);
        Logger::Info("D3D9Hook: Initializing... ImageBase: 0x%llX", ImageBase);

        // First attempt: read game's active D3D9 device pointer from memory
        U64 DevicePtr = 0;
        if (Memory::ReadSafe(Offsets.D3DDevice, &DevicePtr) && DevicePtr)
        {
            Device = reinterpret_cast<IDirect3DDevice9*>(DevicePtr);
            DeviceVTable = *reinterpret_cast<void***>(Device);
            Logger::Info("D3D9Hook: Located game D3D9 device: 0x%p, VTable: 0x%p", Device, DeviceVTable);
        }

        // Second attempt: create dummy device to retrieve class VTable if not in memory yet
        if (!DeviceVTable)
        {
            Logger::Info("D3D9Hook: Device not found in globals, creating dummy device...");
            IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
            if (pD3D)
            {
                WNDCLASSA Wc = { 0 };
                Wc.lpfnWndProc = DefWindowProcA;
                Wc.hInstance = GetModuleHandleA(nullptr);
                Wc.lpszClassName = "ShaiyaDummyD3D9Window";
                RegisterClassA(&Wc);

                HWND DummyHwnd = CreateWindowA(
                    "ShaiyaDummyD3D9Window",
                    "Dummy",
                    WS_OVERLAPPEDWINDOW,
                    0, 0, 100, 100,
                    nullptr, nullptr,
                    Wc.hInstance,
                    nullptr
                );

                if (DummyHwnd)
                {
                    D3DPRESENT_PARAMETERS D3dpp = { 0 };
                    D3dpp.Windowed = TRUE;
                    D3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                    D3dpp.hDeviceWindow = DummyHwnd;

                    IDirect3DDevice9* DummyDevice = nullptr;
                    HRESULT Hr = pD3D->CreateDevice(
                        D3DADAPTER_DEFAULT,
                        D3DDEVTYPE_HAL,
                        DummyHwnd,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                        &D3dpp,
                        &DummyDevice
                    );

                    if (SUCCEEDED(Hr) && DummyDevice)
                    {
                        DeviceVTable = *reinterpret_cast<void***>(DummyDevice);
                        DummyDevice->Release();
                        Logger::Info("D3D9Hook: Retrieved VTable from dummy device: 0x%p", DeviceVTable);
                    }

                    DestroyWindow(DummyHwnd);
                    UnregisterClassA("ShaiyaDummyD3D9Window", Wc.hInstance);
                }
                pD3D->Release();
            }
        }

        if (!DeviceVTable)
        {
            Logger::Error("D3D9Hook: Failed to acquire IDirect3DDevice9 VTable!");
            return false;
        }

        bool Hook1 = Memory::HookVTableFunction(
            DeviceVTable,
            42,
            reinterpret_cast<void*>(HookedEndScene),
            reinterpret_cast<void**>(&OriginalEndScene)
        );

        bool Hook2 = Memory::HookVTableFunction(
            DeviceVTable,
            16,
            reinterpret_cast<void*>(HookedReset),
            reinterpret_cast<void**>(&OriginalReset)
        );

        IsHooked = Hook1 && Hook2;
        Logger::Info("D3D9Hook: Hook applied! EndScene hooked: %d, Reset hooked: %d", Hook1, Hook2);
        return IsHooked;
    }

    void D3D9Hook::Uninitialize()
    {
        Logger::Info("D3D9Hook: Uninitializing...");

        if (IsHooked && DeviceVTable)
        {
            if (OriginalEndScene)
            {
                Memory::RestoreVTableFunction(DeviceVTable, 42, reinterpret_cast<void*>(OriginalEndScene));
                OriginalEndScene = nullptr;
            }

            if (OriginalReset)
            {
                Memory::RestoreVTableFunction(DeviceVTable, 16, reinterpret_cast<void*>(OriginalReset));
                OriginalReset = nullptr;
            }

            IsHooked = false;
        }

        Sleep(100);
        Renderer::Uninitialize();

        Device = nullptr;
        IsInitialized = false;
        Logger::Info("D3D9Hook: Uninitialized successfully.");
    }

    HRESULT __fastcall D3D9Hook::HookedEndScene(IDirect3DDevice9* InDevice)
    {
        if (!IsInitialized && InDevice)
        {
            Device = InDevice;
            D3DDEVICE_CREATION_PARAMETERS Params = { 0 };
            InDevice->GetCreationParameters(&Params);

            HWND TargetHwnd = Params.hFocusWindow;
            if (!TargetHwnd)
                TargetHwnd = WndProcHook::GetWindowHandle();

            Logger::Info("D3D9 HookedEndScene: First frame! Device: 0x%p, Window: 0x%p", InDevice, TargetHwnd);

            if (Renderer::InitializeD3D9(TargetHwnd, InDevice))
            {
                IsInitialized = true;
                Logger::Info("D3D9 HookedEndScene: Renderer initialized successfully.");
            }
            else
            {
                Logger::Error("D3D9 HookedEndScene: Failed to initialize renderer.");
            }
        }

        if (IsInitialized)
        {
            Renderer::NewFrame();
            Menu::Render();
            Renderer::RenderDrawData();
        }

        return OriginalEndScene(InDevice);
    }

    HRESULT __fastcall D3D9Hook::HookedReset(IDirect3DDevice9* InDevice, D3DPRESENT_PARAMETERS* Params)
    {
        Logger::Info("D3D9 HookedReset: Resetting device objects.");

        if (IsInitialized)
            Renderer::PreResetD3D9();

        HRESULT Result = OriginalReset(InDevice, Params);

        if (IsInitialized && SUCCEEDED(Result))
            Renderer::PostResetD3D9();

        return Result;
    }
}
