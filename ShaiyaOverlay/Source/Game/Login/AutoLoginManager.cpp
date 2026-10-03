#include "AutoLoginManager.h"
#include "Core/Logger.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"
#include "Hooks/WndProcHook.h"
#include <stdio.h>

namespace ShaiyaOverlay
{
    AutoLoginConfig AutoLoginManager::Config = { false, "", "", 0, 3 };
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
        Config.CharacterSlot = 3;

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
        Config.CharacterSlot = GetPrivateProfileIntA("AutoLogin", "CharacterSlot", 3, FoundPath);
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
        UINT Scan = MapVirtualKeyA(Vk, MAPVK_VK_TO_VSC);
        if (Vk == VK_RETURN) Scan = 0x1C;

        if (Offsets.KeyBuffer)
        {
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + Scan) = 0x80;
        }

        LPARAM lpDown = 1 | (Scan << 16);
        LPARAM lpUp = 1 | (Scan << 16) | (1 << 30) | (1 << 31);

        PostMessageA(Hwnd, WM_KEYDOWN, Vk, lpDown);
        Sleep(60);
        PostMessageA(Hwnd, WM_KEYUP, Vk, lpUp);

        if (Offsets.KeyBuffer)
        {
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + Scan) = 0x00;
        }
    }

    void AutoLoginManager::SendClick(HWND Hwnd, int X, int Y)
    {
        LPARAM LParam = MAKELPARAM(X, Y);
        PostMessageA(Hwnd, WM_LBUTTONDOWN, MK_LBUTTON, LParam);
        Sleep(35);
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

        int WaitOffsets = 40;
        while (Running && (!Offsets.GameStateAddr || !Offsets.LoginPtr) && WaitOffsets-- > 0)
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
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Waiting for server handshake...");
            Logger::Info("[AutoLogin] Waiting for login server RSA handshake...");

            int HandshakeTimeout = 150; // 15 seconds
            while (Running && HandshakeTimeout-- > 0)
            {
                U16 Handshake = 0;
                if (Offsets.HandshakeStatusAddr && Memory::ReadSafe(Offsets.HandshakeStatusAddr, &Handshake))
                {
                    if (Handshake == 0x0101)
                    {
                        Logger::Info("[AutoLogin] Handshake confirmed (0x%04X)!", Handshake);
                        break;
                    }
                }
                Sleep(100);
            }

            if (!Running) return 0;
            Sleep(500);

            // Wait for CLogin instance constructor to finish
            U64 pLogin = 0;
            int LoginWait = 100;
            while (Running && !pLogin && LoginWait-- > 0)
            {
                U64 candidate = 0;
                if (Offsets.LoginPtr && Memory::ReadSafe(Offsets.LoginPtr, &candidate) && candidate)
                {
                    U32 sig = 0;
                    if (Memory::ReadSafe(candidate + 35624, &sig) && sig == 1139802112)
                    {
                        pLogin = candidate;
                        break;
                    }
                }
                Sleep(100);
            }

            // Wait 2.5 seconds for full intro fade-in and textures to settle
            StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Waiting for login screen to settle...");
            Sleep(2500);

            if (pLogin)
            {
                // Defocus text boxes (focus = 2) so engine render loop doesn't wipe our buffers
                U8 noFocus = 2;
                Memory::WriteSafe(pLogin + 1576, noFocus);

                // Write Username buffer at pLogin + 8 (35 bytes)
                char userBuf[35] = { 0 };
                StringUtils::Copy(userBuf, Config.Username, sizeof(userBuf));
                Memory::WriteBytesSafe(pLogin + 8, userBuf, sizeof(userBuf));

                // Write Password buffer at pLogin + 43 (35 bytes)
                char passBuf[35] = { 0 };
                StringUtils::Copy(passBuf, Config.Password, sizeof(passBuf));
                Memory::WriteBytesSafe(pLogin + 43, passBuf, sizeof(passBuf));

                // Also write directly to pNet buffers as a safety net
                if (Offsets.NetworkPtr)
                {
                    U64 pNet = 0;
                    if (Memory::ReadSafe(Offsets.NetworkPtr, &pNet) && pNet)
                    {
                        Memory::WriteBytesSafe(pNet + 3864, userBuf, sizeof(userBuf));
                        Memory::WriteBytesSafe(pNet + 3899, passBuf, sizeof(passBuf));
                    }
                }

                Logger::Info("[AutoLogin] Submitting credentials in background (user: %s)...", Config.Username);
                StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Submitting credentials...");

                if (Offsets.SubmitLoginAddr)
                {
                    HANDLE hThread = CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
                        using SubmitLoginFn = __int64(__fastcall*)(U64 pThis);
                        auto Fn = reinterpret_cast<SubmitLoginFn>(Offsets.SubmitLoginAddr);
                        Fn(reinterpret_cast<U64>(param));
                        return 0;
                    }, reinterpret_cast<LPVOID>(pLogin), 0, nullptr);

                    if (hThread)
                    {
                        WaitForSingleObject(hThread, 4000);
                        CloseHandle(hThread);
                    }
                }
            }
            else
            {
                Logger::Error("[AutoLogin] CLogin pointer is null!");
            }

            Sleep(800);
        }

        // STEP 2: Wait for Server List and select server
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Waiting for server list...");
        int ServerWait = 150; // 15 seconds max
        U32 ServerCount = 0;
        U64 pLogin = 0;

        while (Running && ServerWait-- > 0)
        {
            if (Offsets.NetworkPtr)
            {
                U64 pNet = 0;
                if (Memory::ReadSafe(Offsets.NetworkPtr, &pNet) && pNet)
                {
                    Memory::ReadSafe(pNet + 3856, &ServerCount);
                    if (ServerCount > 0)
                        break;
                }
            }
            Sleep(100);
        }

        if (!Running || ServerCount == 0) return 0;

        Logger::Info("[AutoLogin] Server list received (%u servers). Selecting server %u in background...", ServerCount, Config.ServerIndex);
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Selecting server %u...", Config.ServerIndex);
        Sleep(500);

        if (Offsets.LoginPtr)
            Memory::ReadSafe(Offsets.LoginPtr, &pLogin);

        U64 pSelectServer = pLogin ? (pLogin + 1584) : 0;
        if (pSelectServer)
        {
            *reinterpret_cast<U32*>(pSelectServer + 6368) = Config.ServerIndex;
            *reinterpret_cast<U32*>(pSelectServer + 14568) = Config.ServerIndex;
            *reinterpret_cast<U8*>(pSelectServer + 1128) = 1;
        }

        // Pulse Enter via internal KeyBuffer (works 100% in background without window focus!)
        if (Offsets.KeyBuffer)
        {
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x1C) = 0x80;
            Sleep(150);
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x1C) = 0x00;
        }
        else
        {
            SendKey(Hwnd, VK_RETURN);
        }
        Logger::Info("[AutoLogin] Triggered server %u confirmation via KeyBuffer!", Config.ServerIndex);

        // STEP 3: Wait for Character Select screen to arrive and stop there
        StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Entering character select...");
        int CharSelectWait = 150;
        U64 pCharSelect = 0;

        while (Running && CharSelectWait-- > 0)
        {
            CurrentState = GetCurrentGameState();
            if (Offsets.CharacterSelectPtr)
                Memory::ReadSafe(Offsets.CharacterSelectPtr, &pCharSelect);

            if (pCharSelect != 0 || CurrentState == GameState::CharacterSelect)
            {
                Logger::Info("[AutoLogin] Character selection screen reached successfully! Pausing automation at Character Select as requested.");
                StringUtils::Format(StatusMessage, sizeof(StatusMessage), "Na Selecao de Personagens");
                Running = false;
                return 0;
            }

            // Retry pulse KeyBuffer if UI was animating
            if (Offsets.KeyBuffer)
            {
                *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x1C) = 0x80;
                Sleep(100);
                *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x1C) = 0x00;
            }
            Sleep(500);
        }

        Running = false;
        return 0;
    }
}
