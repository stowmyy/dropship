#pragma once

#include "util/win/win_firewall/windows_firewall.h"
#include "util/win/win_network/windows_network.h"

using json = nlohmann::json;

class Firewall
{

	/* consts */
	private:
		static inline const std::string __group_name { "stormy/dropship" };
		static inline const std::string __group_name_legacy { "stormy.gg/dropship" };

		static inline const std::string __win_net_fw_popup_name { "firewall_network_profile_wizard" };
		static inline const std::string __win_net_fw_manual_popup_name { "how_to_fix" };

		//static constexpr auto __network_query_timeout { 2s };
		static const auto __network_query_delay_s { 2 };
		//static const auto __win_net_fw_popup_close_delay { 2 };


	public:

		Firewall();
		~Firewall();

		void render();

		void tryWriteSettingsToFirewall(std::string data, std::string block);
		std::optional<std::string> tryFetchSettingsFromFirewall();

	private:

		bool __win_net_fw_popup_ignored { false };

		bool _coInitilizeSuccess;
		//std::optional<util::win_network::NetworkInformation> _network_information; // note; optional not needed, takes <10ms
		util::win_network::NetworkInformation _network_information;
		double _network_information_updated_frametime { 0. };

	private:

		/* fetch network status */
		/* timeout:2s*/
		void _queryNetworkStatus(); // auto timeout = __network_query_timeout

		void _validateRules();
};

