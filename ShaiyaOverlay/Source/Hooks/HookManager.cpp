#include "HookManager.h"
#include "D3D9Hook.h"
#include "D3D11Hook.h"
#include "WndProcHook.h"
#include "Core/Logger.h"

namespace ShaiyaOverlay
{
    bool HookManager::Active = false;

    bool HookManager::Initialize()
    {
        if (Active)
            return true;

        Logger::Info("HookManager: Initializing hooks...");

        // WndProc hook (non-fatal if it fails; GetAsyncKeyState will still handle keys)
        if (!WndProcHook::Initialize())
        {
            Logger::Error("HookManager: Warning - WndProcHook failed to initialize. Hotkeys will use GetAsyncKeyState fallback.");
        }
        else
        {
            Logger::Info("HookManager: WndProcHook initialized.");
        }

        // Initialize graphics hooks (try D3D11 first; only fallback to D3D9 if D3D11 is not available)
        bool D3D11Ok = D3D11Hook::Initialize();
        bool D3D9Ok = false;
        if (!D3D11Ok)
        {
            D3D9Ok = D3D9Hook::Initialize();
        }

        if (!D3D9Ok && !D3D11Ok)
        {
            Logger::Error("HookManager: Neither D3D9 nor D3D11 hook succeeded!");
            WndProcHook::Uninitialize();
            return false;
        }

        Active = true;
        Logger::Info("HookManager: Active successfully. (D3D9: %d, D3D11: %d)", D3D9Ok, D3D11Ok);
        return true;
    }

    void HookManager::Uninitialize()
    {
        if (!Active)
            return;

        Logger::Info("HookManager: Uninitializing hooks...");
        WndProcHook::Uninitialize();
        D3D9Hook::Uninitialize();
        D3D11Hook::Uninitialize();
        Active = false;
        Logger::Info("HookManager: All hooks removed.");
    }
}
