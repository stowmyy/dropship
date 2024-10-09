#include "pch.h"

#include "win_net_fw.h"


namespace util::win_net_fw {

    namespace {

        /* get network firewall policy */
        CComPtr<INetFwPolicy2> getNetworkFirewallPolicy()
        {

            CComPtr<INetFwPolicy2> pNetFwPolicy2;
            if (FAILED(pNetFwPolicy2.CoCreateInstance(__uuidof(NetFwPolicy2)))) throw std::runtime_error("failed to get network firewall policy");

            return pNetFwPolicy2;
        }

    }

    /* gets firewall enabled for network type */
    bool isFirewallEnabledForNetworkProfile(NET_FW_PROFILE_TYPE2 profile)
    {

        auto pNetFwPolicy2 = getNetworkFirewallPolicy();

        VARIANT_BOOL fwEnabled;
        if (FAILED(pNetFwPolicy2->get_FirewallEnabled(profile, &fwEnabled))) throw std::runtime_error("failed to query firewall state for network profile");

        return (fwEnabled != VARIANT_FALSE);
    }

    void enableFirewallForNetworkProfile(NET_FW_PROFILE_TYPE2 profile)
    {

        auto pNetFwPolicy2 = getNetworkFirewallPolicy();
        if (FAILED(pNetFwPolicy2->put_FirewallEnabled(profile, VARIANT_TRUE))) throw std::runtime_error("failed to enable firewall for network profile");
    
    }

}
