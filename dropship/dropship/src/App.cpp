#include "pch.h"

#include "App.h"

extern std::unique_ptr<Updater> g_updater;
extern std::unique_ptr<Dashboard> g_dashboard;
extern std::unique_ptr<Firewall> g_firewall;
extern std::unique_ptr<Settings> g_settings;

#ifdef _DEBUG
	extern std::unique_ptr<Debug> g_debug;
#endif

void App::render(bool* p_open) {

#ifdef _DEBUG
	if (g_settings) {
		ImGui::Begin("debug");

		ImGui::Text(((json) (*g_settings).getAppSettings()).dump(4).c_str());
		ImGui::End();
	}
	if (g_debug) {
		(*g_debug).render();
	}
#endif

	//static const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
	static const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::SetNextWindowSize(ImVec2(418, 0), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(400, 40), ImGuiCond_Once);
	ImGui::Begin("dropship", p_open, window_flags);

	{
		/* draw */

		if (g_updater) (*g_updater).render();
		if (g_dashboard) (*g_dashboard).render();
		if (g_firewall) (*g_firewall).render();
		if (g_settings) (*g_settings).render();
	}

	ImGui::End();
}


extern ImFont* font_subtitle;

void App::renderError(std::string error) {

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
	//ImGui::SetNextWindowSize(ImVec2(418, 0), ImGuiCond_Once);
	ImGui::Begin("failed", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);


	ImGui::TextColored(ImVec4{ 1, 0.6f, 0.6f, 1 }, "app crashed");
	ImGui::SameLine();
	ImGui::Text("::[");

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushFont(font_subtitle);
	ImGui::SeparatorText("error");
	ImGui::TextColored(ImVec4{ 1, 0.6f, 0.6f, 1 }, error.c_str());

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::SeparatorText("get help");
	//ImGui::Text("https://discord.stormy.gg/");
	//ImGui::Text("@stormyy_ow");


	/* discord */
	{
		static const ImVec4 color_button = { 0.345f, 0.396f, 0.949f, 1.0f };
		static const ImVec4 color_button_hover = { 0.345f, 0.396f, 0.949f, 0.9f };
		static const ImVec4 color_button_active = { 0.345f, 0.396f, 0.949f, 0.8f };

		ImGui::PushStyleColor(ImGuiCol_Button, color_button);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_button_active);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_button_hover);
		{
			if (ImGui::Button("discord")) {
				// system("start https://discord.stormy.gg");
				system("start https://discord.gg/QYrF8CVhbC");
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
			// ImGui::SetItemTooltip("https://discord.stormy.gg/");
			ImGui::SetItemTooltip("https://discord.gg/QYrF8CVhbC");
			ImGui::PopStyleColor(1);

		}
		ImGui::PopStyleColor(3);
	}

	ImGui::SameLine();

	/* twitter */
	{
		static const ImVec4 color_button = { 0.114f, 0.631f, 0.949f, 1.0f };
		static const ImVec4 color_button_hover = { 0.114f, 0.631f, 0.949f, 0.9f };
		static const ImVec4 color_button_active = { 0.114f, 0.631f, 0.949f, 0.8f };

		ImGui::PushStyleColor(ImGuiCol_Button, color_button);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_button_active);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_button_hover);
		{
			if (ImGui::Button("@stormyy_ow")) {
				system("start https://twitter.com/stormyy_ow");
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
			ImGui::SetItemTooltip("https://twitter.com/stormyy_ow");
			ImGui::PopStyleColor(1);

		}
		ImGui::PopStyleColor(3);
	}

	ImGui::PopFont();

	ImGui::PopStyleColor(2);

	ImGui::Spacing();

	ImGui::End();
}
