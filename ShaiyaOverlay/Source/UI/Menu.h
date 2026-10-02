#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    class Menu
    {
    public:
        static void Render();

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
