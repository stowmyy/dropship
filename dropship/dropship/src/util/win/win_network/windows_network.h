#pragma once

#include <netlistmgr.h> // INetworkListManager
//#include <icftypes.h> // NET_FW_PROFILE_TYPE2_

#include "util/win/win_net_fw.h"

namespace util::win_network {

	struct NetworkInformation
	{
		// number of networks connected to. 2 may indicate vpn.
		// networks may be disconnected from internet.
		int connected_networks_count;

		// (bitmask & NET_FW_PROFILE2_PRIVATE) if any connected networks are PRIVATE
		// (bitmask & NET_FW_PROFILE2_PUBLIC) if any connected networks are PUBLIC
		// (bitmask & NET_FW_PROFILE2_DOMAIN) if any connected networks are DOMAIN
		// (bitmask & NET_FW_PROFILE2_ALL) if any connected networks are DOMAIN, PUBLIC, or PRIVATE

		long network_profiles_connected;
		long firewall_profiles_enabled;

		// is there a mismatch? ex. are they connected to a private network, but the private firewall profile is disabled?
		bool connected_networks_are_enabled_in_firewall;

		// imgui timer;
		//double _updated_at;
	};

	NetworkInformation queryNetwork();

}


