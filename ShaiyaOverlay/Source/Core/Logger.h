#pragma once

#include "Types.h"

namespace ShaiyaOverlay
{
    class Logger
    {
    public:
        static bool Initialize();
        static void Uninitialize();

        static void Log(const char* Format, ...);
        static void Info(const char* Format, ...);
        static void Error(const char* Format, ...);

    private:
        static void WriteOutput(const char* Level, const char* Message);

        static HANDLE ConsoleHandle;
        static HANDLE FileHandle;
        static bool Initialized;
    };
}
