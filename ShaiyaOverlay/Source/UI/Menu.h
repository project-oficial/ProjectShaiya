#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class Menu
    {
    public:
        static void Render();

        static bool IsSnaplinesEnabled() { return SnaplinesEnabled; }
        static void SetSnaplinesEnabled(bool enabled) { SnaplinesEnabled = enabled; }
        static bool IsQuestWaypointsEnabled() { return QuestWaypointsEnabled; }
        static void SetQuestWaypointsEnabled(bool enabled) { QuestWaypointsEnabled = enabled; }

    private:
        static void RenderMainWindow();
        static void RenderStatusTab();
        static void RenderAutoComboTab();
        static void RenderAutoHealTab();
        static void RenderAutoLootTab();
        static void RenderAutoBuffTab();
        static void RenderGrindBotTab();

        static void RenderGroundItemSnaplines();
        static void RenderQuestWaypoints();

        static bool SnaplinesEnabled;
        static bool QuestWaypointsEnabled;
    };
}
