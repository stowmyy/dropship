#include "pch.h"

#include "core/Dashboard.h"

extern std::unique_ptr<Settings> g_settings;

Dashboard::Dashboard()
{

	if (!g_settings) throw std::runtime_error("dashboard depends on g_settings.");

	auto i = 1;

	this->header_actions.push_back(std::make_unique<PopupButton>("socials", "icon_heart", std::move(std::vector<Action>({
		{
			.title = "discord\n ",
			.description = "suggestions\nhelp",
			.action = []() { system("start https://discord.stormy.gg"); },
			.external = true,
		},
		{
			.title = "twitch",
			.description = "stormyy_ow",
			.action = []() { system("start https://twitch.tv/stormyy_ow"); },
			.external = true,
		},
		{
			.title = "twitter",
			.description = "stormyy_ow",
			.action = []() { system("start https://twitter.com/stormyy_ow"); },
			.external = true,
		},
		{
			.title = "github\n ",
			.description = "guide\ncode",
			.action = []() { system("start https://github.com/stowmyy/dropship"); },
			.external = true,
		},
	})), i++));

	this->header_actions.push_back(std::make_unique<PopupButton>("options", "icon_options", std::move(std::vector<Action>({
		{
			.title = "auto update",
			.description = "",
			.action = [this]() { (*g_settings).toggleOptionAutoUpdate(); },
			.state = [this]() { return (*g_settings).getAppSettings().options.auto_update; },
			.external = false,
		},
		{
			.title = "ping servers",
			//.description = "constantly",
			.description = "",
			.action = [this]() { (*g_settings).toggleOptionPingServers(); },
			.state = [this]() { return (*g_settings).getAppSettings().options.ping_servers; },
			.external = false,

			//.disabled = true,
			.divide_next = true,
		},
		{
			.title = "network settings",
			.tooltip = "windowsdefender://network",
			.action = []() { system("start windowsdefender://network"); },
			.external = true,
		},
		{
			.title = "firewall rules",
			.tooltip = "wf.msc",
			.action = []() { system("start wf.msc"); },
			.external = true,
		},
	})), i++));

	this->header_actions.push_back(std::make_unique<PopupButton>("unblock", "icon_clock_undo", std::move(std::vector<Action>({
		{
			.action = [this]() { (*g_settings).unblockAll(); },
		}
	})), i++, "Unblocks all servers\n\nIf you are ever failing to connect\nto a server, quickly clicking this\nwill prevent a competitive ban"));

	//(*(this->header_actions)).insert((*(this->header_actions)).end(), {
	//	std::make_shared<PopupButton>("socials", "icon_heart"),
	//	std::make_shared<PopupButton>("options", "icon_options")
	//});

	//for (int i = 0; i < (*header_actions).size(); i++) {
	//	printf("xx %d\n", i);
	//}

}

Dashboard::~Dashboard()
{
	/*for (auto& b : (this->header_actions)) {
		delete b;
	}*/
}

