#include "EditorTheme.h"

#include "imgui.h"

namespace
{
    ImVec4 Color(float r, float g, float b, float a = 1.0f)
    {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
    }
}

void EditorTheme::Apply()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.CellPadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(9.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 14.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    style.WindowRounding = 10.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = Color(220, 226, 233);
    colors[ImGuiCol_TextDisabled] = Color(120, 128, 138);

    colors[ImGuiCol_WindowBg] = Color(24, 26, 27);
    colors[ImGuiCol_ChildBg] = Color(34, 36, 37);
    colors[ImGuiCol_PopupBg] = Color(39, 41, 42);

    colors[ImGuiCol_Border] = Color(92, 96, 98);
    colors[ImGuiCol_BorderShadow] = Color(0, 0, 0, 0.0f);

    colors[ImGuiCol_FrameBg] = Color(57, 60, 61);
    colors[ImGuiCol_FrameBgHovered] = Color(74, 78, 79);
    colors[ImGuiCol_FrameBgActive] = Color(88, 96, 96);

    colors[ImGuiCol_TitleBg] = Color(15, 17, 20);
    colors[ImGuiCol_TitleBgActive] = Color(22, 25, 29);
    colors[ImGuiCol_TitleBgCollapsed] = Color(15, 17, 20);

    colors[ImGuiCol_MenuBarBg] = Color(18, 20, 23);

    colors[ImGuiCol_ScrollbarBg] = Color(18, 20, 23);
    colors[ImGuiCol_ScrollbarGrab] = Color(55, 62, 70);
    colors[ImGuiCol_ScrollbarGrabHovered] = Color(70, 80, 90);
    colors[ImGuiCol_ScrollbarGrabActive] = Color(88, 100, 112);

    colors[ImGuiCol_CheckMark] = Color(89, 211, 190);
    colors[ImGuiCol_SliderGrab] = Color(238, 171, 103);
    colors[ImGuiCol_SliderGrabActive] = Color(250, 198, 123);

    colors[ImGuiCol_Button] = Color(57, 60, 61);
    colors[ImGuiCol_ButtonHovered] = Color(80, 88, 86);
    colors[ImGuiCol_ButtonActive] = Color(89, 211, 190);

    colors[ImGuiCol_Header] = Color(57, 60, 61);
    colors[ImGuiCol_HeaderHovered] = Color(77, 82, 82);
    colors[ImGuiCol_HeaderActive] = Color(89, 211, 190);

    colors[ImGuiCol_Separator] = Color(52, 58, 66);
    colors[ImGuiCol_SeparatorHovered] = Color(56, 163, 199);
    colors[ImGuiCol_SeparatorActive] = Color(85, 190, 225);

    colors[ImGuiCol_ResizeGrip] = Color(56, 163, 199, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = Color(56, 163, 199, 0.55f);
    colors[ImGuiCol_ResizeGripActive] = Color(56, 163, 199, 0.85f);

    colors[ImGuiCol_Tab] = Color(39, 41, 42);
    colors[ImGuiCol_TabHovered] = Color(74, 78, 79);
    colors[ImGuiCol_TabActive] = Color(57, 60, 61);
    colors[ImGuiCol_TabUnfocused] = Color(29, 31, 32);
    colors[ImGuiCol_TabUnfocusedActive] = Color(45, 48, 49);

    colors[ImGuiCol_DockingPreview] = Color(56, 163, 199, 0.35f);
    colors[ImGuiCol_DockingEmptyBg] = Color(18, 20, 23);

    colors[ImGuiCol_PlotLines] = Color(56, 163, 199);
    colors[ImGuiCol_PlotLinesHovered] = Color(85, 190, 225);
    colors[ImGuiCol_PlotHistogram] = Color(216, 166, 87);
    colors[ImGuiCol_PlotHistogramHovered] = Color(235, 185, 100);

    colors[ImGuiCol_TableHeaderBg] = Color(28, 32, 37);
    colors[ImGuiCol_TableBorderStrong] = Color(58, 64, 72);
    colors[ImGuiCol_TableBorderLight] = Color(43, 48, 55);
    colors[ImGuiCol_TableRowBg] = Color(0, 0, 0, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = Color(255, 255, 255, 0.03f);

    colors[ImGuiCol_TextSelectedBg] = Color(56, 163, 199, 0.35f);
    colors[ImGuiCol_DragDropTarget] = Color(85, 190, 225, 0.90f);
    colors[ImGuiCol_NavHighlight] = Color(56, 163, 199);
    colors[ImGuiCol_NavWindowingHighlight] = Color(220, 226, 233, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = Color(0, 0, 0, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = Color(0, 0, 0, 0.35f);
}
