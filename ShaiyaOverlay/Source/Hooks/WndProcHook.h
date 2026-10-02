#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class WndProcHook
    {
    public:
        static bool Initialize();
        static void Uninitialize();

        static bool AttachToWindow(HWND Hwnd);
        static bool IsAttached() { return OriginalWndProc != nullptr; }

        static HWND GetWindowHandle() { return WindowHandle; }
        static bool IsMenuOpen() { return MenuOpen; }
        static void SetMenuOpen(bool Open) { MenuOpen = Open; }
        static void ToggleMenu() { MenuOpen = !MenuOpen; }
        static bool ShouldUnload() { return UnloadRequested; }
        static void RequestUnload() { UnloadRequested = true; }

    private:
        static LRESULT CALLBACK HookedWndProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
        static BOOL CALLBACK EnumWindowsCallback(HWND Hwnd, LPARAM LParam);

        static HWND WindowHandle;
        static WNDPROC OriginalWndProc;
        static bool MenuOpen;
        static bool UnloadRequested;
    };
}
