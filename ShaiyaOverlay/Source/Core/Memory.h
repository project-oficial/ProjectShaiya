#pragma once

#include "Types.h"

namespace ShaiyaOverlay
{
    class Memory
    {
    public:
        template <typename T>
        static bool ReadSafe(U64 Address, T* OutBuffer)
        {
            if (!Address || !OutBuffer)
                return false;

            __try
            {
                *OutBuffer = *reinterpret_cast<const T*>(Address);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        template <typename T>
        static bool WriteSafe(U64 Address, const T& Value)
        {
            if (!Address)
                return false;

            __try
            {
                *reinterpret_cast<T*>(Address) = Value;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        static bool ReadBytesSafe(U64 Address, void* OutBuffer, U32 Size);
        static bool WriteBytesSafe(U64 Address, const void* InBuffer, U32 Size);
        static U64 FindPattern(HMODULE Module, const U8* Pattern, const char* Mask);
        static U64 GetModuleBase(const char* ModuleName = nullptr);
        static U32 GetModuleSize(HMODULE Module);
        static bool HookVTableFunction(void** VTable, U32 Index, void* NewFunc, void** OldFunc);
        static bool RestoreVTableFunction(void** VTable, U32 Index, void* OldFunc);
    };
}
