#include "pch.h"

#include "Settings.h"
#include <fstream>


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
				{ "tunneling", p.options.tunneling },
			}},
			{"config", {
				{ "blocked_endpoints", p.config.blocked_endpoints },
			}},
		};

		if (p.config.tunneling_path)
		{
			j["/config/tunneling_path"_json_pointer] = p.config.tunneling_path.value();
		}
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
			"/options/tunneling"_json_pointer,

			/* vector<string> */
			"/config/blocked_endpoints"_json_pointer,
		};

		for (auto& p : compare)
			if (j.contains(p) && j_default.at(p) != j.at(p))
				result[p] = j.at(p);


		/* optional values */
		const auto p = "/config/tunneling_path"_json_pointer;
		if (j.contains(p))
		{
			result[p] = j.at(p);
		}

		return result;
	}


	void from_json(const json& j, dropship_app_settings& p) {
		if (j.contains("/options/auto_update"_json_pointer)) j.at("/options/auto_update"_json_pointer).get_to(p.options.auto_update);
		if (j.contains("/options/ping_servers"_json_pointer)) j.at("/options/ping_servers"_json_pointer).get_to(p.options.ping_servers);
		if (j.contains("/options/tunneling"_json_pointer)) j.at("/options/tunneling"_json_pointer).get_to(p.options.tunneling);

		if (j.contains("/config/blocked_endpoints"_json_pointer)) j.at("/config/blocked_endpoints"_json_pointer).get_to(p.config.blocked_endpoints);

		/* optional values */
		//if (j.contains("/config/tunneling_path"_json_pointer)) j.at("/config/tunneling_path"_json_pointer).get_to(p.config.tunneling_path);
		const auto pt = "/config/tunneling_path"_json_pointer;
		if (j.contains(pt)) {
			p.config.tunneling_path = std::make_optional<std::filesystem::path>(j.at(pt));
		}
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
		if (this->__ow2_endpoints.contains(e)) {

			auto& endpoint = this->__ow2_endpoints.at(e);

			for (auto s : endpoint.blocked_servers)
			{
				auto& server = this->__ow2_servers.at(s);
				result += server.block;
				result += ',';
			}
		}
	}

	if (!result.empty()) {
		result.pop_back();
	}

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

		(*g_firewall).tryWriteSettingsToFirewall(s, this->getAllBlockedAddresses(), this->getAppSettings().options.tunneling ? this->getAppSettings().config.tunneling_path : std::nullopt);


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


void Settings::loadServerEndpointConfig() {
	// Try to load from overrides.json file (next to executable)
	wchar_t exe_path[MAX_PATH];
	GetModuleFileNameW(NULL, exe_path, MAX_PATH);
	std::filesystem::path config_path = std::filesystem::path(exe_path).parent_path() / "overrides.json";
	
	if (std::filesystem::exists(config_path)) {
		try {
			std::ifstream config_file(config_path);
			json config = json::parse(config_file);
			
			// Override/add servers from config
			if (config.contains("servers")) {
				int overridden = 0;
				for (auto& [key, value] : config["servers"].items()) {
					if (value.contains("block")) {
						auto existing = this->__ow2_servers.find(key);
						if (existing != this->__ow2_servers.end()) {
							// Merge: update only provided fields
							existing->second.block = value["block"].get<std::string>();
						} else {
							// New entry
							this->__ow2_servers[key] = dropship::settings::unique_server{
								.block = value["block"].get<std::string>()
							};
						}
						overridden++;
					}
				}
				println("Loaded {} server overrides from overrides.json", overridden);
			}
			
			// Override/add endpoints from config
			if (config.contains("endpoints")) {
				int overridden = 0;
				for (auto& [key, value] : config["endpoints"].items()) {
					auto existing = this->__ow2_endpoints.find(key);
					
					// Parse blocked_servers if provided
					std::set<std::string> blocked_servers;
					bool has_blocked_servers = value.contains("blocked_servers");
					if (has_blocked_servers) {
						for (auto& server : value["blocked_servers"]) {
							blocked_servers.insert(server.get<std::string>());
						}
					}
					
					if (existing != this->__ow2_endpoints.end()) {
						// Merge with existing: use new value if provided, else keep existing
						this->__ow2_endpoints[key] = dropship::settings::unique_endpoint{
							.description = value.value("description", existing->second.description),
							.ip_ping = value.value("ip_ping", existing->second.ip_ping),
							.blocked_servers = has_blocked_servers ? blocked_servers : existing->second.blocked_servers
						};
					} else {
						// New entry
						this->__ow2_endpoints[key] = dropship::settings::unique_endpoint{
							.description = value.value("description", ""),
							.ip_ping = value.value("ip_ping", ""),
							.blocked_servers = blocked_servers
						};
					}
					overridden++;
				}
				println("Loaded {} endpoint overrides from overrides.json", overridden);
			}
			
		} catch (json::exception& e) {
			println("Error loading overrides.json: {}", e.what());
			println("Using hardcoded configuration as fallback");
			// Keep the hardcoded values that are already initialized
		} catch (std::exception& e) {
			println("Error reading overrides.json: {}", e.what());
			println("Using hardcoded configuration as fallback");
		}
	} else {
		println("overrides.json not found at {}, using hardcoded configuration", config_path.string());
	}
}


Settings::Settings() {

	if (!g_endpoints) throw std::runtime_error("settings depends on g_endpoints.");
	if (!g_firewall) throw std::runtime_error("settings depends on g_firewall.");

	// Load server and endpoint configuration from overrides.json (or use hardcoded fallback)
	this->loadServerEndpointConfig();

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

	// testing
	// for (auto& e : *g_endpoints) {
	// 	if (this->_dropship_app_settings.options.ping_servers)
	// 	{
	// 		e->start_pinging();
	// 	}
	// 	else
	// 	{
	// 		e->stop_pinging();
	// 	}
	// }
}

void Settings::toggleOptionTunneling() {
	this->_dropship_app_settings.options.tunneling = !this->_dropship_app_settings.options.tunneling;
	this->tryWriteSettingsToStorage();
}

void Settings::setConfigTunnelingPath(std::optional<std::filesystem::path> path)
{
	this->_dropship_app_settings.config.tunneling_path = path;
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
