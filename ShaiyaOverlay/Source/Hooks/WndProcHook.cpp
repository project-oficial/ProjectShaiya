#include "WndProcHook.h"
#include "Core/Memory.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include <windows.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ShaiyaOverlay
{
    HWND WndProcHook::WindowHandle = nullptr;
    WNDPROC WndProcHook::OriginalWndProc = nullptr;
    bool WndProcHook::MenuOpen = false;
    bool WndProcHook::UnloadRequested = false;

    BOOL CALLBACK WndProcHook::EnumWindowsCallback(HWND Hwnd, LPARAM LParam)
    {
        DWORD ProcessId = 0;
        GetWindowThreadProcessId(Hwnd, &ProcessId);

        if (ProcessId == GetCurrentProcessId())
        {
            char ClassName[256] = { 0 };
            GetClassNameA(Hwnd, ClassName, sizeof(ClassName));

            // Skip console and tool helper windows
            if (lstrcmpA(ClassName, "ConsoleWindowClass") == 0 ||
                lstrcmpA(ClassName, "ShaiyaDummyWindow") == 0 ||
                lstrcmpA(ClassName, "ShaiyaDummyD3D9Window") == 0 ||
                lstrcmpA(ClassName, "ShaiyaDummyD3D11Window") == 0)
            {
                return TRUE;
            }

            if (GetWindow(Hwnd, GW_OWNER) == nullptr && IsWindowVisible(Hwnd))
            {
                *reinterpret_cast<HWND*>(LParam) = Hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }

    bool WndProcHook::Initialize()
    {
        if (OriginalWndProc != nullptr)
            return true;

        WindowHandle = nullptr;

        // Method 1: Find SDL window directly
        WindowHandle = FindWindowA("SDL_app", nullptr);
        if (WindowHandle)
        {
            Logger::Info("WndProcHook: Found window via FindWindowA('SDL_app'): 0x%p", WindowHandle);
        }

        // Method 2: Read from game global memory
        if (!WindowHandle)
        {
            U64 HwndVal = 0;
            if (Memory::ReadSafe(Offsets.GameHwnd, &HwndVal) && HwndVal)
            {
                WindowHandle = reinterpret_cast<HWND>(HwndVal);
                Logger::Info("WndProcHook: Found window via GameHwnd: 0x%p", WindowHandle);
            }
        }

        // Method 3: EnumWindows fallback
        if (!WindowHandle)
        {
            EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&WindowHandle));
            if (WindowHandle)
                Logger::Info("WndProcHook: Found window via EnumWindows: 0x%p", WindowHandle);
        }

        if (!WindowHandle)
        {
            WindowHandle = GetActiveWindow();
            if (WindowHandle)
                Logger::Info("WndProcHook: Found window via GetActiveWindow: 0x%p", WindowHandle);
        }

        if (!WindowHandle)
        {
            Logger::Error("WndProcHook: Could not locate game window handle!");
            return false;
        }

        SetLastError(0);
        if (IsWindowUnicode(WindowHandle))
        {
            OriginalWndProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc))
            );
        }
        else
        {
            OriginalWndProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrA(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc))
            );
        }

        DWORD Err = GetLastError();
        if (!OriginalWndProc && Err != 0)
        {
            Logger::Error("WndProcHook: SetWindowLongPtr failed on HWND 0x%p with error code: %lu", WindowHandle, Err);
            return false;
        }

        Logger::Info("WndProcHook: Subclassed HWND 0x%p successfully. OriginalWndProc: 0x%p", WindowHandle, OriginalWndProc);
        return true;
    }

    void WndProcHook::Uninitialize()
    {
        if (OriginalWndProc && WindowHandle)
        {
            if (IsWindowUnicode(WindowHandle))
                SetWindowLongPtrW(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OriginalWndProc));
            else
                SetWindowLongPtrA(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OriginalWndProc));

            OriginalWndProc = nullptr;
            Logger::Info("WndProcHook: Restored original WndProc.");
        }
    }

    LRESULT CALLBACK WndProcHook::HookedWndProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
    {
        if (Msg == WM_KEYDOWN)
        {
            if (WParam == VK_INSERT || WParam == VK_END)
                return 0;
        }
        else if (Msg == WM_KEYUP)
        {
            if (WParam == VK_INSERT)
            {
                MenuOpen = !MenuOpen;
                Logger::Info("WndProcHook: Menu toggled via INSERT. State: %s", MenuOpen ? "OPEN" : "CLOSED");
                return 0;
            }
            else if (WParam == VK_END)
            {
                UnloadRequested = true;
                Logger::Info("WndProcHook: Unload requested via END.");
                return 0;
            }
        }

        if (MenuOpen)
        {
            if (ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam))
                return TRUE;
        }

        if (OriginalWndProc)
        {
            if (IsWindowUnicode(Hwnd))
                return CallWindowProcW(OriginalWndProc, Hwnd, Msg, WParam, LParam);
            else
                return CallWindowProcA(OriginalWndProc, Hwnd, Msg, WParam, LParam);
        }

        return DefWindowProcA(Hwnd, Msg, WParam, LParam);
    }
}
