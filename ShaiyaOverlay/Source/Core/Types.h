#pragma once

#include <windows.h>

namespace ShaiyaOverlay
{
    using U8 = unsigned char;
    using U16 = unsigned short;
    using U32 = unsigned long;
    using U64 = unsigned long long;

    using I8 = signed char;
    using I16 = signed short;
    using I32 = signed int;
    using I64 = signed long long;

    using F32 = float;
    using F64 = double;

    struct Vector3
    {
        F32 X;
        F32 Y;
        F32 Z;

        Vector3() : X(0.0f), Y(0.0f), Z(0.0f) {}
        Vector3(int) : X(0.0f), Y(0.0f), Z(0.0f) {}
        Vector3(F32 InX, F32 InY, F32 InZ) : X(InX), Y(InY), Z(InZ) {}

        F32 DistanceTo(const Vector3& Other) const
        {
            F32 Dx = X - Other.X;
            F32 Dy = Y - Other.Y;
            F32 Dz = Z - Other.Z;
            return Sqrt(Dx * Dx + Dy * Dy + Dz * Dz);
        }

        static F32 Sqrt(F32 Value)
        {
            if (Value <= 0.0f) return 0.0f;
            F32 X = Value;
            F32 XHalf = 0.5f * X;
            I32 I = *(I32*)&X;
            I = 0x5f3759df - (I >> 1);
            X = *(F32*)&I;
            X = X * (1.5f - XHalf * X * X);
            X = X * (1.5f - XHalf * X * X);
            return 1.0f / X;
        }
    };

    template <typename T, U32 Capacity>
    class FixedList
    {
    public:
        FixedList() : Count(0) {}

        void Clear()
        {
            Count = 0;
        }

        bool Add(const T& Item)
        {
            if (Count >= Capacity)
                return false;
            Items[Count++] = Item;
            return true;
        }

        U32 GetCount() const { return Count; }
        U32 GetCapacity() const { return Capacity; }

        T& operator[](U32 Index) { return Items[Index]; }
        const T& operator[](U32 Index) const { return Items[Index]; }

    private:
        T Items[Capacity];
        U32 Count;
    };
}
