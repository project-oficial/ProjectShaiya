#include "BuffManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"

namespace ShaiyaOverlay
{
    FixedList<BuffInfo, 32> BuffManager::ActiveBuffs;

    void BuffManager::Update()
    {
        // Buff tracking ready for packet / struct parser
        ActiveBuffs.Clear();
    }
}
