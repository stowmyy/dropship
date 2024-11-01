#include "pch.h"

#include "Dashboard.h"


extern std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
extern std::unique_ptr<Settings> g_settings;
extern std::unique_ptr<core::tunneling::Tunneling> g_tunneling;
extern std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;

#include "images.h"

extern ImFont* font_title;
extern ImFont* font_subtitle;
extern ImFont* font_text;


void renderBackground() {
    static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
    ImVec2 const windowPos = ImGui::GetWindowPos();
    ImVec2 const windowSize = ImGui::GetWindowSize();
    ImDrawList* bg_list = ImGui::GetBackgroundDrawList();

    // background texture
    {
        // TODO dark mode if deactivated?
        bg_list->AddRectFilled(windowPos, windowPos + ImGui::GetWindowSize(), white, 9);
        bg_list->AddImageRounded(_get_texture("background_app"), windowPos, windowPos + ImVec2(windowSize.x, std::min(windowSize.y, 699.0f)), ImVec2(0, 0), ImVec2(1, 1), white, 9);
    }
}

void renderButtons(std::vector<std::unique_ptr<PopupButton>>& buttons) {
    /*for (auto& button : buttons) {
        (*button).render();
        ImGui::SameLine();
    }
    ImGui::NewLine();*/

    static auto &style = ImGui::GetStyle();

    static const auto width = (ImGui::GetContentRegionAvail().x / buttons.size()) - ((style.ItemSpacing.x / 2.0f) * (buttons.size() - 1));

    for (auto& button : buttons) {
        (*button).render2({ width, 40 });
        if (!(*button).getTooltip().empty()) ImGui::SetItemTooltip((*button).getTooltip().c_str());
        ImGui::SameLine();
    }
    ImGui::NewLine();
}

void renderHeader(std::string title, std::string description) {

    static auto& style = ImGui::GetStyle();
    ImDrawList* list = ImGui::GetWindowDrawList();
    const auto widgetPos = ImGui::GetCursorScreenPos();

    auto const size = 24;
    static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    // dashboard copy
    ImGui::BeginGroup();
    {
        list->AddText(font_title, font_title->FontSize, widgetPos - ImVec2(1, 0), color_text, title.c_str());
        ImGui::Dummy({ 0, font_title->FontSize - 6 });

        //ImGui::Bullet();
        ImGui::TextWrapped(description.c_str());
    }
    ImGui::EndGroup();

    //ImGui::Spacing();
}

void renderEndpoints() {
    ImGui::BeginChild("endpoints_scrollable", ImVec2(ImGui::GetContentRegionAvail().x, 500), false);
    {
        ImGui::Spacing();
        for (int i = 0; i < (*g_endpoints).size(); i++) {
            (*(*g_endpoints)[i]).render(i);

            if (ImGui::IsItemActivated()) {
                //printf("%s\n", (*(*endpoints)[i]).getTitle().c_str());

                //(*settings_m).syncEndpoint((*endpoints)[i]);

                if ((*(*g_endpoints)[i]).getBlockDesired()) {
                    (*g_settings).addBlockedEndpoint((*(*g_endpoints)[i]).getTitle());
                }
                else {
                    (*g_settings).removeBlockedEndpoint((*(*g_endpoints)[i]).getTitle());
                }

            }
        }
    }
    ImGui::EndChild();
}

void renderFooter(bool* open) {

    static auto& style = ImGui::GetStyle();
    ImDrawList* list = ImGui::GetWindowDrawList();

    static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
    static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    ImGui::Dummy({ ImGui::GetContentRegionAvail().x, ImGui::GetFont()->FontSize + (style.FramePadding.y * 2) });
    list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), white, 5);
    list->AddText(ImGui::GetItemRectMin() + style.FramePadding + ImVec2(2, 0), color_text, "Advanced");

    static ImVec2 frame = ImVec2(24, 24);
    auto const pos = ImVec2(ImGui::GetItemRectMax() - frame - style.FramePadding);
    auto const uv_min = !*open ? ImVec2(0, 0) : ImVec2(0, 1);
    auto const uv_max = !*open ? ImVec2(1, 1) : ImVec2(1, 0);
    list->AddImage(_get_texture("icon_angle"), pos, pos + frame, uv_min, uv_max, color_text);

    if (ImGui::IsItemClicked())
        *open = !*open;

    if (*open)
    {
        ImGui::BeginGroup();
        {
            ImGui::Indent(style.FramePadding.x);
            {
                ImGui::Text("Available in a future update.");
                // static bool test;
                // ToggleButton("test", &test);
            }
            ImGui::Unindent();
        }
        ImGui::EndGroup();

    }
}

void Dashboard::render() {

    /* background */
    renderBackground();

    /* header */
    renderHeader(
        "OW2 // DROPSHIP",
        "Deselecting regions will block them permanently until you select them again."
    );

    if (g_settings)
    {
        (*g_settings).renderWaitingStatus();
    }

    renderButtons(this->header_actions);

    /* endpoints */
    renderEndpoints();

    if (g_tunneling)
    {
        g_tunneling->render();
    }

    /* footer */
    //renderFooter(&(this->footer));
}



