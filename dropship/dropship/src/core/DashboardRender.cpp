#include "pch.h"

#include "Dashboard.h"


extern std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
extern std::unique_ptr<Settings> g_settings;
extern std::unique_ptr<core::tunneling::Tunneling> g_tunneling;
extern std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;
extern std::unique_ptr<Updater> g_updater;

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

void renderNotice() {
    static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
    static const ImU32 white2 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .8f });
    static const ImU32 white4 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .6f });
    static const ImU32 transparent = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0 });
    
    ImDrawList* list = ImGui::GetWindowDrawList();
    ImDrawList* bg_list = ImGui::GetBackgroundDrawList();

    static const auto padding = ImVec2(14, 8);

    static const auto height = 102;

    list->PushClipRect(ImGui::GetCursorScreenPos() + ImVec2(0, padding.y), ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x, 120 + 24), true);
    
    ImGui::Spacing();
    ImGui::Dummy(padding);
    ImGui::SameLine(NULL, 0);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, white4);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, white);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, white2);

    // if scrollbar
    // ImGui::BeginChild("notice", ImVec2(ImGui::GetContentRegionAvail().x - padding.x, height), 0);

    // if no scrollbar
    ImGui::BeginChild("notice", ImVec2(ImGui::GetContentRegionAvail().x, height), 0);
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding.y);
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding.y);
            ImGui::Dummy({ 20, 20 });
            list->AddImage((void*)_get_texture("icon_bolt"), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), white);
            ImGui::SameLine();

            list->AddText(font_title, 28, ImGui::GetCursorScreenPos() - ImVec2(0, 6), white, "NOTICE");
            ImGui::PushFont(font_subtitle);
            {
                ImGui::SameLine(262);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2);
                ImGui::TextUnformatted("10/03/2025");

                ImGui::TextWrapped("GSG1 (Singapore) and GTK1 (Tokyo) have been updated, please let us know if you have issues.");
                // ImGui::TextWrapped("The blocklist has been updated, please let us know if you have any connection issues.");
                // ImGui::TextWrapped("To report a connection issue: Please check which server you are connected to with CTRL");

                ImGui::Spacing();
            }
            ImGui::PopFont();
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(4);

    static const auto bg_color = (ImU32) ImColor::HSV(1.0f - ((1.4f) / 32.0f), 0.4f, 1.0f, 1.0f);

    // background texture
    {
        bg_list->AddRectFilled(ImGui::GetItemRectMin() - ImVec2(padding.x, 0), ImGui::GetItemRectMax(), bg_color, 9);
    }

    {
        static const auto color = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.09f });
        const auto pos = ImGui::GetItemRectMin() - padding;

        static const auto image = _get_image("background_diagonal");
        list->AddImage(image.texture, pos, pos + ImVec2((float)image.width, (float)image.height), ImVec2(0, 0), ImVec2(1, 1), color);
    }

    list->PopClipRect();

    //ImGui::Spacing();
}

void Dashboard::render() {

    /* background */
    renderBackground();

    if (!g_updater) {
        ImGui::TextColored(ImVec4{ 1, 0.6f, 0.6f, 1 }, "TEST VERSION - Updater disabled\nUpdate manually");
    }

    /* notice */
    renderNotice();

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



