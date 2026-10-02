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
        static void RenderOverviewWindow();
        static void RenderEntitiesWindow();
        static void RenderGroundItemsWindow();
        static void RenderSkillsWindow();
        static void RenderQuickSlotsWindow();
        static void RenderQuestsWindow();
        static void RenderBuffsWindow();
        static void RenderGroundItemSnaplines();
        static void RenderQuestWaypoints();

        static bool SnaplinesEnabled;
        static bool QuestWaypointsEnabled;
    };
}
