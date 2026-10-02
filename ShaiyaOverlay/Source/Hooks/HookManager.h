#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class HookManager
    {
    public:
        static bool Initialize();
        static void Uninitialize();
        static bool IsActive() { return Active; }

    private:
        static bool Active;
    };
}
