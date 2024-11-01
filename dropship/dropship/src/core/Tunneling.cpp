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

		/* if tunneling is enabled but no path defined, attempt automatic tunneling */
		if (g_settings->getAppSettings().options.tunneling && !g_settings->getAppSettings().config.tunneling_path.has_value())
		{
			const auto possible_paths = this->_queryFirewallForPossibleExePaths("Overwatch Application");

			/* if there's only one good option, set path */
			if (possible_paths.size() == 1)
			{
				const auto path = *(possible_paths.begin());
				g_settings->setConfigTunnelingPath(std::make_optional(path));
			}

			/* otherwise open list */
		}

		//wprintf(g_settings->getAppSettings().config.tunneling_path.value_or(TEXT("NONE")).c_str());
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

	void core::tunneling::Tunneling::render()
	{
#ifdef _DEBUG
		ImGui::Begin("debug");
		if (ImGui::CollapsingHeader("tunneling", ImGuiTreeNodeFlags_None))
		{
			if (g_settings->getAppSettings().config.tunneling_path)
			{
				ImGui::Text("path: %s", g_settings->getAppSettings().config.tunneling_path.value().c_str());
			}
			else
			{
				ImGui::Text("PATH NOT SET");
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

						//std::string path_utf8encoded = util::utf8::utf8_encode(path_wstring.value());
						//std::println("{}", path_utf8encoded);

						//wprintf(path_wstring.value().c_str());
						//wprintf(util::utf8::utf8_decode(path_utf8encoded).c_str());

						//g_settings->setConfigTunnelingPath(std::make_optional(path_utf8encoded));
						g_settings->setConfigTunnelingPath(std::make_optional(path_wstring.value()));
					}
					else
					{
						std::println("failed");
					}
				}
				ImGui::SetItemTooltip("todo");

				if (ImGui::Button("clear tunneling path", { ImGui::GetContentRegionAvail().x, 0 })) {
					g_settings->setConfigTunnelingPath(std::nullopt);
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
		const bool config_tunneling_path = g_settings->getAppSettings().config.tunneling_path.has_value();

		/* if tunneling popup was ignored, unignore if tunneling is toggled again*/
		static bool prev_options_tunneling { options_tunneling };
		if (options_tunneling && !prev_options_tunneling)
		{
			not_ignored = true;
		}
		prev_options_tunneling = options_tunneling;


		const bool tunneling_active = options_tunneling && config_tunneling_path;

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
			//ImGui::SetItemTooltip("Configure in options");
			//if (ImGui::IsItemHovered()) ImGui::SetItemTooltip(util::utf8::utf8_encode(g_settings->getAppSettings().config.tunneling_path.value_or(TEXT("Configure in options"))).c_str());
			if (ImGui::IsItemHovered()) ImGui::SetItemTooltip(g_settings->getAppSettings().config.tunneling_path ? g_settings->getAppSettings().config.tunneling_path.value().string().c_str() : "Configure in options");
		}

		/* open configuration if tunneling is enabled but there is no path defined */
		if (options_tunneling && !config_tunneling_path && not_ignored && !ImGui::IsWindowAppearing())
		{
			ImGui::OpenPopup(popup_name.c_str());
		}

		/* configuration popup */
		{
			ImVec2 center = ImGui::GetWindowViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize({ 400, 0 });
			if (ImGui::BeginPopupModal(popup_name.c_str(), &not_ignored, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextWrapped("Tunneling allows you to block servers per-application instead of globally");
				ImGui::Spacing();
				ImGui::TextWrapped("This prevents servers in other games and apps from becoming unintentionally blocked");
				ImGui::Spacing();
				ImGui::TextWrapped("To enable tunneling, all you need to do is locate Overwatch.exe");

				const auto n_buttons{ 2 };
				const ImVec2 button{ (ImGui::GetContentRegionAvail().x - ((style.ItemSpacing.x / 1.f) * (n_buttons - 1))) / n_buttons, 0 };

				/* choice 00 */
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 0.5f });
				ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0.04f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0.09f });
				{
					if (ImGui::Button("Skip for now", button)) {
						not_ignored = false;
					}
				}
				ImGui::PopStyleColor(4);

				ImGui::SameLine();

				/* choice 02 */
				if (ImGui::Button("Continue", button)) {
					ImGui::OpenPopup(popup_name_continue.c_str());
				}

				/* configuration popup continued (part 2) */
				{
					ImVec2 center = ImGui::GetWindowViewport()->GetCenter();
					ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
					ImGui::SetNextWindowSize({ 666, 0 });
					if (ImGui::BeginPopupModal(popup_name_continue.c_str(), &not_ignored, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
					{

						static std::optional<std::string> error_text{ std::nullopt };

						static auto possible_paths{ this->_queryFirewallForPossibleExePaths("Overwatch Application") };
						static int selected{ possible_paths.size() > 0 ? 0 : -1 };

						//ImGui::Text("selected %d", selected);

						ImGui::TextWrapped("Please choose the correct Overwatch.exe from your computer's filesystem.");

						ImGui::Spacing();

						int n = 0;
						for (auto& p : possible_paths) {

							const ImU32 color = (ImU32)ImColor::HSV(1.0f - ((n + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
							const ImU32 color_hover = (ImU32)ImColor::HSV(1.0f - ((n + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
							const ImU32 color_secondary_faded = (ImU32)ImColor::HSV(1.0f - ((n + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);
							static const auto transparent = ImVec4(0.f, 0.f, 0.f, 0.f);

							ImGui::PushStyleColor(ImGuiCol_Header, color_hover);
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color_secondary_faded);
							ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);

							//ImGui::Bullet(); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
							if (ImGui::Selectable(p.c_str(), selected == n, ImGuiSelectableFlags_DontClosePopups))
							{
								selected = n;
							}
							if (selected == n && ImGui::IsItemHovered()) ImGui::SetItemTooltip(p.c_str());
							n++;

							ImGui::PopStyleColor(3);
						}

						//std::println("file exists: {}", std::filesystem::exists("S:\\Overwatch\\_retail_\\Overwatch.exe") ? "true" : "false");
						//std::println("file exists: {}", std::filesystem::exists("S:\\overwatch\\_retail_\\overwatch.exe") ? "true" : "false");

						ImGui::Spacing();
						//wprintf(L"C:\\Users\\stormy\\AppData\\Local\\Temp\\べてのファ 況ロ");

						if (ImGui::Button(possible_paths.size() > 0 ? "Add another .." : "Add path ..", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
						{
							auto path = util::win_filesystem::prompt_file();
							if (path) {

								std::println("{}", path.value().string());

								//wprintf(path_wstring.value().c_str());
								//wprintf(util::utf8::utf8_decode(path_utf8encoded).c_str());

								//g_settings->setConfigTunnelingPath(std::make_optional(path_utf8encoded));

								int size_before = possible_paths.size();
								possible_paths.insert(path.value().string());
								if (possible_paths.size() != size_before)
								{
									selected = std::max(0, (int)possible_paths.size() - 1);
								};
								error_text.reset();
							}
							else
							{
								error_text = std::make_optional("Adding another file didn't work.");
							}
						}

						if (error_text)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)ImColor::HSV(1.0f - ((0 + 1) / 32.0f), 0.5f, 1.0f, 1.0f));
							//ImGui::Indent();
							ImGui::TextUnformatted(error_text.value().c_str());
							ImGui::PopStyleColor();
						}

						ImGui::Spacing();

						{
							const auto n_buttons{ 2 };
							const ImVec2 button{ (ImGui::GetContentRegionAvail().x - ((style.ItemSpacing.x / 1.f) * (n_buttons - 1))) / n_buttons, 0 };

							/* choice 00 */
							ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 0.5f });
							ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0.0f });
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0.04f });
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0.09f });
							{
								if (ImGui::Button("Skip for now", button)) {
									not_ignored = false;
								}
							}
							ImGui::PopStyleColor(4);

							ImGui::SameLine();

							/* choice 02 */

							if (selected == -1) ImGui::BeginDisabled();
							if (ImGui::Button("Done", button)) {

								g_settings->setConfigTunnelingPath(*std::next(possible_paths.begin(), selected));
								not_ignored = false; // FIXME
							}
							if (selected == -1) ImGui::EndDisabled();
						}

						ImGui::EndPopup();
					}
				}

				ImGui::EndPopup();
			}
		}
	}
}
