#pragma once

#include "Core/Types.h"
#include "Buff.h"

namespace ShaiyaOverlay
{
    class BuffManager
    {
    public:
        static void Update();
        static const FixedList<BuffInfo, 32>& GetBuffs() { return ActiveBuffs; }
        static U32 GetBuffCount() { return ActiveBuffs.GetCount(); }

    private:
        static FixedList<BuffInfo, 32> ActiveBuffs;
    };
}
