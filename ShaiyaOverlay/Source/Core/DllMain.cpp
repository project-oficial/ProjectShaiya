#include "Types.h"
#include "Logger.h"
#include "Hooks/HookManager.h"
#include "Hooks/WndProcHook.h"
#include "Game/GameOffsets.h"
#include "Game/Login/AutoLoginManager.h"
#include "Core/MCPTool/MCPBridge.h"

namespace ShaiyaOverlay
{
    static HMODULE ModuleHandle = nullptr;

    static DWORD WINAPI MainThread(LPVOID Parameter)
    {
        Logger::Initialize();
        Logger::Info("Shaiya Hardcore Overlay loaded into process PID: %lu", GetCurrentProcessId());

        Sleep(500);

        if (!GameOffsets::Initialize())
        {
            Logger::Error("GameOffsets: Initialization failed! AOB pattern scan missed required offsets. Auto-ejecting DLL...");
            Logger::Uninitialize();
            Sleep(200);
            FreeLibraryAndExitThread(ModuleHandle, 0);
            return 1;
        }

        if (!HookManager::Initialize())
        {
            Logger::Error("Initialization failed! Auto-ejecting DLL to prevent hanging...");
            HookManager::Uninitialize();
            Logger::Uninitialize();
            Sleep(200);
            FreeLibraryAndExitThread(ModuleHandle, 0);
            return 1;
        }

        Logger::Info("Overlay active! Press [INSERT] to toggle UI, [END] to unload.");

        AutoLoginManager::Initialize();

#ifdef MCP_TOOL
        MCPBridge::StartServer();
#endif

        while (!WndProcHook::ShouldUnload())
        {
            // Global key polling fallback only for emergency unload if game window lost focus
            if (GetAsyncKeyState(VK_END) & 0x8000)
            {
                Logger::Info("VK_END detected via GetAsyncKeyState. Triggering unload...");
                break;
            }

            Sleep(50);
        }

        Logger::Info("Unload requested. Cleaning up resources...");

        AutoLoginManager::Shutdown();

        WndProcHook::RequestUnload();

#ifdef MCP_TOOL
        MCPBridge::StopServer();
#endif

        HookManager::Uninitialize();
        Logger::Info("Shutdown complete. Ejecting DLL...");
        Logger::Uninitialize();

        Sleep(300);

        FreeLibraryAndExitThread(ModuleHandle, 0);
        return 0;
    }
}

BOOL WINAPI DllMain(HINSTANCE Instance, DWORD Reason, LPVOID Reserved)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        ShaiyaOverlay::ModuleHandle = Instance;
        DisableThreadLibraryCalls(Instance);

        HANDLE Thread = CreateThread(
            nullptr,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(ShaiyaOverlay::MainThread),
            Instance,
            0,
            nullptr
        );

        if (Thread)
        {
            CloseHandle(Thread);
        }
    }

    return TRUE;
}
