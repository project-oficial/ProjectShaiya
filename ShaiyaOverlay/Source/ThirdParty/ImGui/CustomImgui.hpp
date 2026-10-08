#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <iostream>
#include <vector>
#include <windows.h>

namespace ImGui
{
    static bool ButtonExItem2(const char* label, const ImVec2& pos_lbl,
        ImTextureID texture, const ImVec2& pos_img,
        const ImVec2& size_img,
        ImVec4 color, const ImVec2& size_arg,
        ImGuiButtonFlags flags)
    {
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);
        ImVec2 pos = window->DC.CursorPos;
        if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset)

            pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
        ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
        ImRect bb(pos, pos + size);
        ItemSize(size, style.FramePadding.y);

        auto bb2 = bb;

        bb2.Max.y += 40.f;

        if (!ItemAdd(bb, id))
            return false;
        if (g.CurrentItemFlags & ImGuiItemFlags_ButtonRepeat)
            flags |= ImGuiButtonFlags_Repeat;
        bool hovered, held;
        bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

        const ImU32 col = (held || hovered) ? ImGui::ColorConvertFloat4ToU32(
            ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered)) :
            ImGui::ColorConvertFloat4ToU32(color);

        RenderNavHighlight(bb, id);
        RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

        //window->DrawList->AddImage( childbgSkin, bb.Min, bb.Max,
        //    ImVec2( 0, 0 ),
        //    ImVec2( 1, 1 ),
        //    col );

        if (texture)
            window->DrawList->AddImage(texture, bb.Min + pos_img, bb.Min + pos_img + size_img, ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32_WHITE);

        //window->DrawList->AddImage( skin.texture, bb.Min, bb.Max, ImVec2( 0, 0 ), ImVec2( 1, 1 ), IM_COL32_WHITE );

        bb.Min.y += 40.f;
        bb.Max.y += 40.f;

        RenderTextClipped(bb.Min + pos_lbl, bb.Min + pos_lbl, label, NULL, &label_size, style.ButtonTextAlign, &bb);

        return pressed;
    }
    static bool Hotkey(const char* label, int* k, const ImVec2& size_arg = { 0.f, 0.f })
    {
        const char* const KeyNames[] = {
            "Unknown",
            "VK_LBUTTON",
            "VK_RBUTTON",
            "VK_CANCEL",
            "VK_MBUTTON",
            "VK_XBUTTON1",
            "VK_XBUTTON2",
            "Unknown",
            "VK_BACK",
            "VK_TAB",
            "Unknown",
            "Unknown",
            "VK_CLEAR",
            "VK_RETURN",
            "Unknown",
            "Unknown",
            "VK_SHIFT",
            "VK_CONTROL",
            "VK_MENU",
            "VK_PAUSE",
            "VK_CAPITAL",
            "VK_KANA",
            "Unknown",
            "VK_JUNJA",
            "VK_FINAL",
            "VK_KANJI",
            "Unknown",
            "VK_ESCAPE",
            "VK_CONVERT",
            "VK_NONCONVERT",
            "VK_ACCEPT",
            "VK_MODECHANGE",
            "VK_SPACE",
            "PG UP",
            "PG DOWN",
            "VK_END",
            "VK_HOME",
            "VK_LEFT",
            "VK_UP",
            "VK_RIGHT",
            "VK_DOWN",
            "VK_SELECT",
            "VK_PRINT",
            "VK_EXECUTE",
            "VK_SNAPSHOT",
            "VK_INSERT",
            "VK_DELETE",
            "VK_HELP",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "A",
            "B",
            "C",
            "D",
            "E",
            "F",
            "G",
            "H",
            "I",
            "J",
            "K",
            "L",
            "M",
            "N",
            "O",
            "P",
            "Q",
            "R",
            "S",
            "T",
            "U",
            "V",
            "W",
            "X",
            "Y",
            "Z",
            "VK_LWIN",
            "VK_RWIN",
            "VK_APPS",
            "Unknown",
            "VK_SLEEP",
            "VK_NUMPAD0",
            "VK_NUMPAD1",
            "VK_NUMPAD2",
            "VK_NUMPAD3",
            "VK_NUMPAD4",
            "VK_NUMPAD5",
            "VK_NUMPAD6",
            "VK_NUMPAD7",
            "VK_NUMPAD8",
            "VK_NUMPAD9",
            "VK_MULTIPLY",
            "VK_ADD",
            "VK_SEPARATOR",
            "VK_SUBTRACT",
            "VK_DECIMAL",
            "VK_DIVIDE",
            "VK_F1",
            "VK_F2",
            "VK_F3",
            "VK_F4",
            "VK_F5",
            "VK_F6",
            "VK_F7",
            "VK_F8",
            "VK_F9",
            "VK_F10",
            "VK_F11",
            "VK_F12",
            "VK_F13",
            "VK_F14",
            "VK_F15",
            "VK_F16",
            "VK_F17",
            "VK_F18",
            "VK_F19",
            "VK_F20",
            "VK_F21",
            "VK_F22",
            "VK_F23",
            "VK_F24",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "VK_NUMLOCK",
            "VK_SCROLL",
            "VK_OEM_NEC_EQUAL",
            "VK_OEM_FJ_MASSHOU",
            "VK_OEM_FJ_TOUROKU",
            "VK_OEM_FJ_LOYA",
            "VK_OEM_FJ_ROYA",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "Unknown",
            "VK_LSHIFT",
            "VK_RSHIFT",
            "VK_LCONTROL",
            "VK_RCONTROL",
            "VK_LMENU",
            "VK_RMENU"
        };
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        ImGuiIO& io = g.IO;
        const ImGuiStyle& style = g.Style;

        const ImGuiID id = window->GetID(label);

        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

        ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), label_size.y + style.FramePadding.y * 2.0f);

        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);

        const ImRect total_bb(window->DC.CursorPos, frame_bb.Max);

        ImGui::ItemSize(total_bb, style.FramePadding.y);

        if (!ImGui::ItemAdd(total_bb, id))
            return false;

        const bool focus_requested = g.LastActiveId != id;

        const bool hovered = ImGui::ItemHoverable(frame_bb, id);

        if (hovered)
        {
            ImGui::SetHoveredID(id);
            g.MouseCursor = ImGuiMouseCursor_TextInput;
        }

        const bool user_clicked = hovered && io.MouseClicked[0];

        if (user_clicked)
        {
            memset(io.MouseDown, 0, sizeof(io.MouseDown));
            memset(io.KeysDown, 0, sizeof(io.KeysDown));
            *k = 0;

            ImGui::SetActiveID(id, window);
            ImGui::FocusWindow(window);
        }
        else if (io.MouseClicked[0]) {
            // Release focus when we click outside
            if (g.ActiveId == id)
            {
                ImGui::ClearActiveID();
            }
        }

        bool value_changed = false;
        int key = *k;

        if (g.ActiveId == id && g.ActiveIdTimer >= 1.f)
        {
            for (auto i = 0; i < 5; i++)
            {
                if (io.MouseDown[i])
                {
                    switch (i) {
                    case 0:
                        key = VK_LBUTTON;
                        break;
                    case 1:
                        key = VK_RBUTTON;
                        break;
                    case 2:
                        key = VK_MBUTTON;
                        break;
                    case 3:
                        key = VK_XBUTTON1;
                        break;
                    case 4:
                        key = VK_XBUTTON2;
                        break;
                    }

                    value_changed = true;

                    ImGui::ClearActiveID();
                }
            }

            if (!value_changed)
            {
                for (auto i = VK_BACK; i <= VK_RMENU; i++)
                {
                    if (io.KeysDown[i])
                    {
                        key = i;

                        value_changed = true;

                        ImGui::ClearActiveID();
                    }
                }
            }

            if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
            {
                *k = 0;
                ImGui::ClearActiveID();
            }
            else {
                *k = key;
            }
        }

        ImGuiCol ccl = ImGuiCol_ButtonActive;
        char buf_display[64] = "None";

        if (*k != 0 && g.ActiveId != id) {
            strcpy_s(buf_display, KeyNames[*k]);
        }
        else if (g.ActiveId == id) {
            ccl = ImGuiCol_Button;
            strcpy_s(buf_display, "<Press a key>");
        }

        ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ccl), true, style.FrameRounding);

        const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size

        ImVec2 render_pos = frame_bb.Min + style.FramePadding;

        std::string keyDisplay = (label_size.x > 0) ? (std::string(label) + " " + buf_display) : (buf_display);


        ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding, keyDisplay.c_str(), NULL, NULL, style.ButtonTextAlign, &clip_rect);

        //if ( label_size.x > 0 )
        //    ImGui::RenderText( ImVec2( total_bb.Min.x, frame_bb.Min.y + style.FramePadding.y ), label );

        return value_changed;
    }

    static void Image2(ImTextureID user_texture_id, const ImVec2& pos, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0))
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);

        if (border_col.w > 0.0f)
            bb.Max += ImVec2(2, 2);

        ImGui::ItemSize(bb);

        if (!ImGui::ItemAdd(bb, 0))
            return;

        window->DrawList->AddImage(user_texture_id, bb.Min + pos, bb.Max + pos, uv0, uv1, ImGui::GetColorU32(tint_col));

    }

    static void Image3(ImTextureID user_texture_id, const ImVec2& pos, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0))
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);

        if (border_col.w > 0.0f)
            bb.Max += ImVec2(2, 2);

        //ImGui::ItemSize( bb );

        //if ( !ImGui::ItemAdd( bb, 0 ) )
        //    return;

        window->DrawList->AddImage(user_texture_id, bb.Min + pos, bb.Max + pos, uv0, uv1, ImGui::GetColorU32(tint_col));

    }

    static void Image4(ImTextureID user_texture_id, const ImVec2& pos, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0))
    {
        ImRect bb({}, size);

        ImGui::GetCurrentWindow()->DrawList->AddImage(user_texture_id, bb.Min + pos, bb.Max + pos, uv0, uv1, ImGui::GetColorU32(tint_col));
    }

    static void SeparatorEx2(ImVec2 padding = { 30.f, 8.f }, ImU32 col = 0x10FFFFFF, float thickness = 1.f)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return;
        ImGuiContext& g = *GImGui;

        float thickness_layout = 0.0f;

        float x1 = window->Pos.x;
        float x2 = window->Pos.x + window->Size.x;
        if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
            x1 += window->DC.Indent.x;

        if (ImGuiTable* table = g.CurrentTable)
        {
            x1 = table->Columns[table->CurrentColumn].MinX;
            x2 = table->Columns[table->CurrentColumn].MaxX;
        }

        //const ImRect bb( window->DC.CursorPos, window->DC.CursorPos + size );
        //ItemSize( size );
        //ItemAdd( bb, 0 );


        ImRect bb(ImVec2(x1 + padding.x, window->DC.CursorPos.y + padding.y), ImVec2(x2 - padding.x, window->DC.CursorPos.y + padding.y + thickness));
        //ImRect bb2( ImVec2( x1 + padding.x, window->DC.CursorPos.y - padding.y ), ImVec2( x2 - padding.x, window->DC.CursorPos.y + thickness + padding.y ) );
        ItemSize(ImVec2(x2 - x1, thickness + padding.y + padding.y));
        const bool item_visible = ItemAdd(bb, 0);
        if (item_visible)
        {
            window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), col);

        }

    }

    inline ImVec4 hex2float_color(uint32_t hex_color, const float a = 1.0f)
    {
        auto* const p_byte = reinterpret_cast<uint8_t*>(&hex_color);
        const auto r = static_cast<float>(static_cast<float>(p_byte[2]) / 255.f);
        const auto g = static_cast<float>(static_cast<float>(p_byte[1]) / 255.f);
        const auto b = static_cast<float>(static_cast<float>(p_byte[0]) / 255.f);
        return { r, g, b, a };
    }

    inline void set_config_imgui()
    {

        auto style = &ImGui::GetStyle();
        auto& io = ImGui::GetIO();

        //style->Alpha = 0.0f;
        style->WindowPadding = ImVec2(8, 8);
        style->WindowMinSize = ImVec2(32, 32);
        style->WindowRounding = 0.5f;
        style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style->FramePadding = ImVec2(4, 2);
        style->FrameRounding = 0.0f;
        style->ItemSpacing = ImVec2(8, 4);
        style->ItemInnerSpacing = ImVec2(4, 4);
        style->TouchExtraPadding = ImVec2(0, 0);
        style->IndentSpacing = 21.0f;
        style->ColumnsMinSpacing = 3.0f;
        style->ScrollbarSize = 12.0f;
        style->ScrollbarRounding = 0.0f;
        style->GrabMinSize = 0.1f;
        style->GrabRounding = 0.0f;
        style->ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style->DisplayWindowPadding = ImVec2(22, 22);
        style->DisplaySafeAreaPadding = ImVec2(4, 4);
        style->AntiAliasedLines = true;
        style->CurveTessellationTol = 1.25f;

        static int hue = 140;

        ImVec4 col_text = ImColor::HSV(hue / 255.f, 20.f / 255.f, 235.f / 255.f);
        ImVec4 col_main = ImColor(9, 82, 128);
        ImVec4 col_back = ImColor(31, 44, 54);
        ImVec4 col_area = ImColor(4, 32, 41);

    }

    inline void ColorsDark()
    {
        auto style = &ImGui::GetStyle();
        auto& io = ImGui::GetIO();
        auto colors = style->Colors;

        io.IniFilename = "";
        style->FrameRounding = 3.0f;	//border radius buttons
        style->WindowTitleAlign.x = 0.5f; //Centraliza o titulo do menu
        style->GrabRounding = 2.0f;	//radius para as trackbar...
        style->WindowRounding = 6.0f; //Bordas do menu sem radius
        style->WindowBorderSize = 0.0f; //deixa uma fina borda no menu

        style->ChildRounding = 5.f;
        style->ScrollbarSize = 11.f;
        style->ScrollbarRounding = 12.f;



        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); //Texto por completo do menu
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); //Texto desativado por completo do menu
        colors[ImGuiCol_WindowBg] = hex2float_color(0x1a192b); //Fundo do menu
        colors[ImGuiCol_ChildBg] = hex2float_color(0x23273c); //Fundo do child
        colors[ImGuiCol_PopupBg] = hex2float_color(0x272c2f); //Fundo da popup/modal ou não
        colors[ImGuiCol_Border] = ImVec4(0.44f, 0.49f, 0.56f, 0.00f); //Linha da borda menu/childs/buttons
        colors[ImGuiCol_BorderShadow] = ImVec4(0.30f, 0.30f, 0.30f, 0.30f); //ImVec4(0.00f, 0.00f, 0.00f, 0.00f); //-- linha entre tabs 
        colors[ImGuiCol_FrameBg] = hex2float_color(0x850fcc); //Fundo da Slider
        colors[ImGuiCol_FrameBgHovered] = hex2float_color(0x592277); //Fundo da Slider mouse sobre
        colors[ImGuiCol_FrameBgActive] = hex2float_color(0xa63ee0); //Fundo da Slider quando interage
        colors[ImGuiCol_TitleBg] = colors[ImGuiCol_ChildBg]; //Fundo do titulo da str_window quando "Não" ativa
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.18f, 0.23f, 1.00f); //Fundo do titulo da str_window quando ativa
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.28f, 0.04f, 0.50f, 0.53f); //Fundo do titulo da str_window quando colapsada
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = hex2float_color(0x2a3034, 0.7f);
        colors[ImGuiCol_ScrollbarGrab] = hex2float_color(0x850fcc);//ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.44f, 0.15f, 0.72f, 0.80f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.44f, 0.15f, 0.72f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);;
        colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f); //Fundo do ponteiro do Slider
        colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); //Fundo do ponteiro do Slider quando ativo
        colors[ImGuiCol_Button] = hex2float_color(0x741aba); //glColor4f(0.556f, 0.266f, 0.678f, 1.0f); //button
        colors[ImGuiCol_ButtonHovered] = hex2float_color(0x631493);
        colors[ImGuiCol_ButtonActive] = hex2float_color(0x592277);
        colors[ImGuiCol_Header] = hex2float_color(0x850fcc);
        colors[ImGuiCol_HeaderHovered] = hex2float_color(0xa63ee0); //selected
        colors[ImGuiCol_HeaderActive] = ImVec4(0.f, 0.f, 0.f, 1.00f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];//ImVec4(0.61f, 0.61f, 0.61f, 1.00f);//Separador
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f); //Separador mouse sobre
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f); //Separador quando ativo
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        //colors[ImGuiCol_CloseButton]			= ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
        //colors[ImGuiCol_CloseButtonHovered]	= ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
        //colors[ImGuiCol_CloseButtonActive]	= ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        //colors[ImGuiCol_ModalWindowDarkening]	= ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.18f, 0.23f, 1.0f);
        colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.04f, 0.50f, 0.50f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
        //colors[ImGuiCol_TabClick]				= ImVec4(0.35f, 0.35f, 0.35f, 1.0f);


        colors[ImGuiCol_TabUnfocused] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_TableRowBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.0f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.0f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.5f);

    }


    inline auto vector_getter = [](void* vec, int idx, const char** out_text)
        {
            auto& vector = *static_cast<std::vector<std::string>*>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
            *out_text = vector.at(idx).c_str();
            return true;
        };

    inline bool ComboBoxArray(const char* label, int* currIndex, std::vector<std::string>& values)
    {
        if (values.empty()) { return false; }
        return Combo(label, currIndex, vector_getter,
            static_cast<void*>(&values), values.size());
    }

    inline bool TabLabels(const char** tabLabels, int tabSize, int& tabIndex, int* tabOrder)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        const ImVec2 itemSpacing = style.ItemSpacing;
        const ImVec4 color = style.Colors[ImGuiCol_Button];
        const ImVec4 colorActive = style.Colors[ImGuiCol_ButtonActive];
        const ImVec4 colorHover = style.Colors[ImGuiCol_ButtonHovered];
        const ImVec4 colorText = style.Colors[ImGuiCol_Text];
        style.ItemSpacing.x = 2.5;
        style.ItemSpacing.y = 1;
        const ImVec4 colorSelectedTab = ImVec4(color.x, color.y, color.z, color.w * 0.5f);
        const ImVec4 colorSelectedTabHovered = ImVec4(colorHover.x, colorHover.y, colorHover.z, colorHover.w * 0.5f);
        const ImVec4 colorSelectedTabText = ImVec4(colorText.x * 0.8f, colorText.y * 0.8f, colorText.z * 0.8f, colorText.w * 0.8f);

        if (tabSize > 0 && (tabIndex < 0 || tabIndex >= tabSize))
        {
            if (!tabOrder)
                tabIndex = 0;
            else
                tabIndex = -1;
        }

        float windowWidth = 0.f, sumX = 0.f;
        windowWidth = ImGui::GetWindowWidth() - style.WindowPadding.x - (ImGui::GetScrollMaxY() > 0 ? style.ScrollbarSize : 0.f);

        const bool isMMBreleased = ImGui::IsMouseReleased(2);
        int justClosedTabIndex = -1, newtabIndex = tabIndex;

        bool selection_changed = false; bool noButtonDrawn = true;

        for (int j = 0, i; j < tabSize; j++)
        {
            i = tabOrder ? tabOrder[j] : j;
            if (i == -1) continue;

            if (sumX > 0.f)
            {
                sumX += style.ItemSpacing.x;
                sumX += ImGui::CalcTextSize(tabLabels[i]).x + 2.f * style.FramePadding.x;

                if (sumX > windowWidth)
                    sumX = 0.f;
                else
                    ImGui::SameLine();
            }

            if (i != tabIndex)
            {
                // Push the style
                style.Colors[ImGuiCol_Button] = colorSelectedTab;
                style.Colors[ImGuiCol_ButtonActive] = colorSelectedTab;
                style.Colors[ImGuiCol_ButtonHovered] = colorSelectedTabHovered;
                style.Colors[ImGuiCol_Text] = colorSelectedTabText;
            }
            // Draw the button
            ImGui::PushID(i);   // otherwise two tabs with the same name would clash.
            if (ImGui::Button(tabLabels[i], ImVec2(windowWidth / tabSize, 35.f))) { selection_changed = (tabIndex != i); newtabIndex = i; }
            ImGui::PopID();
            if (i != tabIndex)
            {
                // Reset the style
                style.Colors[ImGuiCol_Button] = color;
                style.Colors[ImGuiCol_ButtonActive] = colorActive;
                style.Colors[ImGuiCol_ButtonHovered] = colorHover;
                style.Colors[ImGuiCol_Text] = colorText;
            }
            noButtonDrawn = false;
            if (sumX == 0.f) sumX = style.WindowPadding.x + ImGui::GetItemRectSize().x; // First element of a line
        }

        tabIndex = newtabIndex;

        // Change selected tab when user closes the selected tab
        if (tabIndex == justClosedTabIndex && tabIndex >= 0)
        {
            tabIndex = -1;
            for (int j = 0, i; j < tabSize; j++)
            {
                i = tabOrder ? tabOrder[j] : j;
                if (i == -1)
                    continue;
                tabIndex = i;
                break;
            }
        }

        // Restore the style
        style.Colors[ImGuiCol_Button] = color;
        style.Colors[ImGuiCol_ButtonActive] = colorActive;
        style.Colors[ImGuiCol_ButtonHovered] = colorHover;
        style.Colors[ImGuiCol_Text] = colorText;
        style.ItemSpacing = itemSpacing;

        return selection_changed;
    }

    inline void ToggleButton(const char* strId, bool* v, float size = 1.f)
    {

        float ANIM_SPEED = 1.f;

        const auto p = ImGui::GetCursorScreenPos();

        auto* DrawList = ImGui::GetWindowDrawList();

        const auto height = ImGui::GetFrameHeight() * size;

        const auto width = height * 1.55f;

        const auto radius = height * 0.50f;

        ImGui::InvisibleButton(strId, ImVec2(width, height));

        if (ImGui::IsItemClicked())
            *v = !*v;

        auto t = *v ? 1.0f : 0.0f;

        auto& g = *GImGui;

        const auto AnimSpeed = 0.08f;

        if (g.LastActiveId == g.CurrentWindow->GetID(strId) && g.LastActiveIdTimer < ANIM_SPEED)
        {
            const auto t_anim = ImSaturate(g.LastActiveIdTimer / AnimSpeed);

            t = *v ? (t_anim) : (1.0f - t_anim);
        }
        //0.34f, 0.05f, 0.62f, 0.84f
        //0.647f, 0.369f, 0.918f
        ImU32 col_bg;

        if (ImGui::IsItemHovered())
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), GetStyleColorVec4(ImGuiCol_CheckMark), 0.f));
        else
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), GetStyleColorVec4(ImGuiCol_CheckMark), 0.f));

        if (*v)
        {
            col_bg = ImGui::GetColorU32(ImGuiCol_Button, t);
        }

        DrawList->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
        DrawList->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
    }

    inline void TSeparator(float paddingHoz, ImColor col = ImColor{ 0.f, 0.f, 0.f, 0.1f })
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;

        float thickness_draw = 1.0f;

        float thickness_layout = 0.0f;

        float x1 = window->Pos.x + paddingHoz;

        float x2 = window->Pos.x + (window->Size.x - paddingHoz);

        if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
            x1 += window->DC.Indent.x;

        //if ( ImGuiTable* table = g.CurrentTable )
        //{
        //    x1 = table->Columns[ table->CurrentColumn ].MinX;
        //    x2 = table->Columns[ table->CurrentColumn ].MaxX;
        //}

        const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));

        ItemSize(ImVec2(0.0f, thickness_layout));

        const bool item_visible = ItemAdd(bb, 0);

        if (item_visible)
        {
            window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), col);
            window->DrawList->AddLine(bb.Min + ImVec2(0.f, 1.f), ImVec2(bb.Max.x, bb.Min.y) + ImVec2(0.f, 1.f), col);

        }
    }
}
