#pragma once


#include "images.h"

extern ImFont* font_title;
extern ImFont* font_subtitle;
extern ImFont* font_text;

struct Action {
	std::string title;
	std::string description;
	std::string tooltip;
	std::function<void()> action;
	std::function<bool()> state = { nullptr };

	bool external = { false };
	bool disabled = { false };

	bool divide_next = { false };
};

class PopupButton
{
public:
	PopupButton(std::string title, std::string icon, std::vector<Action> const actions, int i = 0, std::string tooltip = "");
	void render();
	void render2(const ImVec2& size = ImVec2(90, 28));

	const std::string& getTooltip() const;

	//static void renderPopups();

private:
	std::string _title;
	std::string _icon;
	std::string _tooltip;
	//const ImVec2 &size;
	int _i;

	std::vector<Action> const _actions;

	std::string _popup_name;

	void renderPopup();
};

