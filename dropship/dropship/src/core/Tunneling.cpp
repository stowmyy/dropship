#include "pch.h"
#include "Tunneling.h"


extern std::unique_ptr<Firewall> g_firewall;
extern std::unique_ptr<Settings> g_settings;

extern ImFont* font_subtitle;


namespace core::tunneling
{
	core::tunneling::Tunneling::Tunneling()
	{
#ifdef _DEBUG
		util::timer::Timer timer("core::tunneling::Tunneling::Tunneling");
#endif

		if (!g_firewall) throw std::runtime_error("tunneling depends on g_firewall.");
		if (!g_settings) throw std::runtime_error("tunneling depends on g_settings.");

		/* auto-detect a path only if no paths are saved yet */
		if (g_settings->getAppSettings().config.tunneling_paths.empty())
		{
			const auto possible_paths = this->_queryFirewallForPossibleExePaths("Overwatch Application");

			if (possible_paths.size() == 1)
			{
				std::set<std::filesystem::path> paths;
				paths.insert(*possible_paths.begin());
				g_settings->setConfigTunnelingPaths(std::move(paths));
			}
		}
	}

	std::set<std::string> core::tunneling::Tunneling::_queryFirewallForPossibleExePaths(std::string rule_name)
	{
#ifdef _DEBUG
		util::timer::Timer timer("core::tunneling::Tunneling::_queryFirewallForPossibleExePathsUTF8Encoded");
#endif
		std::set<std::string> result;
		{
			util::win_firewall::forFirewallRulesWithName(rule_name, [&result](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& rules) {

				CComBSTR application_name;
				if (SUCCEEDED(FwRule->get_ApplicationName(&application_name)) && application_name)
				{

					std::wstring ws(application_name, SysStringLen(application_name));

					// ws.includes '_retail_' ??

					//if is valid
					std::filesystem::path path (ws);
					if (std::filesystem::exists(path))
					{
						//if was used in the last 30 days?
						//const auto write_time = std::filesystem::last_write_time(ws);
						result.insert(path.string());
					}

				}
			});
		}

		/* test multiple options */
		/*result.insert(("S:\\Overwatch\\_beta_\\Overwatch.exe"));
		result.insert(("C:\\Program Files (x86)\\Overwatch\\Overwatch.exe"));
		result.insert(("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Overwatch\\Overwatch.exe"));*/

		/* test no options */
		/*std::set<std::string> h;
		return h;*/

		return result;
	}

	void core::tunneling::Tunneling::openExplainer()
	{
		this->_open_explainer_next_frame = true;
	}

	void core::tunneling::Tunneling::render()
	{
#ifdef _DEBUG
		ImGui::Begin("debug");
		if (ImGui::CollapsingHeader("tunneling", ImGuiTreeNodeFlags_None))
		{
			if (!g_settings->getAppSettings().config.tunneling_paths.empty())
			{
				for (auto& p : g_settings->getAppSettings().config.tunneling_paths) {
					ImGui::Text("path: %s", p.string().c_str());
				}
			}
			else
			{
				ImGui::Text("NO PATHS SET");
			}

			{
				if (ImGui::Button("PRINT UNIQUE", { ImGui::GetContentRegionAvail().x, 0 })) {
					auto x = this->_queryFirewallForPossibleExePaths("Overwatch Application");
					for (auto& f : x)
					{
						std::println("{}", f);
					}
				}
			}

			{
				if (ImGui::Button("set tunneling path", { ImGui::GetContentRegionAvail().x, 0 })) {
					auto path_wstring = util::win_filesystem::prompt_file();
					if (path_wstring) {
						auto paths = g_settings->getAppSettings().config.tunneling_paths;
						paths.insert(path_wstring.value());
						g_settings->setConfigTunnelingPaths(std::move(paths));
					}
					else
					{
						std::println("failed");
					}
				}
				ImGui::SetItemTooltip("todo");

				if (ImGui::Button("clear all paths", { ImGui::GetContentRegionAvail().x, 0 })) {
					g_settings->setConfigTunnelingPaths({});
				}
			}
		}
		ImGui::End();
#endif

		static const auto list = ImGui::GetWindowDrawList();
		static const auto& style = ImGui::GetStyle();


		static const std::string popup_name { "tunneling" };
		static const std::string popup_name_continue { "find Overwatch.exe" };
		static bool not_ignored { true };

		const bool options_tunneling = g_settings->getAppSettings().options.tunneling;
		const auto& current_paths = g_settings->getAppSettings().config.tunneling_paths;
		const bool config_tunneling_paths_empty = current_paths.empty();

		/* if tunneling popup was ignored, unignore if tunneling is toggled again*/
		static bool prev_options_tunneling { options_tunneling };
		if (options_tunneling && !prev_options_tunneling)
		{
			not_ignored = true;
		}
		prev_options_tunneling = options_tunneling;


		const bool tunneling_active = options_tunneling && !config_tunneling_paths_empty;

		/* tunneling indicator */
		{
			static const std::string text1{ "tunneling: " };
			static const auto width_text1{ font_subtitle->CalcTextSizeA(14, FLT_MAX, 0.0f, text1.c_str()) };
			static const auto color_text1{ ImGui::ColorConvertFloat4ToU32({ .8f, .8f, .8f, style.Alpha }) };

			static const auto color_on{ ImColor::HSV(0.4f, 0.44f, 1.f) };
			static const auto color_off{ ImColor::HSV(0.9666f, 0.44f, 1.f) };

			const std::string text2 = tunneling_active ? "active" : "not active";
			const auto width_text = font_subtitle->CalcTextSizeA(14, FLT_MAX, 0.0f, (text1 + text2).c_str());
			auto& color_text2 = tunneling_active ? color_on : color_off;

			const auto original_pos = ImGui::GetCursorScreenPos() + ImVec2((ImGui::GetContentRegionAvail().x - width_text.x) / 2.f, 0.f);

			ImGui::SetCursorScreenPos(original_pos);
			list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos(), color_text1, text1.c_str());

			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(width_text1.x, 0.f));
			list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos(), color_text2, text2.c_str());

			ImGui::SetCursorScreenPos(original_pos);
			ImGui::Dummy(width_text);
			if (ImGui::IsItemHovered()) ImGui::SetItemTooltip(current_paths.empty() ? "Configure in options" : current_paths.begin()->string().c_str());
		}

		/* open configuration if tunneling is enabled, no paths set, and not dismissed */
		if (options_tunneling && config_tunneling_paths_empty && not_ignored && !ImGui::IsWindowAppearing())
		{
			ImGui::OpenPopup(popup_name.c_str());
		}
		/* open explainer on next frame when requested from Dashboard */
		if (this->_open_explainer_next_frame)
		{
			ImGui::OpenPopup(popup_name.c_str());
			this->_open_explainer_next_frame = false;
		}
		/* reset not_ignored when paths become non-empty (so next time they clear, popup re-opens) */
		if (!config_tunneling_paths_empty)
		{
			not_ignored = true;
		}

		/* configuration popup */
		{
			ImVec2 center = ImGui::GetWindowViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize({ 400, 0 });
			if (ImGui::BeginPopupModal(popup_name.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextWrapped("Tunneling allows you to block servers per-application instead of globally");
				ImGui::Spacing();
				ImGui::TextWrapped("This prevents servers in other games and apps from becoming unintentionally blocked");
				ImGui::Spacing();
				ImGui::TextWrapped("To enable tunneling, all you need to do is locate Overwatch.exe");

				const auto n_buttons{ 2 };
				const ImVec2 button{ (ImGui::GetContentRegionAvail().x - ((style.ItemSpacing.x / 1.f) * (n_buttons - 1))) / n_buttons, 0 };

				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 0.5f });
				ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0.04f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0.09f });
				{
					if (ImGui::Button("Skip for now", button)) {
						not_ignored = false;
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::PopStyleColor(4);

				ImGui::SameLine();

				if (ImGui::Button("Continue", button)) {
					not_ignored = false;
					ImGui::CloseCurrentPopup();
					this->_open_path_picker_next_frame = true;
				}

				ImGui::EndPopup();
			}
		}

		/* open path picker on next frame after Continue */
		if (this->_open_path_picker_next_frame)
		{
			ImGui::OpenPopup(popup_name_continue.c_str());
			this->_open_path_picker_next_frame = false;
		}

		/* path picker popup (separate, not nested) */
		{
			ImVec2 center = ImGui::GetWindowViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize({ 750, 0 });
			if (ImGui::BeginPopupModal(popup_name_continue.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
			{
				static std::set<std::string> possible_paths;
				static std::set<std::string> auto_detected_paths;
				static int selected;
				if (ImGui::IsWindowAppearing()) {
					auto_detected_paths = Tunneling::_queryFirewallForPossibleExePaths("Overwatch Application");
					possible_paths = auto_detected_paths;
					for (auto& p : g_settings->getAppSettings().config.tunneling_paths) {
						possible_paths.insert(p.string());
					}
					selected = possible_paths.size() > 0 ? 0 : -1;
				}

				ImGui::TextWrapped("Select the Overwatch.exe to use for per-application blocking.");
				ImGui::Spacing();

				int n = 0;
				for (auto it = possible_paths.begin(); it != possible_paths.end();) {
					auto& p = *it;
					const ImU32 color = (ImU32)ImColor::HSV(1.0f - ((n + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
					const ImU32 color_hover = (ImU32)ImColor::HSV(1.0f - ((n + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
					const ImU32 color_secondary_faded = (ImU32)ImColor::HSV(1.0f - ((n + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);

					ImGui::PushStyleColor(ImGuiCol_Header, color_hover);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color_secondary_faded);
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);

					if (ImGui::Selectable(p.c_str(), selected == n, ImGuiSelectableFlags_DontClosePopups, ImVec2(ImGui::GetContentRegionAvail().x - 110, 0)))
							{
								selected = n;
							}
							if (selected == n && ImGui::IsItemHovered()) ImGui::SetItemTooltip(p.c_str());
							ImGui::PopStyleColor(3);

							ImGui::SameLine();

							ImGui::PushStyleColor(ImGuiCol_Text, color);
							ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0.0f });
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0.1f });
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0.2f });

							bool is_auto = (auto_detected_paths.find(p) != auto_detected_paths.end());
							if (is_auto) ImGui::BeginDisabled();
							if (ImGui::Button(std::format("Remove##{}", n).c_str(), { 100, 0 })) {
						auto paths_new = g_settings->getAppSettings().config.tunneling_paths;
						paths_new.erase(std::filesystem::path(p));
						g_settings->setConfigTunnelingPaths(std::move(paths_new));
						possible_paths.erase(it++);
						selected = possible_paths.size() > 0 ? 0 : -1;
						continue;
					}
					if (is_auto) {
						ImGui::EndDisabled();
						ImGui::SetItemTooltip("Auto-detected path — use \"Add another\" to save a custom path");
					}
					ImGui::PopStyleColor(4);

					++it;
					n++;
				}

				ImGui::Spacing();

				if (ImGui::Button(possible_paths.size() > 0 ? "Add another .." : "Add path ..", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
				{
					auto path = util::win_filesystem::prompt_file();
					if (path) {
						auto paths_new = g_settings->getAppSettings().config.tunneling_paths;
						paths_new.insert(path.value());
						g_settings->setConfigTunnelingPaths(std::move(paths_new));
						possible_paths.insert(path.value().string());
						auto it2 = possible_paths.find(path.value().string());
						selected = (int)std::distance(possible_paths.begin(), it2);
					}
				}

				ImGui::Spacing();

				{
					const auto n_buttons{ 2 };
					const ImVec2 button{ (ImGui::GetContentRegionAvail().x - ((style.ItemSpacing.x / 1.f) * (n_buttons - 1))) / n_buttons, 0 };

					ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 0.5f });
					ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0.04f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0.09f });
					{
						if (ImGui::Button("Skip for now", button)) {
							not_ignored = false;
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::PopStyleColor(4);

					ImGui::SameLine();

					if (selected == -1) ImGui::BeginDisabled();
					if (ImGui::Button("Done", button)) {
						std::set<std::filesystem::path> paths;
						for (auto& p : possible_paths) {
							paths.insert(std::filesystem::path(p));
						}
						g_settings->setConfigTunnelingPaths(std::move(paths));
						not_ignored = false;
						ImGui::CloseCurrentPopup();
					}
					if (selected == -1) ImGui::EndDisabled();
				}

				ImGui::EndPopup();
			}
		}
	}
}
