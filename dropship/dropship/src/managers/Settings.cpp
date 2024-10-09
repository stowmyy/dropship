#include "pch.h"

#include "Settings.h"


extern std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
extern std::unique_ptr<Firewall> g_firewall;
extern std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;

extern ImFont* font_subtitle;

namespace dropship::settings {
	void to_json(json& j, const dropship_app_settings& p) {

		

		j = json {
			{"options", {
				{ "auto_update", p.options.auto_update },
				{ "ping_servers", p.options.ping_servers },
			}},
			{"config", {
				{ "blocked_endpoints", p.config.blocked_endpoints },
			}},
		};
	}
	
	json strip_diff_dropship_app_settings(const json& j_default, const json& j) {

		json result;

		// web https://json.nlohmann.me/features/json_pointer/

		/* types that support equality */
		const std::array /*<json::json_pointer, _>*/ compare
		{
			/* bool */
			"/options/auto_update"_json_pointer,
			"/options/ping_servers"_json_pointer,

			/* vector<string> */
			"/config/blocked_endpoints"_json_pointer,
		};

		for (auto& p : compare)
			if (j.contains(p) && j_default.at(p) != j.at(p))
				result[p] = j.at(p);

		return result;
	}


	void from_json(const json& j, dropship_app_settings& p) {
		if (j.contains("/options/auto_update"_json_pointer)) j.at("/options/auto_update"_json_pointer).get_to(p.options.auto_update);
		if (j.contains("/options/ping_servers"_json_pointer)) j.at("/options/ping_servers"_json_pointer).get_to(p.options.ping_servers);

		if (j.contains("/config/blocked_endpoints"_json_pointer)) j.at("/config/blocked_endpoints"_json_pointer).get_to(p.config.blocked_endpoints);
	}
}




const dropship::settings::dropship_app_settings& Settings::getAppSettings()
{
	return this->_dropship_app_settings;
};


std::string Settings::getAllBlockedAddresses() {

	std::string result;

	if (this->_dropship_app_settings.config.blocked_endpoints.empty()) return result;

	std::set<std::string> blocked_servers;

	/* note this causes weird crash. */
	/*for (auto& e : this->_dropship_app_settings.config.blocked_endpoints)
	{
		auto& endpoint = this->__ow2_endpoints.at(e);

		for (auto s : endpoint.blocked_servers)
		{
			blocked_servers.insert(s);
		}
	}

	for (auto& s : blocked_servers)
	{
		auto& server = this->__ow2_servers.at(s);
		result += server.block;
		result += ',';
	}*/

	//if (!result.empty())
	//{
	//	result = result.substr(0, result.length() - 1);
	//}

	for (auto& e : this->_dropship_app_settings.config.blocked_endpoints)
	{
		auto& endpoint = this->__ow2_endpoints.at(e);

		for (auto s : endpoint.blocked_servers)
		{
			auto& server = this->__ow2_servers.at(s);
			result += server.block;
			result += ',';
		}
	}


	result.pop_back();

	// throws errors
	//println("blocking {}", result);

	return result;

}



std::optional<json> Settings::readStoragePatch__win_firewall() {

#ifdef _DEBUG
	util::timer::Timer timer("readStoragePatch__win_firewall");
#endif

	std::optional<json> result = std::nullopt;

	auto loaded_settings = (*g_firewall).tryFetchSettingsFromFirewall();
	if (loaded_settings)
	{

		try {
			//result = std::make_optional<json>(loaded_settings.value());
			result = std::make_optional<json>(json::from_msgpack(loaded_settings.value()));
			println("loaded patch: {}", result.value().dump(4));
		}
		catch (json::exception& e) {
			println("json error: {}", e.what());
		}
	}

	return result;
}

void Settings::tryLoadSettingsFromStorage() {
	auto loaded_settings = this->readStoragePatch();
	if (loaded_settings) {

		try {
			json settings = this->__default_dropship_app_settings;

			//settings.merge_patch(loaded_settings.value());

			/* merge obects: false */
			settings.update(loaded_settings.value(), false);

			this->_dropship_app_settings = settings;
		}
		catch (json::exception& e) {
			println("tryLoadSettingsFromStorage() json error: {}", e.what());
		}
	}
}


/* note: windows firewall has a 1024 description */
void Settings::tryWriteSettingsToStorage(bool force) {

#ifdef _DEBUG
	util::timer::Timer timer("tryWriteSettingsToStorage");
#endif

	const auto game_open = g_window_watcher && (*g_window_watcher).isActive();
	if (game_open && !force)
	{
		//this->_waiting_for_config_write = true;
		auto any = false;
		for (auto& e : (*g_endpoints)) {
			if ((*e).getBlockDesired() != (*e)._getBlockedState()) { any = true; };
		}
		this->_waiting_for_config_write = any;
	}
	else if (force && this->_waiting_for_config_write)
	{
		this->_waiting_for_config_write = false;
	}

	if (!this->_waiting_for_config_write || force)
	{
		// calculate diff
		/*auto diff = json::diff(this->__default_dropship_app_settings, this->_dropship_app_settings);
		println("diff: {}", diff.dump(4));*/

		json stripped = dropship::settings::strip_diff_dropship_app_settings(this->__default_dropship_app_settings, this->_dropship_app_settings);

		if (stripped.is_null())
		{
			stripped = this->__default_dropship_app_settings;
		}

		// throws error
		//println("writing: {}", stripped.dump(4));

		/* note: diff calcuated in to_json defined above */
		auto packed = json::to_msgpack(stripped);
		std::string s(packed.begin(), packed.end());

		(*g_firewall).tryWriteSettingsToFirewall(s, this->getAllBlockedAddresses());


		// TODO if not failed
		for (auto& endpoint : (*g_endpoints)) {
			(*endpoint)._setBlockedState(this->_dropship_app_settings.config.blocked_endpoints.contains((*endpoint).getTitle()));
		}


		//std::basic_string_view<uint8_t> packed_sv (packed.data(), packed.size());
		//std::string packed_s{ packed_sv };
		//auto s = std::string_view<uint8_t>(packed);

		//(*g_firewall).tryWriteSettingsToFirewall(s);

		//printf("packed: <%s>\n", s.c_str());
	}

	else
	{
		println("waiting for game close");
	}

}

//json mergeSettings(const json& a, const json& b) {
//
//	println("a {}", a.dump(4));
//	println("b {}", b.dump(4));
//
//	throw std::runtime_error("xd");
//
//	/* calculate diff */
//	json merge_patch = a;
//
//	//merge_patch.merge_patch(b);
//	for (auto& b : a.flatten()) {
//		println("ac: {}", b.dump(4));
//	}
//
//	println("merge: {}", merge_patch.dump(4));
//
//	//a.merge_patch(b);
//
//	return a;
//}



std::optional<json> Settings::readStoragePatch() {
	return this->readStoragePatch__win_firewall();
}


Settings::Settings() {

	if (!g_endpoints) throw std::runtime_error("settings depends on g_endpoints.");
	if (!g_firewall) throw std::runtime_error("settings depends on g_firewall.");


	this->tryLoadSettingsFromStorage();

	/*
		1. grab settings from firewall manager
		2. if failed, show failed and use default settings.
	
	*/

	(*g_endpoints).reserve(this->__ow2_endpoints.size());

	for (auto& [key, e] : this->__ow2_endpoints)
	{
		auto blocked = this->_dropship_app_settings.config.blocked_endpoints.contains(key);

		(*g_endpoints).push_back(std::make_shared<Endpoint2>(
			key,
			e.description,
			this->_dropship_app_settings.options.ping_servers ? e.ip_ping : "",
			blocked
		));
	}


	/*std::cout << "__ow2_ranges: " << __ow2_servers.dump(4) << std::endl;
	std::cout << "__ow2_ranges: " << __ow2_ranges.dump(4) << std::endl;
	std::cout << "__ow2_endpoints: " << __ow2_endpoints.dump(4) << std::endl;
	std::cout << "__default: " << __default.dump(4) << std::endl;*/

}

Settings::~Settings() {


	printf("destructor");
}

void Settings::unblockAll() {
	for (auto& endpoint : (*g_endpoints)) {
		(*endpoint).setBlockDesired(false);
	}
	this->_dropship_app_settings.config.blocked_endpoints.clear();
	this->tryWriteSettingsToStorage(true); // force
}

void Settings::toggleOptionAutoUpdate() {
	this->_dropship_app_settings.options.auto_update = !this->_dropship_app_settings.options.auto_update;
	this->tryWriteSettingsToStorage();
}

void Settings::toggleOptionPingServers() {
	this->_dropship_app_settings.options.ping_servers = !this->_dropship_app_settings.options.ping_servers;
	this->tryWriteSettingsToStorage();
}


// todo std::unordered_set
void Settings::addBlockedEndpoint(std::string endpoint_title) {

	auto [_position, hasBeenInserted] = this->_dropship_app_settings.config.blocked_endpoints.insert(endpoint_title);

	if (hasBeenInserted)
	{
		this->tryWriteSettingsToStorage();
	}

}

// todo std::unordered_set
void Settings::removeBlockedEndpoint(std::string endpoint_title) {

	auto const num_removed = this->_dropship_app_settings.config.blocked_endpoints.erase(endpoint_title);

	if (num_removed > 0)
	{
		this->tryWriteSettingsToStorage();
	}

}

void Settings::render() {
	if (g_window_watcher) {

		const auto game_open = (*g_window_watcher).isActive();

		// pause config writes when game is open
		if (game_open) {
			//this->_waiting_for_config_write = true;
		}

		// trigger a write when game is closed
		else
		{
			if (this->_waiting_for_config_write)
			{
				this->_waiting_for_config_write = false;
				this->tryWriteSettingsToStorage();
			}
		}

	}
}

void Settings::renderWaitingStatus()
{
	if (g_window_watcher) {

		if (this->_waiting_for_config_write)
		{
			//static auto& style = ImGui::GetStyle();
			ImDrawList* list = ImGui::GetWindowDrawList();

			static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });

			static const auto color = ImGui::ColorConvertFloat4ToU32({ .4f, .4f, .4f, 1.0f });
			// static const auto color_2 = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0.6f });
			static const std::string text = "GAME RESTART REQUIRED";
			//static const std::string text = "GAME SHUT DOWN REQUIRED";

			static const auto font = font_subtitle;
			static const auto font_size = font->CalcTextSizeA(font_subtitle->FontSize, FLT_MAX, 0.0f, text.c_str());


			ImGui::Dummy({ ImGui::GetContentRegionAvail().x, font_size.y + 16 });

			list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 5.0f);

			{
				static const auto color = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.09f });
				const auto pos = ImGui::GetItemRectMin();

				static const auto image = _get_image("background_diagonal");

				list->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
				list->AddImage(image.texture, pos, pos + ImVec2((float)image.width, (float)image.height), ImVec2(0, 0), ImVec2(1, 1), color);
				list->PopClipRect();
			}

			const auto pos = ImGui::GetItemRectMin() + ImVec2((ImGui::GetItemRectSize().x - font_size.x) / 2, 8 - 2);
			list->AddText(font_subtitle, font_subtitle->FontSize, pos, white, text.c_str());

		}

		/*static auto& style = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		ImGui::TextWrapped("Click Unblock if not connecting");
		ImGui::PopStyleColor();*/
	}

	else {
		ImGui::TextDisabled("Do not block servers with game open");
	}
}
