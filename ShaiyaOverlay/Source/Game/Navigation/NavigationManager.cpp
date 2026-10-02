#include "NavigationManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Core/Logger.h"
#include "Game/GameOffsets.h"

namespace ShaiyaOverlay
{
    bool NavigationManager::Active = false;
    Vector3 NavigationManager::TargetPos;
    F32 NavigationManager::ArrivalRadius = 2.5f;
    F32 NavigationManager::RemainingDistance = 0.0f;
    char NavigationManager::DestinationName[64] = { 0 };
    U32 NavigationManager::LastPacketTick = 0;

    static bool KeyIsDown = false;

    static HWND GetGameHwnd()
    {
        if (Offsets.GameHwnd)
        {
            U64 HwndVal = 0;
            if (Memory::ReadSafe(Offsets.GameHwnd, &HwndVal) && HwndVal)
                return reinterpret_cast<HWND>(HwndVal);
        }
        return FindWindowA("SDL_app", nullptr);
    }

    void NavigationManager::WalkTo(const Vector3& Target, const char* TargetName, F32 StopDistance)
    {
        TargetPos = Target;
        ArrivalRadius = StopDistance > 1.0f ? StopDistance : 1.5f;
        Active = true;
        LastPacketTick = 0;

        if (TargetName && TargetName[0] != '\0')
            StringUtils::Copy(DestinationName, TargetName, sizeof(DestinationName));
        else
            StringUtils::Format(DestinationName, sizeof(DestinationName), "Pos (%.0f, %.0f)", Target.X, Target.Z);

        Logger::Info("Navigation: Auto-walk started to %s at (%.1f, %.1f, %.1f)",
            DestinationName, Target.X, Target.Y, Target.Z);

        HWND Hwnd = GetGameHwnd();
        if (Hwnd)
        {
            PostMessageA(Hwnd, WM_KEYDOWN, 'W', 1 | (0x11 << 16));
            KeyIsDown = true;
        }

        if (Offsets.KeyBuffer)
        {
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x11) = 0x80;
            KeyIsDown = true;
        }
    }

    void NavigationManager::Stop()
    {
        if (!Active && !KeyIsDown)
            return;

        Active = false;
        RemainingDistance = 0.0f;

        HWND Hwnd = GetGameHwnd();
        if (Hwnd && KeyIsDown)
        {
            PostMessageA(Hwnd, WM_KEYUP, 'W', 1 | (0x11 << 16) | (1 << 30) | (1 << 31));
        }

        if (Offsets.KeyBuffer)
        {
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x11) = 0x00;
        }

        KeyIsDown = false;

        Logger::Info("Navigation: Auto-walk stopped.");
    }

    void NavigationManager::Update()
    {
        if (!Active)
            return;

        HWND Hwnd = GetGameHwnd();

        // Cancel on manual user movement or escape ONLY when game is foreground
        if (Hwnd && GetForegroundWindow() == Hwnd)
        {
            if ((GetAsyncKeyState('W') & 0x8000) ||
                (GetAsyncKeyState('A') & 0x8000) ||
                (GetAsyncKeyState('S') & 0x8000) ||
                (GetAsyncKeyState('D') & 0x8000) ||
                (GetAsyncKeyState(VK_ESCAPE) & 0x8000))
            {
                Logger::Info("Navigation: Manual cancel detected.");
                Stop();
                return;
            }
        }

        if (!Offsets.WorldManager)
        {
            Stop();
            return;
        }

        U64 LocalPlayerPtr = 0;
        if (!Memory::ReadSafe(Offsets.WorldManager + Offsets.LocalPlayerPtrOffset, &LocalPlayerPtr) || !LocalPlayerPtr)
        {
            Stop();
            return;
        }

        Vector3 CurPos;
        if (!Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosX, &CurPos.X) ||
            !Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosY, &CurPos.Y) ||
            !Memory::ReadSafe(LocalPlayerPtr + Offsets.PlayerPosZ, &CurPos.Z))
        {
            Stop();
            return;
        }

        F32 Dx = TargetPos.X - CurPos.X;
        F32 Dz = TargetPos.Z - CurPos.Z;
        RemainingDistance = Vector3::Sqrt(Dx * Dx + Dz * Dz);

        // Check if arrived at destination
        if (RemainingDistance <= ArrivalRadius)
        {
            Logger::Info("Navigation: Arrived at %s! Distance: %.1fm", DestinationName, RemainingDistance);
            Stop();
            return;
        }

        F32 InvLen = 1.0f / RemainingDistance;
        F32 DirX = Dx * InvLen;
        F32 DirZ = Dz * InvLen;

        // Orient camera behind the character looking towards destination
        if (Offsets.CameraEye)
        {
            F32 LookX = 0.0f;
            F32 LookY = 0.0f;
            F32 LookZ = 0.0f;
            Memory::ReadSafe(Offsets.CameraEye + 0x0C, &LookX);
            Memory::ReadSafe(Offsets.CameraEye + 0x10, &LookY);
            Memory::ReadSafe(Offsets.CameraEye + 0x14, &LookZ);

            F32 CamDist = 5.0f;
            F32 EyeX = LookX - CamDist * DirX;
            F32 EyeZ = LookZ - CamDist * DirZ;

            // Primary CameraEye at 0x1407F8520
            *reinterpret_cast<F32*>(Offsets.CameraEye) = EyeX;
            *reinterpret_cast<F32*>(Offsets.CameraEye + 8) = EyeZ;

            // Secondary CameraEye at ImageBase + 0x9DF010
            U64 CamEye2 = (Offsets.CameraEye - 0x7F8520) + 0x9DF010;
            if (CamEye2)
            {
                *reinterpret_cast<F32*>(CamEye2) = EyeX;
                *reinterpret_cast<F32*>(CamEye2 + 8) = EyeZ;
            }
        }

        // Set direction vectors in LocalPlayer
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x60) = DirX;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x64) = 0.0f;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x68) = DirZ;

        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x338) = DirX;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x33C) = 0.0f;
        *reinterpret_cast<F32*>(LocalPlayerPtr + 0x340) = DirZ;

        // Ensure keydown is maintained directly in memory buffer and via PostMessage fallback
        if (Offsets.KeyBuffer)
        {
            *reinterpret_cast<U8*>(Offsets.KeyBuffer + 0x11) = 0x80;
        }

        if (Hwnd && !KeyIsDown)
        {
            PostMessageA(Hwnd, WM_KEYDOWN, 'W', 1 | (0x11 << 16));
            KeyIsDown = true;
        }
    }
}
