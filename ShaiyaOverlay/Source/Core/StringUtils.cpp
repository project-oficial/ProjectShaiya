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

    static unsigned char SimplifyDoubleChar(unsigned char c1, unsigned char c2, bool bChangeToLowerCase)
    {
        if (c1 == 0xC2)
        {
            if (c2 == 0xAA) return 'a';
            if (c2 == 0xBA) return 'o';
            if (c2 == 0xA9) return 'c';
            if (c2 == 0xAE) return 'r';
        }

        if (c1 == 0xC3)
        {
            if (c2 >= 0x80 && c2 <= 0x85) return bChangeToLowerCase ? 'a' : 'A';
            if (c2 >= 0xA0 && c2 <= 0xA5) return 'a';
            if (c2 >= 0x88 && c2 <= 0x8B) return bChangeToLowerCase ? 'e' : 'E';
            if (c2 >= 0xA8 && c2 <= 0xAB) return 'e';
            if (c2 >= 0x8C && c2 <= 0x8F) return bChangeToLowerCase ? 'i' : 'I';
            if (c2 >= 0xAC && c2 <= 0xAF) return 'i';
            if (c2 >= 0x92 && c2 <= 0x96) return bChangeToLowerCase ? 'o' : 'O';
            if (c2 >= 0xB2 && c2 <= 0xB6) return 'o';
            if (c2 >= 0x99 && c2 <= 0x9C) return bChangeToLowerCase ? 'u' : 'U';
            if (c2 >= 0xB9 && c2 <= 0xBC) return 'u';
            if (c2 == 0x87) return bChangeToLowerCase ? 'c' : 'C';
            if (c2 == 0xA7) return 'c';
            if (c2 == 0x91) return bChangeToLowerCase ? 'n' : 'N';
            if (c2 == 0xB1) return 'n';
            if (c2 == 0x9F) return 's';
            if (c2 == 0x9D) return bChangeToLowerCase ? 'y' : 'Y';
            if (c2 == 0xBD || c2 == 0xBF) return 'y';
        }

        if (c1 == 0xC5)
        {
            if (c2 == 0xBD) return bChangeToLowerCase ? 'z' : 'Z';
            if (c2 == 0xBE) return 'z';
            if (c2 == 0xB8) return bChangeToLowerCase ? 'y' : 'Y';
        }

        return c1;
    }

    static unsigned char SimplifySingleChar(unsigned char c, bool bChangeToLowerCase)
    {
        if ((c >= 0xC0 && c <= 0xC5) || (c >= 0xE0 && c <= 0xE5) || c == 0xAA)
            return (c >= 0xE0 || bChangeToLowerCase) ? 'a' : 'A';

        if ((c >= 0xC8 && c <= 0xCB) || (c >= 0xE8 && c <= 0xEB))
            return (c >= 0xE8 || bChangeToLowerCase) ? 'e' : 'E';

        if ((c >= 0xCC && c <= 0xCF) || (c >= 0xEC && c <= 0xEF))
            return (c >= 0xEC || bChangeToLowerCase) ? 'i' : 'I';

        if ((c >= 0xD2 && c <= 0xD6) || (c >= 0xF2 && c <= 0xF6) || c == 0xBA)
            return (c >= 0xF2 || bChangeToLowerCase) ? 'o' : 'O';

        if ((c >= 0xD9 && c <= 0xDC) || (c >= 0xF9 && c <= 0xFC))
            return (c >= 0xF9 || bChangeToLowerCase) ? 'u' : 'U';

        if (c == 0xA9 || c == 0xC7 || c == 0xE7)
            return (c == 0xE7 || bChangeToLowerCase) ? 'c' : 'C';

        if (c == 0xD1 || c == 0xF1)
            return (c == 0xF1 || bChangeToLowerCase) ? 'n' : 'N';

        if (c == 0xAE)
            return 'r';

        if (c == 0xDF)
            return 's';

        if (c == 0x8E || c == 0x9E)
            return (c == 0x9E || bChangeToLowerCase) ? 'z' : 'Z';

        if (c == 0x9F || c == 0xDD || c == 0xFD || c == 0xFF)
            return (c == 0xFD || c == 0xFF || bChangeToLowerCase) ? 'y' : 'Y';

        return c;
    }

    void StringUtils::NormalizeAccents(char* Str, U32 MaxLen, bool ChangeToLowerCase)
    {
        if (!Str || Str[0] == '\0') return;

        U32 StrLen = Length(Str);
        U32 Limit = (MaxLen > 0 && MaxLen - 1 < StrLen) ? MaxLen - 1 : StrLen;

        U32 InIdx = 0;
        U32 OutIdx = 0;

        while (InIdx < Limit && Str[InIdx] != '\0')
        {
            unsigned char c = static_cast<unsigned char>(Str[InIdx]);
            if (c >= 0x80)
            {
                // Check if this is a UTF-8 2-byte leading character (0xC2, 0xC3, 0xC5)
                if ((c == 0xC2 || c == 0xC3 || c == 0xC5) && (InIdx + 1 < Limit) && (static_cast<unsigned char>(Str[InIdx + 1]) >= 0x80))
                {
                    unsigned char c2 = SimplifyDoubleChar(c, static_cast<unsigned char>(Str[InIdx + 1]), ChangeToLowerCase);
                    if (c2 < 0x80)
                    {
                        Str[OutIdx++] = static_cast<char>(c2);
                        InIdx += 2;
                        continue;
                    }
                    else
                    {
                        InIdx += 2;
                        continue;
                    }
                }

                // Single-byte ANSI / ISO-8859-1 / Windows-1252 character
                unsigned char c2 = SimplifySingleChar(c, ChangeToLowerCase);
                if (c2 < 0x80)
                {
                    Str[OutIdx++] = static_cast<char>(c2);
                }
                ++InIdx;
            }
            else
            {
                if (ChangeToLowerCase && c >= 'A' && c <= 'Z')
                    Str[OutIdx++] = static_cast<char>(c + ('a' - 'A'));
                else
                    Str[OutIdx++] = static_cast<char>(c);
                ++InIdx;
            }
        }

        Str[OutIdx] = '\0';
    }
}
