#include "pch.h"

#include "PopupButton.h"

PopupButton::PopupButton(std::string title, std::string icon, std::vector<Action> const actions, int i, std::string tooltip):
	_title(title),
	_icon(icon),
    _actions(actions),
    _i(i),

	//popup_name(title + "_popup")
	_popup_name(title),
    _tooltip(tooltip)
{
	
}

void PopupButton::render2(const ImVec2& size) {

    static auto& style = ImGui::GetStyle();
    ImDrawList* list = ImGui::GetWindowDrawList();

    /*const auto color_button = ImColor::HSV(0, 0.4f, 1, 1.f);
    const auto color_button_hover = ImColor::HSV(0, 0.25f, 1, 1.f);*/

    //static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
    //static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    const ImU32 color_button = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
    const ImU32 color_button_hover = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
    const ImU32 color_secondary_faded = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);

    {
        static const auto font = font_text;
        static const auto font_size = font->CalcTextSizeA(24, FLT_MAX, 0.0f, this->_title.c_str());

        ImGui::Dummy(size);
        auto hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        /* background */
        if (hovered)
            list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_secondary_faded, 5.0f);

        const auto pos = ImGui::GetItemRectMin() + ImVec2(((ImGui::GetItemRectSize().x - font_size.x) / 2) + 10 + 4, 7);
        list->AddText(font, 24, pos, color_button, this->_title.c_str());


        const ImVec2 frame2 = { 20.0f, 20.0f };
        const ImVec2 pos2 = { pos.x - frame2.x - 8.0f, pos.y + 4.0f };


        list->AddImage((void*)_get_texture(this->_icon), pos2, pos2 + frame2, ImVec2(0, 0), ImVec2(1, 1), color_button);

    }


    if (ImGui::IsItemClicked()) {
        if (this->_actions.size() != 1) {
            ImGui::OpenPopup(this->_popup_name.c_str());
        }
        else {
            this->_actions[0].action();
        }
    }

    this->renderPopup();
}

const std::string& PopupButton::getTooltip() const { return this->_tooltip; }

void PopupButton::render() {

	ImDrawList* list = ImGui::GetWindowDrawList();

	/*const auto color_button = ImColor::HSV(0, 0.4f, 1, 1.f);
	const auto color_button_hover = ImColor::HSV(0, 0.25f, 1, 1.f);*/

    const ImU32 color_button = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
    const ImU32 color_button_hover = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
    const ImU32 color_secondary_faded = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);

	//ImGui::SameLine();
	ImGui::Dummy({ 24.0f, 24.0f });

	list->AddImage((void*)_get_texture(this->_icon), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

	if (ImGui::IsItemClicked()) {
        printf("opening %s", this->_popup_name.c_str());
		ImGui::OpenPopup(this->_popup_name.c_str());
	}

    this->renderPopup();
}


void PopupButton::renderPopup() {

    if (ImGui::BeginPopupModal(this->_popup_name.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImDrawList* list = ImGui::GetWindowDrawList();

        // handle close
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered() && !ImGui::IsWindowAppearing())
            ImGui::CloseCurrentPopup();

        ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
        ImGui::PushFont(font_subtitle);
        {
            int i = 1;

            for (auto& item : this->_actions) {
                const ImU32 color = (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
                // const ImU32 color_secondary = (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
                const ImU32 color_secondary_faded = (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);



                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color_secondary_faded);
                {
                    if (item.state == nullptr) {
                        if (ImGui::MenuItem(item.title.c_str(), item.description.c_str(), nullptr, !item.disabled)) item.action();
                    }
                    else {
                        bool checked = item.state();
                        // .. &checked, ..
                        if (ImGui::MenuItem(item.title.c_str(), item.description.empty() ? (checked ? "on" : "off") : (checked ? item.description.c_str() : nullptr), nullptr, !item.disabled)) item.action();
                    }
                    if (!item.tooltip.empty()) ImGui::SetItemTooltip(item.tooltip.c_str());
                    if (item.external) {
                        static auto offset = ImVec2(-8, 8);
                        static auto frame = ImVec2(18, 18);

                        const ImVec2 pos = ImVec2(ImGui::GetItemRectMax().x - frame.x, ImGui::GetItemRectMin().y) + offset;
                        list->AddImage(_get_texture("icon_outside_window"), pos, pos + frame, ImVec2(0, 0), ImVec2(1, 1), color);
                    }
                }
                ImGui::PopStyleColor();
                i ++;

                if (item.divide_next) {
                    ImGui::Spacing();
                }
            }

        }
        ImGui::PopFont();
        ImGui::PopItemFlag();

        ImGui::EndPopup();
    }
}
