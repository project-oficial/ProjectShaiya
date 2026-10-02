#pragma once

#include "Types.h"

namespace ShaiyaOverlay
{
    struct Vector2
    {
        F32 X;
        F32 Y;

        Vector2() : X(0.0f), Y(0.0f) {}
        Vector2(F32 InX, F32 InY) : X(InX), Y(InY) {}
    };

    struct Matrix4x4
    {
        F32 M[16];
    };

    class Camera
    {
    public:
        static bool WorldToScreen(const Vector3& WorldPos, Vector2& OutScreen, F32 ScreenWidth, F32 ScreenHeight);
    };
}
