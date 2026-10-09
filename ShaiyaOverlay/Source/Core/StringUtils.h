#pragma once

#include "Types.h"

namespace ShaiyaOverlay
{
    class StringUtils
    {
    public:
        static U32 Length(const char* Str);
        static void Copy(char* Dest, const char* Src, U32 MaxLen);
        static void Format(char* Buffer, U32 MaxLen, const char* FormatStr, ...);
        static void FormatFloat(char* Buffer, U32 MaxLen, F32 Value, I32 Precision = 1);
        static bool Equals(const char* Str1, const char* Str2);
        static char NormalizeChar(char C);
        static void NormalizeString(const char* Src, char* Dst, U32 MaxLen);
        static bool ContainsNormalized(const char* Haystack, const char* Needle);
        static bool ContainsCaseInsensitive(const char* Haystack, const char* Needle);
        static void AnsiToUtf8(const char* AnsiStr, char* Utf8Str, U32 MaxLen);
        static void NormalizeAccents(char* Str, U32 MaxLen = 0, bool ChangeToLowerCase = false);
    };
}
