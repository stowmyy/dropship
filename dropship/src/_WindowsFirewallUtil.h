#pragma once

#define NOMINMAX

/*
    !! QUARANTINE ZONE !!

    there is a lot of win32 stuff in this one file.
      • the official windows examples have massive memory leaks and are bad.
      • thus one uses ccomptr to avoid memory leaks: https://github.com/microsoft/Windows-classic-samples/blob/44d192fd7ec6f2422b7d023891c5f805ada2c811/Samples/Win7Samples/security/windowsfirewall/enumeratefirewallrules/EnumerateFirewallRules.cpp

*/

/*
    v2 notes
        - CComPtr is ~shared_ptr
*/


#include <string>
#include <format>
#include <iostream>

#include <vector>
#include <functional> // std::function

#include <comdef.h> // _com_error
#include <netlistmgr.h>

#include <windows.h>
#include <stdio.h>
#include <winsock.h> // for getprotocolbyname
#include <atlcomcli.h>
#include <netfw.h>
#include <shlwapi.h> // for SHLoadIndirectString

#include "failable.h"
#include "util.hpp"

#include "imgui.h"

#undef min
#undef max


struct NetworkInformation
{
    // number of networks connected to. 2 may indicate vpn.
    // networks may be disconnected from internet.
    int connected_networks;

    // (bitmask & NET_FW_PROFILE2_PRIVATE) if any connected networks are PRIVATE
    // (bitmask & NET_FW_PROFILE2_PUBLIC) if any connected networks are PUBLIC
    // (bitmask & NET_FW_PROFILE2_DOMAIN) if any connected networks are DOMAIN
    // (bitmask & NET_FW_PROFILE2_ALL) if any connected networks are DOMAIN, PUBLIC, or PRIVATE

    long network_profiles_connected;
    long firewall_profiles_enabled;

    // is there a mismatch? ex. are they connected to a private network, but the private firewall profile is disabled?
    bool valid;

    // imgui timer;
    double _updated_at;
};


struct _pFwPolicyRules
{
    CComPtr<INetFwPolicy2> pNetFwPolicy2;
    CComPtr<INetFwRules> pFwRules;
};

class _WindowsFirewallUtil : public failable
{
    private:

        NetworkInformation networkInfo;
        std::string _group_name = "stormy.gg/dropship";

        // ugh
        bool modal_simple = true;

        void failH(std::string why, HRESULT hr = NULL)
        {
            if (hr != NULL)
            {
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                printf("%s failed: \"%s\" 0x%08lx\n", why.c_str(), errMsg, hr);
            }
            else
            {
                printf("%s", why.c_str());
            }
            this->fail(why);
        }

        CComPtr<INetFwPolicy2> getNetPolicy()
        {
            HRESULT hr;

            // Retrieve INetFwPolicy2
            CComPtr<INetFwPolicy2> pNetFwPolicy2;
            hr = pNetFwPolicy2.CoCreateInstance(__uuidof(NetFwPolicy2));
            if (FAILED(hr))
            {
                this->failH("pNetFwPolicy2.CoCreateInstance failed", hr);
            }

            return pNetFwPolicy2;
        }

        _pFwPolicyRules getFwRules()
        {
            HRESULT hr;

            auto pNetFwPolicy2 = getNetPolicy();

            // Retrieve INetFwRules
            CComPtr<INetFwRules> pFwRules;
            hr = pNetFwPolicy2->get_Rules(&pFwRules);
            if (FAILED(hr))
            {
                this->failH("pNetFwPolicy2->get_Rules failed", hr);
            }

            return _pFwPolicyRules
            {
                .pNetFwPolicy2 = pNetFwPolicy2,
                .pFwRules = pFwRules,
            };
        }

        // returns true if succeeded
        // original function took (INetFwProfile* fwProfile)
        void WindowsFirewallIsOn(NET_FW_PROFILE_TYPE2 profile, bool* fwOn)
        {
            HRESULT hr;

            // Retrieve INetFwPolicy2
            auto pNetFwPolicy2 = getNetPolicy();

            // Get the current state of the firewall.
            VARIANT_BOOL fwEnabled;
            hr = pNetFwPolicy2->get_FirewallEnabled(profile, &fwEnabled);
            if (FAILED(hr))
            {
                this->failH("pNetFwPolicy2->get_FirewallEnabled failed", hr);
                return;
            }

            *fwOn = (fwEnabled != VARIANT_FALSE);
        }

        void WindowsFirewallToggle(NET_FW_PROFILE_TYPE2 profile, bool enable)
        {
            HRESULT hr;
            bool fwOn;
            WindowsFirewallIsOn(profile, &fwOn);

            // Retrieve INetFwPolicy2
            CComPtr<INetFwPolicy2> pNetFwPolicy2;
            hr = pNetFwPolicy2.CoCreateInstance(__uuidof(NetFwPolicy2));
            if (FAILED(hr))
            {
                this->failH("pNetFwPolicy2.CoCreateInstance failed", hr);
                return;
            }

            // if it's off and we want it on
            if (!fwOn && enable)
            {
                // Turn the firewall on.
                hr = pNetFwPolicy2->put_FirewallEnabled(profile, VARIANT_TRUE);
                if (FAILED(hr))
                {
                    this->failH("pNetFwPolicy2->put_FirewallEnabled failed", hr);
                    return;
                }

                printf("The firewall is now on.\n");
            }

            // if it's on and we want it off
            else if (fwOn && !enable)
            {
                // Turn the firewall off.
                hr = pNetFwPolicy2->put_FirewallEnabled(profile, VARIANT_FALSE);
                if (FAILED(hr))
                {
                    this->failH("pNetFwPolicy2->put_FirewallEnabled failed", hr);
                    return;
                }

                printf("The firewall is now off.\n");
            }
        }

        void toggleNeededFirewallProfiles()
        {
            // private
            if (
                this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_PRIVATE
                && !(this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_PRIVATE)
                )
            {
                this->WindowsFirewallToggle(NET_FW_PROFILE2_PRIVATE, true);
            }

            // public
            if (
                this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_PUBLIC
                && !(this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_PUBLIC)
                )
            {
                this->WindowsFirewallToggle(NET_FW_PROFILE2_PUBLIC, true);
            }

            this->refetchNetworkStatus();
        }

        // TODO v2 come  back here

        // returns true if succeeded
        void refetchNetworkStatus()
        {
            int netcount = 0;
            long connectedNetworkProfileBitmask = 0x0;

            // https://learn.microsoft.com/en-us/windows/win32/api/objbase/ne-objbase-coinit#remarks
            // CoInitializeEx(NULL, COINIT_MULTITHREADED)

            HRESULT hr;

            CComPtr<INetworkListManager> pNetworkListManager;

            if (SUCCEEDED(
                pNetworkListManager.CoCreateInstance(CLSID_NetworkListManager)
            ))
            {

                CComPtr<IEnumNetworks> pEnumerator;
                hr = pNetworkListManager->GetNetworks(NLM_ENUM_NETWORK_CONNECTED, &pEnumerator);
                if (FAILED(hr))
                {
                    this->failH("failed getting connected networks", hr);
                    return;
                }

                CComPtr<IEnumVARIANT> pVariant;
                hr = pEnumerator.QueryInterface(&pVariant);
                if (FAILED(hr))
                {
                    this->failH("pEnumerator.QueryInterface", hr);
                    return;
                }

                for (CComVariant var; pVariant->Next(1, &var, nullptr) == S_OK; var.Clear())
                {
                    CComPtr<INetwork> pINetwork;
                    if (SUCCEEDED(var.ChangeType(VT_DISPATCH)) &&
                        SUCCEEDED(V_DISPATCH(&var)->QueryInterface(IID_PPV_ARGS(&pINetwork))))
                    {
                        netcount += 1;

                        NLM_NETWORK_CATEGORY network_cat;

                        /*BSTR bstrVal;
                        if (SUCCEEDED(pINetwork->GetName(&bstrVal)))
                        {
                            wprintf(L" \"%s\"\n", bstrVal);
                        }*/

                        /*VARIANT_BOOL booVal;
                        if (SUCCEEDED(pINetwork->get_IsConnectedToInternet(&booVal)))
                        {
                            printf(booVal ? "connected\n" : "disconnected\n");
                        }*/

                        if (SUCCEEDED(pINetwork->GetCategory(&network_cat)))
                        {
                            switch (network_cat)
                            {
                            case NLM_NETWORK_CATEGORY_PRIVATE:

                                connectedNetworkProfileBitmask |= NET_FW_PROFILE2_PRIVATE;
                                // printf(" private", NLM_NETWORK_CATEGORY_PRIVATE);
                                break;

                            case NLM_NETWORK_CATEGORY_PUBLIC:

                                connectedNetworkProfileBitmask |= NET_FW_PROFILE2_PUBLIC;
                                // printf(" public", NLM_NETWORK_CATEGORY_PUBLIC);
                                break;

                            case NLM_NETWORK_CATEGORY_DOMAIN_AUTHENTICATED:

                                connectedNetworkProfileBitmask |= NET_FW_PROFILE2_DOMAIN;
                                // printf(" domain", NLM_NETWORK_CATEGORY_DOMAIN_AUTHENTICATED);
                                break;

                            default:

                                break;
                            }
                        }

                    }
                }

                long fw_profiles = 0x0;

                bool private_on = false;
                bool public_on = false;
                bool domain_on = false;

                this->WindowsFirewallIsOn(NET_FW_PROFILE2_PRIVATE, &private_on);
                this->WindowsFirewallIsOn(NET_FW_PROFILE2_PUBLIC, &public_on);
                this->WindowsFirewallIsOn(NET_FW_PROFILE2_DOMAIN, &domain_on);

                if (private_on) fw_profiles |= NET_FW_PROFILE2_PRIVATE;
                if (public_on) fw_profiles |= NET_FW_PROFILE2_PUBLIC;
                if (domain_on) fw_profiles |= NET_FW_PROFILE2_DOMAIN;

                // 
                // 
                // TODO replace with                 //hr = this->pNetFwPolicy2->get_CurrentProfileTypes(&(this->current_network_profile_type_bitmask));
                // 
                // 

                bool verifiedProfiles =
                    fw_profiles != 0x0
                    && ((fw_profiles & connectedNetworkProfileBitmask) == connectedNetworkProfileBitmask)
                ;

                double _updated_at = 0.0;

                if (ImGui::GetCurrentContext() != nullptr && ImGui::GetTime())
                {
                    _updated_at = ImGui::GetTime();
                }

                this->networkInfo = { netcount, connectedNetworkProfileBitmask, fw_profiles, verifiedProfiles, _updated_at };
            }

            /*if (this->networkInfo.connected_networks == 0 && ImGui::GetCurrentContext() != nullptr)
            {
                ImGui::OpenPopup("offline");
            }*/
        }

        double modal_warnings_fixed_time = 0;


        // predicate returns void and takes arguments by reference. second argument for removing rules.
        void forFirewallRulesInGroup(std::function<void(const CComPtr<INetFwRule>&, const CComPtr<INetFwRules>&)> predicate)
        {
            HRESULT hr;

            auto _pFwPolicyRules = getFwRules();

            // Iterate through all of the rules in pFwRules
            CComPtr<IUnknown> pEnumerator;
            hr = _pFwPolicyRules.pFwRules->get__NewEnum(&pEnumerator);
            if (FAILED(hr))
            {
                wprintf(L"get__NewEnum failed: 0x%08lx\n", hr);
                return;
            }

            CComPtr<IEnumVARIANT> pVariant;
            hr = pEnumerator.QueryInterface(&pVariant);
            if (FAILED(hr))
            {
                wprintf(L"get__NewEnum failed to produce IEnumVariant: 0x%08lx\n", hr);
                return;
            }

            ULONG cFetched = 0;
            for (CComVariant var; pVariant->Next(1, &var, &cFetched) == S_OK; var.Clear())
            {
                CComPtr<INetFwRule> pFwRule;
                if (SUCCEEDED(var.ChangeType(VT_DISPATCH)) &&
                    SUCCEEDED(V_DISPATCH(&var)->QueryInterface(IID_PPV_ARGS(&pFwRule))))
                {
                    CComBSTR groupName;
                    if (SUCCEEDED(pFwRule->get_Grouping(&groupName)) && groupName)
                    {
                        const std::string s_groupName(_bstr_t(groupName, true));

                        if (this->_group_name == s_groupName)
                        {
                            predicate(pFwRule, _pFwPolicyRules.pFwRules);
                        }
                    }
                }
            }
        }

    public:

        ~_WindowsFirewallUtil()
        {
            CoUninitialize();
            WSACleanup();
        }

        _WindowsFirewallUtil() :
             networkInfo({ 0, 0x0, 0x0, false, 0.0 })
        {
            HRESULT hr;

            // Initialize COM.
            hr = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (FAILED(hr))
            {
                this->failH("CoInitializeEx failed", hr);
                return;
            }
            else
            {
                // We use WinSock only to convert protocol numbers to names.
                // If we cannot initialize WinSock, then just proceed without it.
                WSADATA wsaData;
                int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

            }

            // verify connected networks and firewall profiles
            this->refetchNetworkStatus();

            // print exisitng rules
            /*printf("existing rules: \n");
            forFirewallRulesInGroup([](const CComPtr<INetFwRule>& pFwRule) {
                CComBSTR ruleName;

                if (SUCCEEDED(pFwRule->get_Name(&ruleName)) && ruleName)
                {
                    printf("  .. %s\n", (char *) _bstr_t(ruleName, true));
                }
            });*/
        }

        // todo v2
        void _RenderUI()
        {
            ImGui::SetNextWindowSize({ 400, 0 });
            if (ImGui::BeginPopupModal("warnings_simple", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
            {
                std::string network_type = "unknown";

                if (this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_PRIVATE && !(this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_PRIVATE))
                {
                    network_type = "private";
                }
                else if (this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_PUBLIC && !(this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_PUBLIC))
                {
                    network_type = "public";
                }


                ImGui::Text("hey! you're on a");
                ImGui::SameLine();
                ImGui::TextColored({ 1, 0, 1, 1 }, network_type.c_str());
                ImGui::SameLine();
                ImGui::Text("network, but that network");
                ImGui::SameLine();
                ImGui::SetCursorPosX(16);
                ImGui::TextWrapped("\ntype isn't set up to block anything in windows firewall.");

                if (this->networkInfo.connected_networks > 1)
                {
                    //ImGui::TextColored(ImVec4{ 1, .5, 0, 1 }, "warn:");
                    ImGui::TextColored(ImVec4{ 1, 0, 1, 1 }, "note:");
                    ImGui::SameLine();
                    ImGui::TextWrapped(std::format("this computer is connected to {0} different networks. are you on a VPN?", this->networkInfo.connected_networks).c_str());
                }

                ImGui::TextWrapped("do you want to enable that network profile in windows firewall?");

                ImGui::Spacing();

                ImGui::SeparatorText("decide");

                {
                    if (ImGui::Button("fix it for me", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
                    {
                        // ..
                        this->toggleNeededFirewallProfiles();

                    }
                    ImGui::SetItemTooltip("this will turn on windows firewall");
                }

                {
                    //ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(6 / 7.0f, 0.6f, 0.6f));
                    //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(6 / 7.0f, 0.7f, 0.7f));
                    //ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(6 / 7.0f, 0.8f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_Button, { 1, 0, 1, 0 });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1, 0, 1, .4 });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 1, 0, 1, .6 });


                    //if (ImGui::Button("i will do it myself..", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    if (ImGui::Button(".. network settings ..", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    {
                        ImGui::OpenPopup("warnings_control_panel");

                        // option 01
                        //system("%systemroot%\\system32\\control.exe /name Microsoft.WindowsFirewall");

                        // option 02
                        system("start windowsdefender://network");

                    }
                    ImGui::PopStyleColor(3);

                    // TODO this was duplicated. get rid of complicated screen, it's not helpful :c
                    {
                        if (ImGui::BeginPopupModal("warnings_control_panel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
                        {
                            // option 01
                            //ImGui::Text("control panel should have opened. from here,\ntap \"Turn Windows Defender Firewall on or off\" \n(on the left side)\n\nturn it on for the missing (red) profile(s)\nin the table above. (recommended: both)");

                            // option 02
                            ImGui::Text("defender firewall should have popped up. from\nhere, look for which network says (active) next\nto it and tap the button that says\n\n");

                            std::string text = "\"Turn on\"";
                            {
                                float font_size = ImGui::GetFontSize() * text.size() / 2;
                                ImGui::SetCursorPosX(
                                    ImGui::GetWindowSize().x / 2 -
                                    font_size + (font_size / 2)
                                );
                            }
                            ImGui::TextColored({ 1, 0, 1, 1 }, text.c_str());

                            ImGui::Text("\nthat's all you need to do! :]");

                            ImGui::Separator();

                            if (ImGui::Button("done", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                            {
                                ImGui::CloseCurrentPopup();
                                this->refetchNetworkStatus();
                            }

                            ImGui::EndPopup();
                        }
                    }
                }

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("warnings_fixed", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
            {
                //ImGui::Text("fixed :D");

                ImGui::Text("windows firewall has been enabled as needed. \nyou can disable it any time in settings.");

                //ImGui::Separator();

                if ((this->modal_warnings_fixed_time > 0) && (this->modal_warnings_fixed_time + 4.0 < ImGui::GetTime()))
                {
                    this->modal_warnings_fixed_time = 0;
                    ImGui::CloseCurrentPopup();
                }
                /*if (ImGui::Button("done", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }*/

                ImGui::EndPopup();

            }

            // below for next frame
            if (!this->networkInfo.valid && !(ImGui::IsPopupOpen("warnings_simple") || ImGui::IsPopupOpen("warnings")))
            {
                if (this->modal_simple)
                {
                    ImGui::OpenPopup("warnings_simple");
                }
                else
                {
                    ImGui::OpenPopup("warnings");
                }
                printf("\nhi");
            }

            if (this->networkInfo.valid && (ImGui::IsPopupOpen("warnings_simple") || ImGui::IsPopupOpen("warnings")) && !ImGui::IsPopupOpen("warnings_fixed"))
            {
                ImGui::OpenPopup("warnings_fixed");
                this->modal_warnings_fixed_time = ImGui::GetTime();
            }
      

            //ImGui::Text("debugging %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
            #ifdef _DEBUG
            {

                ImGui::Begin("debug");

                if (ImGui::CollapsingHeader("_WindowsFirewallUtil.h", ImGuiTreeNodeFlags_None))
                {

                    if (ImGui::Button("disable private", { 0, 0 }))
                    {
                        this->WindowsFirewallToggle(NET_FW_PROFILE2_PRIVATE, false);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("disable public", { 0, 0 }))
                    {
                        this->WindowsFirewallToggle(NET_FW_PROFILE2_PUBLIC, false);
                    }

                    //ImGui::BeginChild("_WindowsFirewallUtil.h", { 0, 200 }, true);
                    //ImGui::SeparatorText("_WindowsFirewallUtil.h");
                    ImGui::Indent();
                    ImGui::Text(std::format("connected networks: {0}", this->networkInfo.connected_networks).c_str());

                    ImGui::Text(std::format("network profiles: {0}", this->networkInfo.network_profiles_connected).c_str());
                    ImGui::Indent();
                    ImGui::Text(std::format("private: {0}", (this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_PRIVATE) ? "true" : "false").c_str());
                    ImGui::Text(std::format("public: {0}", (this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_PUBLIC) ? "true" : "false").c_str());
                    ImGui::Text(std::format("domain: {0}", (this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_DOMAIN) ? "true" : "false").c_str());
                    ImGui::Text(std::format("all: {0}", (this->networkInfo.network_profiles_connected & NET_FW_PROFILE2_ALL) ? "true" : "false").c_str());
                    ImGui::Unindent();

                    ImGui::Text(std::format("firewall profiles: {0}", this->networkInfo.firewall_profiles_enabled).c_str());
                    ImGui::Indent();
                    ImGui::Text(std::format("private: {0}", (this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_PRIVATE) ? "true" : "false").c_str());
                    ImGui::Text(std::format("public: {0}", (this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_PUBLIC) ? "true" : "false").c_str());
                    ImGui::Text(std::format("domain: {0}", (this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_DOMAIN) ? "true" : "false").c_str());
                    ImGui::Text(std::format("all: {0}", (this->networkInfo.firewall_profiles_enabled & NET_FW_PROFILE2_ALL) ? "true" : "false").c_str());
                    ImGui::Unindent();
                    ImGui::Unindent();
                    //ImGui::EndChild();
                }

                ImGui::End();
            }
            #endif
            
        }

        // returns true if succeeded
        bool add_rule(Endpoint* e, NET_FW_RULE_DIRECTION_ dir, bool enabled, NET_FW_PROFILE_TYPE2_ profile = NET_FW_PROFILE2_ALL)
        {
            //BSTR bstrRuleName = SysAllocString(std::wstring(e.title.begin(), e.title.end()).c_str());
            BSTR bstrRuleName = _com_util::ConvertStringToBSTR(e->title.c_str());
            BSTR bstrRuleDescription = _com_util::ConvertStringToBSTR(e->_firewall_rule_description.c_str());
            BSTR bstrRuleRAddresses = _com_util::ConvertStringToBSTR(e->_firewall_rule_address.c_str());
            BSTR bstrRuleGroup = _com_util::ConvertStringToBSTR(this->_group_name.c_str());

            HRESULT hr;

            auto _pFwPolicyRules = getFwRules();

            // todo v2
            INetFwRule* pFwRule = nullptr;
            hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule);
            if (FAILED(hr))
            {
                this->failH("CoCreateInstance for Firewall Rule failed \n", hr);
                goto Cleanup;
            }

            // Populate the Firewall Rule object
            pFwRule->put_Name(bstrRuleName);
            pFwRule->put_Description(bstrRuleDescription);
            //pFwRule->put_ApplicationName(bstrRuleApplication);
            pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
            pFwRule->put_RemoteAddresses(bstrRuleRAddresses);
            pFwRule->put_Direction(dir);
            pFwRule->put_Grouping(bstrRuleGroup);
            pFwRule->put_Profiles(profile);
            pFwRule->put_Action(NET_FW_ACTION_BLOCK);
            pFwRule->put_Enabled(enabled ? VARIANT_TRUE : VARIANT_FALSE);

            // Add the Firewall Rule
            hr = _pFwPolicyRules.pFwRules->Add(pFwRule);
            if (FAILED(hr))
            {
                this->failH("Firewall Rule Add failed: \n", hr);

                // WILL FAIL IF NOT RUNNING AS ADMINISTRATOR
                goto Cleanup;
            }

            printf("Firewall Rule Added\n");

        Cleanup:

            // Free BSTR's
            SysFreeString(bstrRuleName);
            SysFreeString(bstrRuleDescription);
            SysFreeString(bstrRuleGroup);
            //SysFreeString(bstrRuleApplication);
            //SysFreeString(bstrRuleLPorts);
            SysFreeString(bstrRuleRAddresses);

            // Release the INetFwRule object
            if (pFwRule != nullptr)
            {
                pFwRule->Release();
            }

            return SUCCEEDED(hr);
        }

        // returns true if succeeded
        bool toggle_group(std::string group, bool enable)
        {
            // BSTR to match group to - "stormy.gg"
            BSTR groupMatch = _com_util::ConvertStringToBSTR(group.c_str());

            HRESULT hr;

            auto pNetFwPolicy2 = getNetPolicy();


            hr = pNetFwPolicy2->EnableRuleGroup(NET_FW_PROFILE2_ALL, groupMatch, enable);
            if (FAILED(hr))
            {
                this->failH("failed to enable group", hr);
            }

            SysFreeString(groupMatch);

            return SUCCEEDED(hr);
        }


        // todo foreach with lambda
        void removeAllRulesForGroup()
        {
            forFirewallRulesInGroup([this](const CComPtr<INetFwRule>& pFwRule, const CComPtr<INetFwRules>& pFwRules) {

                CComBSTR ruleName;
                if (FAILED(pFwRule->get_Name(&ruleName)) && ruleName)
                {
                    this->failH("failed to get rule name");
                }

                pFwRules->Remove(ruleName);
            });

            printf("Removed all firewall rules for group: \"%s\"\n", this->_group_name.c_str());
        }


        void refetchStatus()
        {
            this->refetchNetworkStatus();
        }

        void syncFirewallEndpointState(std::vector<Endpoint>* endpoints, bool endpointDominant, bool only_unblocks = false)
        {
            forFirewallRulesInGroup([&](const CComPtr<INetFwRule>& pFwRule, const CComPtr<INetFwRules>& pFwRules) {
                CComBSTR ruleName;

                if (SUCCEEDED(pFwRule->get_Name(&ruleName)) && ruleName)
                {
                    VARIANT_BOOL __enabled;
                    if (FAILED(pFwRule->get_Enabled(&__enabled)))
                        return;

                    bool ruleEnabled;
                    ruleEnabled = (__enabled != VARIANT_FALSE);

                    NET_FW_RULE_DIRECTION rule_direction;
                    pFwRule->get_Direction(&rule_direction);

                    const std::string s_ruleName (_bstr_t(ruleName, true));

                    for (auto& e : *endpoints)
                    {

                        if (e.title == s_ruleName)
                        {
                            if (!e.active_desired_state != ruleEnabled && (!only_unblocks || (only_unblocks && (!e.active && e.active_desired_state))))
                            {

                                // if endpointDominant, set firewall to mirror endpoint state
                                if (endpointDominant)
                                {

                                    printf(std::format("{3} ({0}) firewall: {1}, ui: {2} . Setting firewall rule to match UI state\n", s_ruleName, ruleEnabled ? "block" : "allow", e.active ? "selected" : "not selected", rule_direction == NET_FW_RULE_DIR_IN ? "IN" : "OUT").c_str());
                                    if (FAILED(pFwRule->put_Enabled(e.active_desired_state ? VARIANT_FALSE : VARIANT_TRUE)))
                                    {
                                        printf("failed to write rule state\n");
                                        continue;
                                    }

                                    e.active = e.active_desired_state;

                                }
                                // if not, set endpoint state to mirror firewall state
                                else

                                {
                                    printf(std::format("({0}) firewall: {1}, ui: {2}. Setting UI state to match firewall state\n", s_ruleName, ruleEnabled ? "block" : "allow", e.active ? "selected" : "not selected").c_str());
                                    e.active = !ruleEnabled;
                                    e.active_desired_state = e.active;
                                }
                            }
                        }
                    }
                }

            });
        }
};
