#include "pch.h"

#include "windows_network.h"

namespace util::win_network {


	NetworkInformation queryNetwork() {

        NetworkInformation output;

        {
            int total_netcount = 0;

            long total_connectedNetworkProfileBitmask = 0x0;
            long total_networkProfilesEnabledInFirewallBitmask = 0x0;

            // https://learn.microsoft.com/en-us/windows/win32/api/objbase/ne-objbase-coinit#remarks
            // CoInitializeEx(NULL, COINIT_MULTITHREADED)

            /* pNetworkListManager */
            CComPtr<INetworkListManager> pNetworkListManager;
            if (FAILED(pNetworkListManager.CoCreateInstance(CLSID_NetworkListManager))) throw std::runtime_error("failed creating windows network instance");
            
            /* network iterator */
            CComPtr<IEnumNetworks> pEnumerator;
            if (FAILED(pNetworkListManager->GetNetworks(NLM_ENUM_NETWORK_CONNECTED, &pEnumerator))) throw std::runtime_error("failed querying connected networks");

            /* network item */
            CComPtr<IEnumVARIANT> pVariant;
            if (FAILED(pEnumerator.QueryInterface(&pVariant))) throw std::runtime_error("failed querying network enumerator");

            /* iterate networks */
            for (CComVariant var; pVariant->Next(1, &var, nullptr) == S_OK; var.Clear())
            {

                /* query network*/
                CComPtr<INetwork> pINetwork;
                if (FAILED(var.ChangeType(VT_DISPATCH))) throw std::runtime_error("failed to change dispatch type");
                if (FAILED(V_DISPATCH(&var)->QueryInterface(IID_PPV_ARGS(&pINetwork)))) throw std::runtime_error("failed to query network");

                total_netcount ++;

                /*
                    CComBSTR
                    CComBSTR
                
                BSTR bstrVal;
                if (SUCCEEDED(pINetwork->GetName(&bstrVal)))
                {
                    wprintf(L" \"%s\"\n", bstrVal);
                }*/

                /*VARIANT_BOOL booVal;
                if (SUCCEEDED(pINetwork->get_IsConnectedToInternet(&booVal)))
                {
                    printf(booVal ? "connected\n" : "disconnected\n");
                }*/

                /* network profile */
                NLM_NETWORK_CATEGORY network_cat;
                if (FAILED(pINetwork->GetCategory(&network_cat))) throw std::runtime_error("failed to query network category");
                
                if (network_cat == NLM_NETWORK_CATEGORY_PRIVATE) total_connectedNetworkProfileBitmask |= NET_FW_PROFILE2_PRIVATE;
                else if (network_cat == NLM_NETWORK_CATEGORY_PUBLIC) total_connectedNetworkProfileBitmask |= NET_FW_PROFILE2_PUBLIC;
                else if (network_cat == NLM_NETWORK_CATEGORY_DOMAIN_AUTHENTICATED) total_connectedNetworkProfileBitmask |= NET_FW_PROFILE2_DOMAIN;
                else throw std::runtime_error("unknown network category detected");
            }

            if (util::win_net_fw::isFirewallEnabledForNetworkProfile(NET_FW_PROFILE2_PRIVATE)) total_networkProfilesEnabledInFirewallBitmask |= NET_FW_PROFILE2_PRIVATE;
            if (util::win_net_fw::isFirewallEnabledForNetworkProfile(NET_FW_PROFILE2_PUBLIC)) total_networkProfilesEnabledInFirewallBitmask |= NET_FW_PROFILE2_PUBLIC;
            if (util::win_net_fw::isFirewallEnabledForNetworkProfile(NET_FW_PROFILE2_DOMAIN)) total_networkProfilesEnabledInFirewallBitmask |= NET_FW_PROFILE2_DOMAIN;

            // 
            // 
            // TODO replace with                 //hr = this->pNetFwPolicy2->get_CurrentProfileTypes(&(this->current_network_profile_type_bitmask));
            // 
            // 

            /*
                • at least one network profile is enabled in firewall
                • all networks connected to are enabled in firewall
            */
            bool connected_networks_are_enabled_in_firewall =
                total_networkProfilesEnabledInFirewallBitmask != 0x0
                && ((total_networkProfilesEnabledInFirewallBitmask & total_connectedNetworkProfileBitmask) == total_connectedNetworkProfileBitmask)
                ;

            double _updated_at = 0.0;

            if (ImGui::GetCurrentContext() != nullptr && ImGui::GetTime())
            {
                _updated_at = ImGui::GetTime();
            }

            //this->networkInfo = { netcount, connectedNetworkProfileBitmask, total_networkProfilesEnabledInFirewallBitmask, connected_networks_are_enabled_in_firewall, _updated_at };
            
            output = {
                .connected_networks_count = total_netcount,
                .network_profiles_connected = total_connectedNetworkProfileBitmask,
                .firewall_profiles_enabled = total_networkProfilesEnabledInFirewallBitmask,
                .connected_networks_are_enabled_in_firewall = connected_networks_are_enabled_in_firewall,
            };

            /*if (this->networkInfo.connected_networks == 0 && ImGui::GetCurrentContext() != nullptr)
            {
                ImGui::OpenPopup("offline");
            }*/
        }

        return output;
	}

}
