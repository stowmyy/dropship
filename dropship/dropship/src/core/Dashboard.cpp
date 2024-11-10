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
			.tooltip = "default: off\n\nwhen a new version is available:\n\ton - update app on launch \n\toff - an update button will appear on the dashboard",
			.action = [this]() { (*g_settings).toggleOptionAutoUpdate(); },
			.state = [this]() { return (*g_settings).getAppSettings().options.auto_update; },
			.external = false,
		},
		{
			.title = "ping servers",
			//.description = "constantly",
			.description = "",
			.tooltip = "default: on\n\n(visual) estimate latency by pinging servers:\n\ton - every few seconds\n\toff - never",
			.action = [this]() { (*g_settings).toggleOptionPingServers(); },
			.state = [this]() { return (*g_settings).getAppSettings().options.ping_servers; },
			.external = false,

			//.disabled = true,
			.divide_next = true,
		},
		{
			.title = "tunneling",
			.tooltip = "default: on\n\nTunneling allows you to block servers\nper-application instead of globally\n\nThis prevents servers in other games\nand apps from becoming unintentionally\nblocked.\n\non - block traffic for only Overwatch.exe\noff - (global) block traffic for this device",
			.action = [this]() { (*g_settings).toggleOptionTunneling(); },
			.state = [this]() { return (*g_settings).getAppSettings().options.tunneling; },
			.external = false,
		},
		{
			.title = "tunneling configuration",
			//.tooltip = "default: automatic\n\nThe location of Overwatch.exe must be known\nin order to enable tunneling. in most situations,\ntunneling should automatically locate your\nOverwatch.exe application. if automatically\nfinding it didn't work, a manual configuration popup will appear when you open the application. this can be ignored, but will show up again every launch unless you disable \"tunneling\" in options  • since tunneling only blocks traffic per - application, if you notice dropship is not working at all(you still connect to blocked servers), you may have selected an incorrect Overwatch.exe.you can always disable tunneling if you choose to. ",
			.action = [this]() {
				g_settings->setConfigTunnelingPath(std::nullopt);
				if (!g_settings->getAppSettings().options.tunneling)
				{
					(*g_settings).toggleOptionTunneling();
				}
			},
			.external = false,
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

