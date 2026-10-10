#include "WndProcHook.h"
#include "Core/Memory.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"
#include "Game/Combat/ComboManager.h"
#include <windows.h>
#include <imgui.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ShaiyaOverlay
{
    HWND WndProcHook::WindowHandle = nullptr;
    WNDPROC WndProcHook::OriginalWndProc = nullptr;
    bool WndProcHook::MenuOpen = false;
    bool WndProcHook::UnloadRequested = false;
    volatile LONG WndProcHook::ActiveWndProcCalls = 0;

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

    bool WndProcHook::AttachToWindow(HWND Hwnd)
    {
        if (!Hwnd)
            return false;

        DWORD WindowPid = 0;
        GetWindowThreadProcessId(Hwnd, &WindowPid);
        if (WindowPid != GetCurrentProcessId())
            return false;

        if (WindowHandle == Hwnd && OriginalWndProc != nullptr)
            return true;

        if (OriginalWndProc != nullptr && WindowHandle != nullptr)
        {
            Uninitialize();
        }

        WindowHandle = Hwnd;
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
            WindowHandle = nullptr;
            return false;
        }

        Logger::Info("WndProcHook: Subclassed HWND 0x%p successfully. OriginalWndProc: 0x%p", WindowHandle, OriginalWndProc);
        return true;
    }

    bool WndProcHook::Initialize()
    {
        if (OriginalWndProc != nullptr)
            return true;

        WindowHandle = nullptr;

        // Priority 1: Read from game global memory (verified to match current PID)
        if (Offsets.GameHwnd)
        {
            U64 HwndVal = 0;
            if (Memory::ReadSafe(Offsets.GameHwnd, &HwndVal) && HwndVal)
            {
                HWND Candidate = reinterpret_cast<HWND>(HwndVal);
                if (AttachToWindow(Candidate))
                    return true;
            }
        }

        // Priority 2: EnumWindows strictly for current process
        HWND FoundHwnd = nullptr;
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&FoundHwnd));
        if (FoundHwnd && AttachToWindow(FoundHwnd))
            return true;

        // Priority 3: Active Window
        HWND ActiveHwnd = GetActiveWindow();
        if (ActiveHwnd && AttachToWindow(ActiveHwnd))
            return true;

        Logger::Error("WndProcHook: Could not locate game window handle during Init (will attach on first Present).");
        return false;
    }

    void WndProcHook::Uninitialize()
    {
        if (OriginalWndProc && WindowHandle)
        {
            WNDPROC Orig = OriginalWndProc;
            if (IsWindowUnicode(WindowHandle))
                SetWindowLongPtrW(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Orig));
            else
                SetWindowLongPtrA(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Orig));

            DWORD start = GetTickCount();
            while (ActiveWndProcCalls > 0 && (GetTickCount() - start) < 500)
            {
                Sleep(10);
            }

            Logger::Info("WndProcHook: Restored original WndProc.");
        }
    }

    LRESULT CALLBACK WndProcHook::HookedWndProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
    {
        InterlockedIncrement(&ActiveWndProcCalls);
        WNDPROC Orig = OriginalWndProc;

        if (UnloadRequested)
        {
            LRESULT res = Orig ? (IsWindowUnicode(Hwnd) ? CallWindowProcW(Orig, Hwnd, Msg, WParam, LParam) : CallWindowProcA(Orig, Hwnd, Msg, WParam, LParam)) : DefWindowProcA(Hwnd, Msg, WParam, LParam);
            InterlockedDecrement(&ActiveWndProcCalls);
            return res;
        }

        if (Msg == WM_KEYDOWN)
        {
            if (WParam == VK_INSERT || WParam == VK_END)
            {
                InterlockedDecrement(&ActiveWndProcCalls);
                return 0;
            }
        }
        else if (Msg == WM_KEYUP)
        {
            if (WParam == VK_INSERT)
            {
                static DWORD LastToggleTick = 0;
                DWORD Now = GetTickCount();
                if (Now - LastToggleTick > 150)
                {
                    MenuOpen = !MenuOpen;
                    LastToggleTick = Now;
                    Logger::Info("WndProcHook: Menu toggled via INSERT. State: %s", MenuOpen ? "OPEN" : "CLOSED");
                }
                InterlockedDecrement(&ActiveWndProcCalls);
                return 0;
            }
            else if (WParam == VK_END)
            {
                UnloadRequested = true;
                Logger::Info("WndProcHook: Unload requested via END.");
                InterlockedDecrement(&ActiveWndProcCalls);
                return 0;
            }
        }

        if (MenuOpen)
        {
            ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam);

            switch (Msg)
            {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                InterlockedDecrement(&ActiveWndProcCalls);
                return 0;
            }

            if (ImGui::GetCurrentContext())
            {
                ImGuiIO& io = ImGui::GetIO();
                if (io.WantCaptureKeyboard)
                {
                    switch (Msg)
                    {
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    case WM_CHAR:
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                        InterlockedDecrement(&ActiveWndProcCalls);
                        return 0;
                    }
                }
            }
        }

        LRESULT result = 0;
        if (Orig)
        {
            if (IsWindowUnicode(Hwnd))
                result = CallWindowProcW(Orig, Hwnd, Msg, WParam, LParam);
            else
                result = CallWindowProcA(Orig, Hwnd, Msg, WParam, LParam);
        }
        else
        {
            result = DefWindowProcA(Hwnd, Msg, WParam, LParam);
        }

        InterlockedDecrement(&ActiveWndProcCalls);
        return result;
    }
}
