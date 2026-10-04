#include "StringUtils.h"
#include <windows.h>
#include <strsafe.h>

namespace ShaiyaOverlay
{
    U32 StringUtils::Length(const char* Str)
    {
        if (!Str) return 0;
        return (U32)lstrlenA(Str);
    }

    void StringUtils::Copy(char* Dest, const char* Src, U32 MaxLen)
    {
        if (!Dest || !Src || MaxLen == 0) return;
        StringCchCopyA(Dest, (size_t)MaxLen, Src);
    }

    void StringUtils::Format(char* Buffer, U32 MaxLen, const char* FormatStr, ...)
    {
        if (!Buffer || !FormatStr || MaxLen == 0) return;
        va_list Args;
        va_start(Args, FormatStr);
        StringCchVPrintfA(Buffer, (size_t)MaxLen, FormatStr, Args);
        va_end(Args);
    }

    void StringUtils::FormatFloat(char* Buffer, U32 MaxLen, F32 Value, I32 Precision)
    {
        if (!Buffer || MaxLen == 0) return;

        bool Negative = false;
        if (Value < 0.0f)
        {
            Negative = true;
            Value = -Value;
        }

        I32 IntegerPart = (I32)Value;
        F32 Remainder = Value - (F32)IntegerPart;

        I32 Multiplier = 1;
        for (I32 I = 0; I < Precision; ++I)
            Multiplier *= 10;

        I32 FractionPart = (I32)(Remainder * (F32)Multiplier + 0.5f);

        if (Negative)
            StringCchPrintfA(Buffer, (size_t)MaxLen, "-%d.%0*d", IntegerPart, Precision, FractionPart);
        else
            StringCchPrintfA(Buffer, (size_t)MaxLen, "%d.%0*d", IntegerPart, Precision, FractionPart);
    }

    bool StringUtils::Equals(const char* Str1, const char* Str2)
    {
        if (!Str1 || !Str2) return Str1 == Str2;
        return lstrcmpA(Str1, Str2) == 0;
    }

    char StringUtils::NormalizeChar(char C)
    {
        if (C >= 'A' && C <= 'Z') return static_cast<char>(C + ('a' - 'A'));
        unsigned char Uc = static_cast<unsigned char>(C);
        if (Uc == 0xE7 || Uc == 0xC7) return 'c';
        if ((Uc >= 0xE0 && Uc <= 0xE6) || (Uc >= 0xC0 && Uc <= 0xC6)) return 'a';
        if ((Uc >= 0xE8 && Uc <= 0xEB) || (Uc >= 0xC8 && Uc <= 0xCB)) return 'e';
        if ((Uc >= 0xEC && Uc <= 0xEF) || (Uc >= 0xCC && Uc <= 0xCF)) return 'i';
        if ((Uc >= 0xF2 && Uc <= 0xF6) || (Uc >= 0xD2 && Uc <= 0xD6)) return 'o';
        if ((Uc >= 0xF9 && Uc <= 0xFC) || (Uc >= 0xD9 && Uc <= 0xDC)) return 'u';
        return C;
    }

    void StringUtils::NormalizeString(const char* Src, char* Dst, U32 MaxLen)
    {
        if (!Dst || MaxLen == 0) return;
        Dst[0] = '\0';
        if (!Src) return;

        U32 OutIdx = 0;
        U32 InIdx = 0;

        while (Src[InIdx] && OutIdx + 1 < MaxLen)
        {
            unsigned char B1 = static_cast<unsigned char>(Src[InIdx]);
            unsigned char B2 = static_cast<unsigned char>(Src[InIdx + 1]);

            // Handle UTF-8 2-byte accented characters
            if (B1 == 0xC3 && B2 != 0)
            {
                if (B2 == 0xA7 || B2 == 0x87)
                    Dst[OutIdx++] = 'c';
                else if ((B2 >= 0xA0 && B2 <= 0xA6) || (B2 >= 0x80 && B2 <= 0x86))
                    Dst[OutIdx++] = 'a';
                else if ((B2 >= 0xA8 && B2 <= 0xAB) || (B2 >= 0x88 && B2 <= 0x8B))
                    Dst[OutIdx++] = 'e';
                else if ((B2 >= 0xAC && B2 <= 0xAF) || (B2 >= 0x8C && B2 <= 0x8F))
                    Dst[OutIdx++] = 'i';
                else if ((B2 >= 0xB2 && B2 <= 0xB6) || (B2 >= 0x92 && B2 <= 0x96))
                    Dst[OutIdx++] = 'o';
                else if ((B2 >= 0xB9 && B2 <= 0xBC) || (B2 >= 0x99 && B2 <= 0x9C))
                    Dst[OutIdx++] = 'u';
                else
                    Dst[OutIdx++] = NormalizeChar(static_cast<char>(B2));
                InIdx += 2;
                continue;
            }

            Dst[OutIdx++] = NormalizeChar(static_cast<char>(B1));
            ++InIdx;
        }

        Dst[OutIdx] = '\0';
    }

    bool StringUtils::ContainsNormalized(const char* Haystack, const char* Needle)
    {
        if (!Haystack || !Needle) return false;

        char NormH[128];
        char NormN[128];
        NormalizeString(Haystack, NormH, sizeof(NormH));
        NormalizeString(Needle, NormN, sizeof(NormN));

        U32 HLen = Length(NormH);
        U32 NLen = Length(NormN);
        if (HLen < 2 || NLen < 2) return false;

        auto Substr = [](const char* H, U32 HL, const char* N, U32 NL) -> bool {
            if (HL < NL) return false;
            for (U32 i = 0; i <= HL - NL; ++i)
            {
                bool Match = true;
                for (U32 j = 0; j < NL; ++j)
                {
                    if (H[i + j] != N[j]) { Match = false; break; }
                }
                if (Match) return true;
            }
            return false;
        };

        return Substr(NormH, HLen, NormN, NLen) || Substr(NormN, NLen, NormH, HLen);
    }

    void StringUtils::AnsiToUtf8(const char* AnsiStr, char* Utf8Str, U32 MaxLen)
    {
        if (!AnsiStr || !Utf8Str || MaxLen == 0) return;
        wchar_t WBuf[128] = { 0 };
        int WLen = MultiByteToWideChar(CP_ACP, 0, AnsiStr, -1, WBuf, 128);
        if (WLen > 0)
        {
            WideCharToMultiByte(CP_UTF8, 0, WBuf, -1, Utf8Str, static_cast<int>(MaxLen), nullptr, nullptr);
            Utf8Str[MaxLen - 1] = '\0';
        }
        else
        {
            Copy(Utf8Str, AnsiStr, MaxLen);
        }
    }
}
