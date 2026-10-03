#include "AutoLoginManager.h"
#include "Core/Logger.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"
#include "Hooks/WndProcHook.h"
#include <stdio.h>

namespace ShaiyaOverlay
{
    AutoLoginConfig AutoLoginManager::Config = { false, "", "", 0, 0 };
    volatile bool AutoLoginManager::Running = false;
    HANDLE AutoLoginManager::ThreadHandle = nullptr;
    char AutoLoginManager::StatusMessage[128] = "Idle";

    const AutoLoginConfig& AutoLoginManager::GetConfig()
    {
        return Config;
    }

    bool AutoLoginManager::Initialize()
    {
        LoadConfig();

        if (Config.Enabled && Config.Username[0] != '\0' && Config.Password[0] != '\0')
        {
            Logger::Info("[AutoLogin] AutoLogin is enabled for user '%s'. Starting automation...", Config.Username);
            Start();
        }
        else
        {
            Logger::Info("[AutoLogin] AutoLogin initialized (Enabled=%s).", Config.Enabled ? "true" : "false");
        }

        return true;
    }

    void AutoLoginManager::Shutdown()
    {
        Stop();
    }

    void AutoLoginManager::LoadConfig()
    {
        Config.Enabled = false;
        Config.Username[0] = '\0';
        Config.Password[0] = '\0';
        Config.ServerIndex = 0;
        Config.CharacterSlot = 0;

        // Candidate paths for auto_login.ini
        const char* CandidatePaths[] = {
            "auto_login.ini",
            "..\\auto_login.ini",
            "ShaiyaOverlay\\auto_login.ini",
            "..\\ShaiyaOverlay\\auto_login.ini"
        };

        char FoundPath[MAX_PATH] = { 0 };
        for (const char* Path : CandidatePaths)
        {
            if (GetFileAttributesA(Path) != INVALID_FILE_ATTRIBUTES)
            {
                GetFullPathNameA(Path, MAX_PATH, FoundPath, nullptr);
                break;
            }
        }

        if (FoundPath[0] == '\0')
        {
            Logger::Info("[AutoLogin] No auto_login.ini found.");
            return;
        }

        Logger::Info("[AutoLogin] Loading configuration from '%s'", FoundPath);
        Config.Enabled = (GetPrivateProfileIntA("AutoLogin", "Enabled", 1, FoundPath) != 0);
        GetPrivateProfileStringA("AutoLogin", "Username", "darklee", Config.Username, sizeof(Config.Username), FoundPath);
        GetPrivateProfileStringA("AutoLogin", "Password", "", Config.Password, sizeof(Config.Password), FoundPath);
        Config.ServerIndex = GetPrivateProfileIntA("AutoLogin", "ServerIndex", 0, FoundPath);
        Config.CharacterSlot = GetPrivateProfileIntA("AutoLogin", "CharacterSlot", 0, FoundPath);
    }

    bool AutoLoginManager::IsRunning()
    {
        return Running;
    }

    GameState AutoLoginManager::GetCurrentGameState()
    {
        if (!Offsets.GameStateAddr)
            return GameState::Unknown;

        U8 RawState = 0;
        if (!Memory::ReadSafe(Offsets.GameStateAddr, &RawState))
            return GameState::Unknown;

        return static_cast<GameState>(RawState);
    }

    const char* AutoLoginManager::GetGameStateName(GameState State)
    {
        switch (State)
        {
        case GameState::Login:           return "Login Screen";
        case GameState::CharacterSelect: return "Character Select";
        case GameState::ServerSelect:    return "Server Select";
        case GameState::CharacterCreate: return "Character Create";
        case GameState::CharacterDelete: return "Character Delete";
        case GameState::InGame:          return "In Game";
        case GameState::Loading:         return "Loading World";
        case GameState::Exit:            return "Exit";
        default:                         return "Unknown";
        }
    }

    const char* AutoLoginManager::GetStatusMessage()
    {
        return StatusMessage;
    }

    void AutoLoginManager::SendChar(HWND Hwnd, char C)
    {
        PostMessageA(Hwnd, WM_CHAR, static_cast<WPARAM>(static_cast<U8>(C)), 0);
    }

    void AutoLoginManager::SendKey(HWND Hwnd, UINT Vk)
    {
        PostMessageA(Hwnd, WM_KEYDOWN, Vk, 0);
        Sleep(15);
        PostMessageA(Hwnd, WM_KEYUP, Vk, (1 << 30) | (1 << 31));
    }

    void AutoLoginManager::SendClick(HWND Hwnd, int X, int Y)
    {
        LPARAM LParam = MAKELPARAM(X, Y);
        PostMessageA(Hwnd, WM_LBUTTONDOWN, MK_LBUTTON, LParam);
        Sleep(25);
        PostMessageA(Hwnd, WM_LBUTTONUP, 0, LParam);
    }

    bool AutoLoginManager::Start(const char* OverrideUser, const char* OverridePass)
    {
        if (Running)
            return false;

        if (OverrideUser && OverrideUser[0] != '\0')
            StringUtils::Copy(Config.Username, OverrideUser, sizeof(Config.Username));

        if (OverridePass && OverridePass[0] != '\0')
            StringUtils::Copy(Config.Password, OverridePass, sizeof(Config.Password));

        if (Config.Username[0] == '\0' || Config.Password[0] == '\0')
        {
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Error: Missing credentials");
            return false;
        }

        Running = true;
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Starting...");

        ThreadHandle = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        return (ThreadHandle != nullptr);
    }

    void AutoLoginManager::Stop()
    {
        Running = false;
        if (ThreadHandle)
        {
            WaitForSingleObject(ThreadHandle, 1000);
            CloseHandle(ThreadHandle);
            ThreadHandle = nullptr;
        }
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Stopped");
    }

    DWORD WINAPI AutoLoginManager::WorkerThread(LPVOID Param)
    {
        Logger::Info("[AutoLogin] Worker thread started.");

        // Wait for GameOffsets to be ready
        int WaitOffsets = 30;
        while (Running && !Offsets.GameStateAddr && WaitOffsets-- > 0)
        {
            Sleep(100);
        }

        HWND Hwnd = WndProcHook::GetWindowHandle();
        if (!Hwnd)
        {
            Hwnd = FindWindowA("SDL_app", "Shaiya");
        }

        if (!Hwnd)
        {
            Logger::Error("[AutoLogin] Could not find game window handle.");
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Error: No window handle");
            Running = false;
            return 0;
        }

        GameState CurrentState = GetCurrentGameState();
        Logger::Info("[AutoLogin] Initial GameState: %d (%s)", static_cast<U8>(CurrentState), GetGameStateName(CurrentState));

        if (CurrentState == GameState::InGame)
        {
            Logger::Info("[AutoLogin] Player is already in-game. Nothing to do.");
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Already in game");
            Running = false;
            return 0;
        }

        // STEP 1: Handle Login Screen (State 1)
        if (CurrentState == GameState::Login)
        {
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Waiting for login screen UI...");
            Sleep(1500);

            if (!Running) return 0;

            RECT rc;
            GetClientRect(Hwnd, &rc);
            int ClientW = rc.right - rc.left;
            int ClientH = rc.bottom - rc.top;
            if (ClientW <= 0) ClientW = 1920;
            if (ClientH <= 0) ClientH = 1080;

            int UserBoxX = ClientW / 2;
            int UserBoxY = ClientH - 260;

            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Entering credentials...");
            Logger::Info("[AutoLogin] Focusing username box at (%d, %d)", UserBoxX, UserBoxY);
            SendClick(Hwnd, UserBoxX, UserBoxY);
            Sleep(150);

            // Clear any text in username box
            for (int i = 0; i < 32 && Running; ++i)
            {
                SendKey(Hwnd, VK_BACK);
                Sleep(8);
            }

            // Type username
            for (size_t i = 0; Config.Username[i] != '\0' && Running; ++i)
            {
                SendChar(Hwnd, Config.Username[i]);
                Sleep(25);
            }
            Sleep(100);

            // Switch to password
            SendKey(Hwnd, VK_TAB);
            Sleep(100);

            // Clear password box
            for (int i = 0; i < 32 && Running; ++i)
            {
                SendKey(Hwnd, VK_BACK);
                Sleep(8);
            }

            // Type password
            for (size_t i = 0; Config.Password[i] != '\0' && Running; ++i)
            {
                SendChar(Hwnd, Config.Password[i]);
                Sleep(25);
            }
            Sleep(200);

            Logger::Info("[AutoLogin] Credentials typed. Pressing Enter to submit...");
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Submitting login...");
            SendKey(Hwnd, VK_RETURN);
            Sleep(1000);
        }

        // STEP 2: Wait for Server Select (State 3) or Character Select (State 2)
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Waiting for server select...");
        int Timeout = 120; // 12 seconds
        while (Running && Timeout-- > 0)
        {
            CurrentState = GetCurrentGameState();
            if (CurrentState == GameState::ServerSelect ||
                CurrentState == GameState::CharacterSelect ||
                CurrentState == GameState::InGame)
            {
                break;
            }
            Sleep(100);
        }

        if (!Running) return 0;

        if (CurrentState == GameState::ServerSelect)
        {
            Logger::Info("[AutoLogin] Server Select screen active. Selecting server index %u...", Config.ServerIndex);
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Selecting server %u...", Config.ServerIndex);
            Sleep(1200);

            // Send Enter to confirm server selection
            SendKey(Hwnd, VK_RETURN);
            Sleep(1000);
        }

        // STEP 3: Wait for Character Select (State 2)
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Waiting for character select...");
        Timeout = 120;
        while (Running && Timeout-- > 0)
        {
            CurrentState = GetCurrentGameState();
            if (CurrentState == GameState::CharacterSelect || CurrentState == GameState::InGame)
            {
                break;
            }
            Sleep(100);
        }

        if (!Running) return 0;

        if (CurrentState == GameState::CharacterSelect)
        {
            Logger::Info("[AutoLogin] Character Select screen active. Selecting character slot %u...", Config.CharacterSlot);
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Selecting character slot %u...", Config.CharacterSlot);
            Sleep(1500);

            // Send Enter to start game with selected character
            SendKey(Hwnd, VK_RETURN);
            Sleep(1000);
        }

        // STEP 4: Wait for In-Game (State 6)
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Entering world...");
        Timeout = 200; // 20 seconds
        while (Running && Timeout-- > 0)
        {
            CurrentState = GetCurrentGameState();
            if (CurrentState == GameState::InGame)
            {
                Logger::Info("[AutoLogin] Reached In-Game world successfully! Auto-login complete.");
                StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Completed (In Game)");
                Running = false;
                return 0;
            }
            Sleep(100);
        }

        if (CurrentState != GameState::InGame)
        {
            Logger::Info("[AutoLogin] Finished with GameState: %d (%s)", static_cast<U8>(CurrentState), GetGameStateName(CurrentState));
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Done (%s)", GetGameStateName(CurrentState));
        }

        Running = false;
        return 0;
    }
}
