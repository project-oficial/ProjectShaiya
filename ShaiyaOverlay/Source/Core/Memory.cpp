#include "Memory.h"
#include "StringUtils.h"

namespace ShaiyaOverlay
{
    bool Memory::ReadBytesSafe(U64 Address, void* OutBuffer, U32 Size)
    {
        if (!Address || !OutBuffer || Size == 0)
            return false;

        __try
        {
            const U8* Src = reinterpret_cast<const U8*>(Address);
            U8* Dst = reinterpret_cast<U8*>(OutBuffer);
            for (U32 I = 0; I < Size; ++I)
            {
                Dst[I] = Src[I];
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool Memory::WriteBytesSafe(U64 Address, const void* InBuffer, U32 Size)
    {
        if (!Address || !InBuffer || Size == 0)
            return false;

        __try
        {
            const U8* Src = reinterpret_cast<const U8*>(InBuffer);
            U8* Dst = reinterpret_cast<U8*>(Address);
            for (U32 I = 0; I < Size; ++I)
            {
                Dst[I] = Src[I];
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    U64 Memory::GetModuleBase(const char* ModuleName)
    {
        return reinterpret_cast<U64>(GetModuleHandleA(ModuleName));
    }

    U32 Memory::GetModuleSize(HMODULE Module)
    {
        if (!Module)
            Module = GetModuleHandleA(nullptr);

        if (!Module)
            return 0;

        U8* Base = reinterpret_cast<U8*>(Module);
        IMAGE_DOS_HEADER* DosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(Base);
        if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return 0;

        IMAGE_NT_HEADERS* NtHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(Base + DosHeader->e_lfanew);
        if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
            return 0;

        return NtHeaders->OptionalHeader.SizeOfImage;
    }

    U64 Memory::FindPattern(HMODULE Module, const U8* Pattern, const char* Mask)
    {
        if (!Module)
            Module = GetModuleHandleA(nullptr);

        if (!Module || !Pattern || !Mask)
            return 0;

        U32 PatternLen = StringUtils::Length(Mask);
        if (PatternLen == 0)
            return 0;

        U8* Base = reinterpret_cast<U8*>(Module);
        U32 Size = GetModuleSize(Module);
        if (Size < PatternLen)
            return 0;

        U32 SearchLen = Size - PatternLen;

        for (U32 I = 0; I < SearchLen; ++I)
        {
            bool Found = true;
            for (U32 J = 0; J < PatternLen; ++J)
            {
                if (Mask[J] == '?' || Base[I + J] == Pattern[J])
                    continue;

                Found = false;
                break;
            }

            if (Found)
                return reinterpret_cast<U64>(Base + I);
        }

        return 0;
    }

    bool Memory::HookVTableFunction(void** VTable, U32 Index, void* NewFunc, void** OldFunc)
    {
        if (!VTable || !NewFunc)
            return false;

        DWORD OldProtect = 0;
        if (!VirtualProtect(&VTable[Index], sizeof(void*), PAGE_EXECUTE_READWRITE, &OldProtect))
            return false;

        if (OldFunc)
            *OldFunc = VTable[Index];

        VTable[Index] = NewFunc;

        VirtualProtect(&VTable[Index], sizeof(void*), OldProtect, &OldProtect);
        return true;
    }

    bool Memory::RestoreVTableFunction(void** VTable, U32 Index, void* OldFunc)
    {
        if (!VTable || !OldFunc)
            return false;

        DWORD OldProtect = 0;
        if (!VirtualProtect(&VTable[Index], sizeof(void*), PAGE_EXECUTE_READWRITE, &OldProtect))
            return false;

        VTable[Index] = OldFunc;

        VirtualProtect(&VTable[Index], sizeof(void*), OldProtect, &OldProtect);
        return true;
    }
}
