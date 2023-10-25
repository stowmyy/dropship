

# include "imgui.h"
# include "theme.h"

void setTheme(THEME theme)
{
    if (theme == dark)
    {
        ImGui::StyleColorsDark();
    }
    else
    {
        ImGui::StyleColorsLight();
    }

    // spacing and padding is not overriden by changing colors

    auto io = ImGui::GetIO();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        //MyApp::SetupImGuiStyle();

        //style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        style.WindowBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        // for fully transparent windows backgrounds
        //style.Colors[ImGuiCol_WindowBg].w = 0.4f;

        // for semi transparent globally
        // TODO. hovering window = .9?
        // style.Alpha = .6f;

        style.WindowRounding = 9.0f;
        style.PopupRounding = 9.0f;
        style.ChildRounding = 9.0f;

        // demo style editor :D
        style.WindowPadding = { 16.0f, 16.0f };
        style.WindowTitleAlign = { 0.0f, 0.5f };
        style.ItemSpacing = { 8.0f, 8.0f };
        style.CellPadding = { 8.0f, 4.0f };

        style.FrameRounding = 5.0f;
        //style.FramePadding = { 4.0f, 4.0f };
        style.FramePadding = { 10.0f, 8.0f };

        style.SelectableRounding = 5.0f;

        style.IndentSpacing = 20;

        style.GrabMinSize = 8;
        style.GrabRounding = 4;

        //style.SeparatorTextPadding = 3;

        style.ItemSpacing = { 10, 10 };

        style.DisabledAlpha = 0.4;

        if (theme == dark)
        {
            style.Colors[ImGuiCol_ModalWindowDimBg] = { 0.0f, 0.0f, 0.0f, 0.90f };
        }
        else if (theme == light)
        {
            style.Colors[ImGuiCol_Text] = { 0, 0, 0, 1 };
            //style.Colors[ImGuiCol_Text] = { 0.4f, 0.4f, 0.4f, 1 };
            //style.Colors[ImGuiCol_Text] = { .2, .2, .2, 1 };
        }

    }

}

void ToggleButton(const char* str_id, bool* v)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !*v;

    float t = *v ? 1.0f : 0.0f;

    ImGuiContext& g = *GImGui;
    float ANIM_SPEED = 0.08f;
    if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
    {
        float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
        t = *v ? (t_anim) : (1.0f - t_anim);
    }

    static const auto color_toggle_background = ImColor::HSV(fmod(-0.02f, 1.0f) / 14.0f, 0.4f, 1.0f, 1.0f);


    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), color_toggle_background, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
}

