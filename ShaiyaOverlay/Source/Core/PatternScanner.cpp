#include "PatternScanner.h"
#include "Memory.h"

namespace ShaiyaOverlay
{
    U8 PatternScanner::HexCharToNibble(char C)
    {
        if (C >= '0' && C <= '9') return static_cast<U8>(C - '0');
        if (C >= 'a' && C <= 'f') return static_cast<U8>(C - 'a' + 10);
        if (C >= 'A' && C <= 'F') return static_cast<U8>(C - 'A' + 10);
        return 0;
    }

    bool PatternScanner::Compile(const char* AobPattern, CompiledPattern* OutPattern)
    {
        if (!AobPattern || !OutPattern)
            return false;

        OutPattern->Length = 0;
        const char* Ptr = AobPattern;

        while (*Ptr)
        {
            while (*Ptr == ' ' || *Ptr == '\t' || *Ptr == '\r' || *Ptr == '\n')
                ++Ptr;

            if (!*Ptr)
                break;

            if (OutPattern->Length >= 128)
                return false;

            if (*Ptr == '?')
            {
                OutPattern->Bytes[OutPattern->Length] = 0;
                OutPattern->Mask[OutPattern->Length] = false;
                ++OutPattern->Length;
                ++Ptr;
                if (*Ptr == '?')
                    ++Ptr;
            }
            else
            {
                char C1 = *Ptr++;
                char C2 = *Ptr ? *Ptr++ : '0';

                U8 High = HexCharToNibble(C1);
                U8 Low = HexCharToNibble(C2);

                OutPattern->Bytes[OutPattern->Length] = static_cast<U8>((High << 4) | Low);
                OutPattern->Mask[OutPattern->Length] = true;
                ++OutPattern->Length;
            }
        }

        return OutPattern->Length > 0;
    }

    U64 PatternScanner::Scan(U64 StartAddress, U64 Size, const char* AobPattern)
    {
        CompiledPattern Pattern;
        if (!Compile(AobPattern, &Pattern) || Size < Pattern.Length)
            return 0;

        const U8* Base = reinterpret_cast<const U8*>(StartAddress);
        U64 ScanEnd = Size - Pattern.Length;

        for (U64 I = 0; I <= ScanEnd; ++I)
        {
            bool Found = true;
            for (U32 J = 0; J < Pattern.Length; ++J)
            {
                if (Pattern.Mask[J] && Base[I + J] != Pattern.Bytes[J])
                {
                    Found = false;
                    break;
                }
            }

            if (Found)
                return StartAddress + I;
        }

        return 0;
    }

    U64 PatternScanner::ScanModule(HMODULE Module, const char* AobPattern)
    {
        if (!Module)
            Module = GetModuleHandleA(nullptr);

        if (!Module)
            return 0;

        U64 Base = reinterpret_cast<U64>(Module);
        U32 Size = Memory::GetModuleSize(Module);
        return Scan(Base, Size, AobPattern);
    }

    U64 PatternScanner::RipRelative(U64 MatchAddress, U32 DispOffset, U32 InstructionLength)
    {
        if (!MatchAddress)
            return 0;

        I32 Disp = 0;
        if (!Memory::ReadSafe(MatchAddress + DispOffset, &Disp))
            return 0;

        return MatchAddress + InstructionLength + static_cast<I64>(Disp);
    }
}
