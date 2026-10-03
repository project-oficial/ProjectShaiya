#pragma once

#include "Core/Types.h"
#include <windows.h>

namespace ShaiyaOverlay
{
    enum class GameState : U8
    {
        Unknown = 0,
        Login = 1,
        CharacterSelect = 2,
        ServerSelect = 3,
        CharacterCreate = 4,
        CharacterDelete = 5,
        InGame = 6,
        Loading = 7,
        Exit = 9
    };

    struct AutoLoginConfig
    {
        bool Enabled;
        char Username[64];
        char Password[64];
        U32 ServerIndex;
        U32 CharacterSlot;
    };

    class AutoLoginManager
    {
    public:
        static bool Initialize();
        static void Shutdown();

        static bool Start(const char* OverrideUser = nullptr, const char* OverridePass = nullptr);
        static void Stop();

        static bool IsRunning();
        static GameState GetCurrentGameState();
        static const char* GetGameStateName(GameState State);
        static const char* GetStatusMessage();
        static const AutoLoginConfig& GetConfig();

    private:
        static DWORD WINAPI WorkerThread(LPVOID Param);
        static void LoadConfig();

        static void SendChar(HWND Hwnd, char C);
        static void SendKey(HWND Hwnd, UINT Vk);
        static void SendClick(HWND Hwnd, int X, int Y);

        static AutoLoginConfig Config;
        static volatile bool Running;
        static HANDLE ThreadHandle;
        static char StatusMessage[128];
    };
}
