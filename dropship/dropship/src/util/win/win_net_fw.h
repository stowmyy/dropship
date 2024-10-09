#pragma once

//#include "util/win/win_firewall/windows_firewall.h";
//#include "util/win/win_network/windows_network.h";

#include <netfw.h> // INetFwPolicy2


namespace util::win_net_fw {

	namespace {
		CComPtr<INetFwPolicy2> getNetworkFirewallPolicy();
	}

	bool isFirewallEnabledForNetworkProfile(NET_FW_PROFILE_TYPE2 profile);

	void enableFirewallForNetworkProfile(NET_FW_PROFILE_TYPE2 profile);

}
