#pragma once

#include "Types.h"

namespace ShaiyaOverlay
{
    struct CompiledPattern
    {
        U8 Bytes[128];
        bool Mask[128];
        U32 Length;
    };

    class PatternScanner
    {
    public:
        static bool Compile(const char* AobPattern, CompiledPattern* OutPattern);
        static U64 Scan(U64 StartAddress, U64 Size, const char* AobPattern);
        static U64 ScanModule(HMODULE Module, const char* AobPattern);
        static U64 RipRelative(U64 MatchAddress, U32 DispOffset, U32 InstructionLength);

    private:
        static U8 HexCharToNibble(char C);
    };
}
