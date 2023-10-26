#pragma once

// Windows.h defines min() and max() as macros
#define NOMINMAX

// i decided to cram all the windows TRASH into one file to reduce risk of spreading this plague

// >> warning: quarintine zone below <<


// this all could have been done through powershell maybe? that would be waaaay easier T.T;
// maybe do that if this sucks. powershell is probably slower.


//#include<comutil.h>
#include <comdef.h>
#include <string>

#include <iostream>
#include <stdlib.h>

#include "atlbase.h"
#include "atlstr.h"

//using namespace std;
#include <windows.h>
#include <stdio.h>
#include <atlcomcli.h>
#include <netfw.h>

#include <format>

#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "oleaut32.lib" )

#pragma comment(lib,"comsuppw.lib") // _bstr_t

#define NET_FW_IP_PROTOCOL_TCP_NAME L"TCP"
#define NET_FW_IP_PROTOCOL_UDP_NAME L"UDP"

#define NET_FW_RULE_DIR_IN_NAME L"In"
#define NET_FW_RULE_DIR_OUT_NAME L"Out"

#define NET_FW_RULE_ACTION_BLOCK_NAME L"Block"
#define NET_FW_RULE_ACTION_ALLOW_NAME L"Allow"

#define NET_FW_RULE_ENABLE_IN_NAME L"TRUE"
#define NET_FW_RULE_DISABLE_IN_NAME L"FALSE"

// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ics/c-enumerating-firewall-rules

#include <codecvt>
//#include <locale>


#include <functional>
using namespace std::placeholders;

#include "failable.h"

#include <vector>

#include "util.hpp" // endpoint


// primary

// for rendering warnings etc.
#include "imgui.h"

// ONLY used for checking network. delete if gone.
#include <netlistmgr.h>


#undef min
#undef max


// TODO dedupe
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static bool EqualBSTR(const BSTR String1, const BSTR String2, bool IgnoreCase = false)
{
    if (String1 == nullptr || String2 == nullptr) {
        return false;
    }

    const size_t MaxCount = std::min(static_cast<size_t>(SysStringLen(String1)), static_cast<size_t>(SysStringLen(String2)));

    if (IgnoreCase) {
        return _wcsnicmp(String1, String2, MaxCount) == 0;
    }
    else {
        return wcsncmp(String1, String2, MaxCount) == 0;
    }
}

//void TextCenter(std::string text) {
//    float font_size = ImGui::GetFontSize() * text.size() / 2;
//    ImGui::SameLine(
//        ImGui::GetWindowSize().x / 2 -
//        font_size + (font_size / 2)
//    );
//
//    ImGui::Text(text.c_str());
//}

static HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2)
{
    HRESULT hr = S_OK;

    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)ppNetFwPolicy2);

    if (FAILED(hr))
    {
        wprintf(L"CoCreateInstance for INetFwPolicy2 failed: 0x%08lx\n", hr);
        goto Cleanup;
    }

Cleanup:
    return hr;
}


// TODO
// IMPORTANT ConvertBSTRToString just like convertstringtobstr, same class


struct RuleData
{
    const std::string title;
    const std::string _firewall_rule_description;
    const std::string group;
    // idea: ping ip is first ip in rule?
};

//struct ProfilesMask
//{
//    bool privateOn;
//    bool publicOn;
//    bool domainOn;
//};

struct NetworkInformation
{
    // number of networks connected to. 2 may indicate vpn.
    // networks may be disconnected from internet.
    int connected_networks;

    // (bitmask & NET_FW_PROFILE2_PRIVATE) if any connected networks are PRIVATE
    // (bitmask & NET_FW_PROFILE2_PUBLIC) if any connected networks are PUBLIC
    // (bitmask & NET_FW_PROFILE2_DOMAIN) if any connected networks are DOMAIN
    //
    // (bitmask & NET_FW_PROFILE2_ALL) if any connected networks are DOMAIN, PUBLIC, or PRIVATE
    // long network_category_bitmask;
    // bool firewall_profile_bitmask;

    long network_profiles_connected;
    long firewall_profiles_enabled;

    /*ProfilesMask connected_network_profiles;
    ProfilesMask enabled_firewall_profiles;*/

    // is there a mismatch? ex. are they connected to a private network, but the private firewall profile is disabled?
    bool valid;

    // imgui timer;
    double _updated_at;
};




class _WindowsFirewallUtil : public failable
{
    private:

        // if network is swapped, this will not update
        // TODO test swapping network
        //NET_FW_PROFILE_TYPE2 current_network_profile_type_bitmask;
        // bitmask can have more than 1 bit set if multiple profiles are active at the same time
        // compare: (CurrentProfilesBitMask & NET_FW_PROFILE2_ALL)
        // INetworkEvents

        //long current_network_profile_type_bitmask;

        // TODO do these need to be pointers?
        INetFwRules* pFwRules;
        INetFwPolicy2* pNetFwPolicy2;
        HRESULT hrComInit;

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
                wprintf(L"failed: 0x%08lx\n", hr);
            }
            else
            {
                printf("%s", why.c_str());
            }
            this->fail(why);
        }

        // returns true if succeeded
        // original function took (INetFwProfile* fwProfile)
        bool WindowsFirewallIsOn(IN NET_FW_PROFILE_TYPE2 profile, OUT bool* fwOn)
        {
            HRESULT hr = S_OK;
            VARIANT_BOOL fwEnabled;

            _ASSERT(profile != NULL);
            _ASSERT(fwOn != NULL);

            *fwOn = false;

            // Get the current state of the firewall.
            //hr = fwProfile->get_FirewallEnabled(&fwEnabled);
            this->pNetFwPolicy2->get_FirewallEnabled(profile, &fwEnabled);
            if (FAILED(hr))
            {
                this->failH("get_FirewallEnabled failed", hr);
                goto error;
            }

            // Check to see if the firewall is on.
            if (fwEnabled != VARIANT_FALSE)
            {
                *fwOn = true;
                //printf("The firewall is on.\n");
            }
            else
            {
                //printf("The firewall is off.\n");
            }

        error:

            return SUCCEEDED(hr);
        }

        // returns true if succeeded
        bool WindowsFirewallToggle(IN NET_FW_PROFILE_TYPE2 profile, IN bool enable)
        {
            HRESULT hr = S_OK;
            bool fwOn;

            _ASSERT(profile != NULL);

            // Check to see if the firewall is off.
            if (!WindowsFirewallIsOn(profile, &fwOn))
            {
                this->failH("WindowsFirewallIsOn failed", hr);
                goto error;
            }

            // if it's off and we want it on
            if (!fwOn && enable)
            {
                // Turn the firewall on.
                hr = this->pNetFwPolicy2->put_FirewallEnabled(profile, VARIANT_TRUE);
                if (FAILED(hr))
                {
                    this->failH("put_FirewallEnabled failed", hr);
                    goto error;
                }

                printf("The firewall is now on.\n");
            }

            // if it's on and we want it off
            else if (fwOn && !enable)
            {
                // Turn the firewall off.
                hr = this->pNetFwPolicy2->put_FirewallEnabled(profile, VARIANT_FALSE);
                if (FAILED(hr))
                {
                    this->failH("put_FirewallEnabled failed", hr);
                    goto error;
                }

                printf("The firewall is now off.\n");
            }

        error:

            return SUCCEEDED(hr);
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
                printf("\n\n\npublic\n\n\n");
                this->WindowsFirewallToggle(NET_FW_PROFILE2_PUBLIC, true);
            }

            this->refetchNetworkStatus();
        }

        // returns true if succeeded
        bool refetchNetworkStatus()
        {
            int netcount = 0;
            long connectedNetworkProfileBitmask = 0x0;

            // https://learn.microsoft.com/en-us/windows/win32/api/objbase/ne-objbase-coinit#remarks
            // CoInitializeEx(NULL, COINIT_MULTITHREADED)
            INetworkListManager* pNetworkListManager = nullptr;
            IEnumNetworks* pEnum = nullptr;
            INetwork* pINetwork = nullptr;

            HRESULT hr = S_OK;


            if (SUCCEEDED(
                CoCreateInstance(CLSID_NetworkListManager, NULL,
                    CLSCTX_ALL, IID_INetworkListManager,
                    (LPVOID*)&pNetworkListManager)
            ))
            {

                hr = pNetworkListManager->GetNetworks(NLM_ENUM_NETWORK_CONNECTED, &pEnum);
                if (FAILED(hr))
                {
                    //this->failH("failed getting connected networks", hr);
                    goto Cleanup;
                }

                //networks->get__NewEnum(&pEnumerator);}

            //ULONG cFetched = 0;

                NLM_NETWORK_CATEGORY network_cat;
                BSTR bstrVal;
                VARIANT_BOOL booVal;

                //hr = pEnum->Next(1, &pINetwork, &cFetched);
                hr = pEnum->Next(1, &pINetwork, nullptr);

                while (SUCCEEDED(hr) && hr != S_FALSE)
                {

                    netcount += 1;
                    printf(std::to_string(netcount).c_str());


                    //hr = pINetwork->GetCategory(&network_cat);

                    /*if (SUCCEEDED(pINetwork->GetName(&bstrVal)))
                    {
                        wprintf(L" \"%s\"", bstrVal);
                    }*/

                    /*if (SUCCEEDED(pINetwork->get_IsConnectedToInternet(&booVal)))
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

                    // printf("\n\n");

                    hr = pEnum->Next(1, &pINetwork, nullptr);

                }

                //long fw_profiles = 0x0;
                // TODO hr = this->pNetFwPolicy2->get_CurrentProfileTypes(&fw_profiles);

                // figure out if firewall is enabled for each active profile


                long fw_profiles = 0x0;

                bool private_on = false;
                bool public_on = false;
                bool domain_on = false;
                //bool all_on = false;

                this->WindowsFirewallIsOn(NET_FW_PROFILE2_PRIVATE, &private_on);
                this->WindowsFirewallIsOn(NET_FW_PROFILE2_PUBLIC, &public_on);
                this->WindowsFirewallIsOn(NET_FW_PROFILE2_DOMAIN, &domain_on);
                //this->WindowsFirewallIsOn(NET_FW_PROFILE2_ALL, &all_on);

                if (private_on) fw_profiles |= NET_FW_PROFILE2_PRIVATE;
                if (public_on) fw_profiles |= NET_FW_PROFILE2_PUBLIC;
                if (domain_on) fw_profiles |= NET_FW_PROFILE2_DOMAIN;

                // 
                // 
                // TODO replace with                 //hr = this->pNetFwPolicy2->get_CurrentProfileTypes(&(this->current_network_profile_type_bitmask));
                // 
                // 



                //if (connectedNetworkProfileBitmask & NET_FW_PROFILE2_PRIVATE)

                /*bool verifiedProfiles =
                    this->networkInfo.firewall_profiles_enabled != 0x0
                    && ((this->networkInfo.firewall_profiles_enabled & this->networkInfo.network_profiles_connected) == this->networkInfo.network_profiles_connected)
                ;*/

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



                //if (!verifiedProfiles)
                //{
                //    //ImGui::OpenPopup("warnings");
                //    // imgui::ispopupopen
                //}


                {

                    // https://stackoverflow.com/questions/38927391/

                    /*
                        WITH long mask = firewall_profiles_enabled & network_categories_connected;

                        (firewall profiles enabled) & (network categories connected)
                        x not gonna happen

                        public, public | 7 true
                        public, private | 0 false
                        private, public | 0 false
                        domain, private | 0 false
                        x domain, all | 1 true
                        all, private | 2 true                             # notice this one. if all FW profiles, we good.
                                                                          # 0xffffff
                        private_public, private | 2 true
                        x private_public, all | 6 true
                        private_public, domain | 0 false
                        !! private, private_public | 2 true      !!!! if only private fw, but both
                    */

                    //long firewall_profiles_enabled = 0x0;
                    //long network_categories_connected = 0x0;

                    //// testing
                    //firewall_profiles_enabled |= NET_FW_PROFILE2_PRIVATE;

                    //network_categories_connected |= NET_FW_PROFILE2_PRIVATE;
                    //network_categories_connected |= NET_FW_PROFILE2_PUBLIC;

                    //printf("\n\n");


                    //bool mask2 = (firewall_profiles_enabled & network_categories_connected) == network_categories_connected;
                    //printf(std::format("{0}, {1}", network_categories_connected, mask2 ? "true" : "false").c_str());

                    //printf("\n\n");

                    /*
                        WITH bool mask2 = (firewall_profiles_enabled & network_categories_connected) == network_categories_connected;

                        (firewall profiles enabled) & (network categories connected)
                        x not gonna happen

                        public, public | true
                        public, private | false
                        private, public | false
                        domain, private | false
                        x domain, all | false
                        all, private | true                                     # still works :D

                        private_public, private | true
                        x private_public, all | false                           # different
                        private_public, domain | false
                        !! private, private_public |                            # good, this is what we wanted
                    */
                }
            }

            /*if (this->networkInfo.connected_networks == 0 && ImGui::GetCurrentContext() != nullptr)
            {
                ImGui::OpenPopup("offline");
            }*/

        Cleanup:

            // CoInitialize will return S_FALSE if it has already been initialized in the calling thread.However, for both invocations that return S_OK and S_FALSE there needs to be a CoUninitialize call.The number of calls to this functions get counted, and only when the number of CoUninitialize equals that of CoInitialize it will actually uninitialize things.
            // So in conclusion, a second call is harmless, and there are no problems with calling this pair of functions more than once.

            if (pNetworkListManager != nullptr)pNetworkListManager->Release();
            if (pEnum != nullptr) pEnum->Release();
            if (pINetwork != nullptr) pINetwork->Release();

            return SUCCEEDED(hr);
        }

        double modal_warnings_fixed_time = 0;

    public:

        //NetworkInformation getNetworkInfo()
        //{
        //    return this->networkInfo;
        //}

        ~_WindowsFirewallUtil()
        {
            // Release the INetFwRules object
            if (pFwRules != nullptr)
            {
                pFwRules->Release();
            }

            // Release INetFwPolicy2
            // this is updated once late on by our util WFCOMInitialize
            //  does it need to be a pointer? maybe for persistance
            if (pNetFwPolicy2 != nullptr)
            {
                pNetFwPolicy2->Release();
            }

            // this can't be destroyed until later
            // https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex
            if (SUCCEEDED(hrComInit))
            {
                printf("\ncounit beta\n");
                CoUninitialize();
            }
        }
        _WindowsFirewallUtil() :
            /*hrComInit(
                CoInitializeEx(
                    0,
                    COINIT_APARTMENTTHREADED
                )
            )*/
            //hrComInit(S_OK),
            hrComInit(
                CoInitializeEx(0, COINIT_APARTMENTTHREADED)
            ),
            //networkInfo({ 0, 0x0, 0x0 })
            networkInfo({ 0, 0x0, 0x0, false, 0.0 })
        {
            // TODO figure out which need to be class persistant
            //HRESULT hrComInit = S_OK;
            HRESULT hr = S_OK;


            // TODO nullptrptr
            //INetFwPolicy2* pNetFwPolicy2 = nullptr;
            //INetFwRules* pFwRules = nullptr;
            //INetFwRule* pFwRule = nullptr;

            // Initialize COM.
            //this->hrComInit = CoInitializeEx(
            //    0,
            //    COINIT_APARTMENTTHREADED
            //);

            // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
            // initialized with a different mode. Since we don't care what the mode is,
            // we'll just use the existing mode.
            if (hrComInit != RPC_E_CHANGED_MODE)
            {
                if (FAILED(hrComInit))
                {
                    this->failH("CoInitializeEx failed", hrComInit);

                    // CoUninitialize();
                    // should get cleaned up when we check for fail and destroy manually

                    // NOTE if anything else is above, make sure to destroy it
                    return;
                }
            }

            // Retrieve INetFwPolicy2
            hr = WFCOMInitialize(&pNetFwPolicy2);
            if (FAILED(hr))
            {
                this->failH("WFCOMInitialize failed?", hr);
                return;
            }

            // ensure that the network type active is bound to an enabled firewall profile.
            // ex: wifi is a private network, make sure the private profile is one the active profiles in the firewall
            {
                // get current types enabled
                //hr = this->pNetFwPolicy2->get_CurrentProfileTypes(&(this->current_network_profile_type_bitmask));

                // get network type active

            }



            // Retrieve INetFwRules
            hr = this->pNetFwPolicy2->get_Rules(&pFwRules);
            if (FAILED(hr))
            {
                this->failH("get_Rules failed", hr);
                return;
            }

            // TODO
            //pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_ALL)
            //this->windows_firewall_enabled = pNetFwPolicy2->get_FirewallEnabled();





            // TODO



            //https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ics/working-with-multiple-profiles/
            //https://learn.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN


            // TODO


            // verify connected networks and firewall profiles
            this->refetchNetworkStatus();

        }

        void _RenderUI()
        {



            /*bool verifiedProfiles =
                this->networkInfo.firewall_profiles_enabled != 0x0
                && ((this->networkInfo.firewall_profiles_enabled & this->networkInfo.network_profiles_connected) == this->networkInfo.network_profiles_connected)
            ;*/
            // do a bitmask shadow;

            //ImGui::Text(ImGui::IsPopupOpen("warnings") ? "mismatch" : "verified profiles");

            /*if (ImGui::Button("disable")) {
                this->WindowsFirewallToggle(NET_FW_PROFILE2_PUBLIC, false);
                this->modal_simple = true;
                this->refetchNetworkStatus();
            }*/

            /*if (this->networkInfo.connected_networks == 0)
            {
                ImGui::TextColored(ImVec4{ 1, 0, 1, 1 }, "note:");
                ImGui::SameLine();
                ImGui::TextWrapped("not currently connected to any networks");
            }*/

            /*if (this->isFailed())*/
                            /**/
                            //ImGui::SetWindowSize(ImVec2(432, 0), ImGuiCond_Once);

            /*if (ImGui::Button("hi"))
            {
                ImGui::OpenPopup("warnings");

            }*/

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
        bool add_rule(Endpoint* e, bool enabled = false, NET_FW_PROFILE_TYPE2_ profile = NET_FW_PROFILE2_ALL)
        {

            //HRESULT hrComInit = S_OK;
            //HRESULT hr = S_OK;

            //INetFwPolicy2* pNetFwPolicy2 = nullptr;
            //INetFwRules* pFwRules = nullptr;
            INetFwRule* pFwRule = nullptr;

            //long CurrentProfilesBitMask = 0;
            //long CurrentProfilesBitMask = profile;

            //BSTR bstrRuleName = SysAllocString(std::wstring(e.title.begin(), e.title.end()).c_str());
            BSTR bstrRuleName = _com_util::ConvertStringToBSTR(e->title.c_str());
            BSTR bstrRuleDescription = _com_util::ConvertStringToBSTR(e->_firewall_rule_description.c_str());
            BSTR bstrRuleRAddresses = _com_util::ConvertStringToBSTR(e->_firewall_rule_address.c_str());
            //BSTR bstrRuleApplication = SysAllocString(L"%programfiles%\\MyApplication.exe");
            //BSTR bstrRuleRPorts = SysAllocString(L"35.236.192.0-35.236.255.255, 35.199.0.0-35.199.63.255, 34.124.0.0/21, 34.23.0.0/16");
            //BSTR bstrRuleRAddresses = _com_util::ConvertStringToBSTR("35.236.192.0-35.236.255.255, 35.199.0.0-35.199.63.255, 34.124.0.0/21, 34.23.0.0/16\0");
            //BSTR bstrRuleRAddresses = _com_util::ConvertStringToBSTR("35.236.192.0-35.236.255.255,35.199.0.0-35.199.63.255,34.124.0.0/21,34.23.0.0/16");
            //BSTR bstrRuleRPorts = SysAllocString(L"134.170.30.202");
            BSTR bstrRuleGroup = _com_util::ConvertStringToBSTR(this->_group_name.c_str());

            HRESULT hr = S_OK;

            // When possible we avoid adding firewall rules to the Public profile.
            // If Public is currently active and it is not the only active profile, we remove it from the bitmask
            /*if ((CurrentProfilesBitMask & NET_FW_PROFILE2_PUBLIC) &&
                (CurrentProfilesBitMask != NET_FW_PROFILE2_PUBLIC))
            {
                CurrentProfilesBitMask ^= NET_FW_PROFILE2_PUBLIC;
            }*/


            // Create a new Firewall Rule object.
            // TODO https://stackoverflow.com/questions/68870009/equivalent-of-python-walrus-operator-in-c11
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
            //pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);

            // TODO disable stuff i don't need to set
            pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
            //pFwRule->put_LocalPorts(bstrRuleLPorts);






            // figure out why ports might not be set. could be order.

            //pFwRule->put_RemotePorts(NET_FW_PORT);


            pFwRule->put_RemoteAddresses(bstrRuleRAddresses);
            pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
            pFwRule->put_Grouping(bstrRuleGroup);
            pFwRule->put_Profiles(profile);
            //pFwRule->put_Action(NET_FW_ACTION_ALLOW);
            pFwRule->put_Action(NET_FW_ACTION_BLOCK);
            pFwRule->put_Enabled(enabled ? VARIANT_TRUE : VARIANT_FALSE);

            // Add the Firewall Rule
            hr = pFwRules->Add(pFwRule);
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

            HRESULT hr = this->pNetFwPolicy2->EnableRuleGroup(NET_FW_PROFILE2_ALL, groupMatch, enable);
            if (FAILED(hr))
            {
                this->failH("failed to enable group", hr);
            }

            SysFreeString(groupMatch);

            /*VARIANT_BOOL t;
            this->pNetFwPolicy2->IsRuleGroupEnabled(NET_FW_PROFILE2_ALL, groupMatch, &t);*/

            //this->group_enabled = (t == 0) ? false : true;

            return SUCCEEDED(hr);
        }

        void removeAllRulesForGroup()
        {
            BSTR groupMatch = _com_util::ConvertStringToBSTR(this->_group_name.c_str());

            HRESULT hr = S_OK;

            ULONG cFetched = 0;
            CComVariant var;

            IUnknown* pEnumerator;
            IEnumVARIANT* pVariant = NULL;

            INetFwRule* pFwRule = NULL;

            // Iterate through all of the rules in pFwRules
            pFwRules->get__NewEnum(&pEnumerator);

            if (pEnumerator)
            {
                hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pVariant);
            }

            while (SUCCEEDED(hr) && hr != S_FALSE)
            {
                var.Clear();
                hr = pVariant->Next(1, &var, &cFetched);

                if (S_FALSE != hr)
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = var.ChangeType(VT_DISPATCH);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = (V_DISPATCH(&var))->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**> (&pFwRule));
                    }

                    if (SUCCEEDED(hr))
                    {
                        BSTR groupName;
                        BSTR ruleName;

                        if (FAILED(pFwRule->get_Grouping(&groupName)))
                            continue;

                        if (FAILED(pFwRule->get_Name(&ruleName)))
                            continue;

                        if (groupName == NULL)
                            continue;

                        const std::string s_groupName(_bstr_t(groupName, true));
                        const std::string s_ruleName(_bstr_t(ruleName, true));

                        if (this->_group_name == s_groupName)
                        {
                            pFwRules->Remove(ruleName);
                        }
                    }
                }
            }

            wprintf(L"Removed all firewall rules for group: \"%s\"\n", groupMatch);

            Cleanup:

                SysFreeString(groupMatch);

                // Release pFwRule
                if (pFwRule != NULL)
                {
                    pFwRule->Release();
                }
        }

        // returns true if succeeded
        void filter(std::string group, std::vector<int>* items)
        {
            //    HRESULT hr = S_OK;

            //    IUnknown* pEnumerator;
            //    IEnumVARIANT* pVariant = nullptr;

            //    pFwRules->get__NewEnum(&pEnumerator);
            //    if (pEnumerator)
            //    {
            //        hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pVariant);
            //    }

            //    ULONG cFetched = 0;
            //    CComVariant var;
            //    INetFwRule* pFwRule = nullptr;

            //    // BSTR to match group to - "stormy.gg"
            //    // destroy after
            //    BSTR groupMatch = _com_util::ConvertStringToBSTR(group.c_str());

            //    while (SUCCEEDED(hr) && hr != S_FALSE)
            //    {
            //        var.Clear();
            //        hr = pVariant->Next(1, &var, &cFetched);

            //        if (SUCCEEDED(hr) && S_FALSE != hr)
            //        {
            //            if (
            //                SUCCEEDED(var.ChangeType(VT_DISPATCH))
            //                &&
            //                SUCCEEDED((V_DISPATCH(&var))->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**>(&pFwRule)))
            //                )
            //            {
            //                {
            //                    // TODO delete? use _bstr_t_ or ccombstr?
            //                    // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/automat/bstr
            //                    BSTR bstrVal;

            //                    //if (SUCCEEDED(pFwRule->get_Name(&bstrVal)))
            //                    if (SUCCEEDED(pFwRule->get_Grouping(&bstrVal)))
            //                    {
            //                        if (EqualBSTR(bstrVal, groupMatch, false))
            //                        {
            //                            _DumpFWRulesInCollection(pFwRule);
            //                            &items->push_back(pFwRule)
            //                        }
            //                        else
            //                        {
            //                            continue;
            //                        }
            //                    }

            //                }

            //            }
            //        }
            //    }

            //    wprintf(L"stormy.gg loaded: %d", this->stormyggCount);
            //    debug.print(std::format("stormy.gg loaded: {0}", this->stormyggCount));

            //    //long x, y;

            //    //this->tmp_rules->get_Count(&x);
            //    //pFwRules->get_Count(&y);

            //    //debug.print(std::format("saved: {0}, new: {1}", x, y));

            //    // destroy bstring
            //    SysFreeString(groupMatch);

            //    // Release pFwRule
            //    if (pFwRule != nullptr)
            //    {
            //        pFwRule->Release();
            //    }

            //    if (pFwRules != nullptr)
            //    {
            //        pFwRules->Release();
            //    }
            //    if (pEnumerator != nullptr)
            //    {
            //        pEnumerator->Release();
            //    }
            //}
        };

        // returns true if succeeded
        bool refetchStatus()
        {
            return this->refetchNetworkStatus();
        }

        // TODO use _com_ptr_t?
        // https://stackoverflow.com/a/60792713
        // memory leaks

        // NOTE - don't use the microsoft examples any more from the official site. they have memory leaks. ugh.
        // https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/Win7Samples/security/windowsfirewall/enumeratefirewallrules/EnumerateFirewallRules.cpp
        void syncFirewallEndpointState(std::vector<Endpoint>* endpoints, bool endpointDominant, bool only_unblocks = false)
        {
            HRESULT hr;

            // Retrieve INetFwPolicy2
            /*CComPtr<INetFwPolicy2> pNetFwPolicy2;
            hr = pNetFwPolicy2.CoCreateInstance(__uuidof(NetFwPolicy2));
            if (FAILED(hr))
            {
                wprintf(L"CoCreateInstance failed: 0x%08lx\n", hr);
                return;
            }

            // Retrieve INetFwRules
            CComPtr<INetFwRules> pFwRules;
            hr = pNetFwPolicy2->get_Rules(&pFwRules);
            if (FAILED(hr))
            {
                wprintf(L"get_Rules failed: 0x%08lx\n", hr);
                return;
            }

            // Obtain the number of Firewall rules
            long fwRuleCount;
            hr = pFwRules->get_Count(&fwRuleCount);
            if (FAILED(hr))
            {
                wprintf(L"get_Count failed: 0x%08lx\n", hr);
                return;
            }

            wprintf(L"The number of rules in the Windows Firewall are %d\n", fwRuleCount);
            */

            // Iterate through all of the rules in pFwRules
            CComPtr<IUnknown> pEnumerator;
            hr = pFwRules->get__NewEnum(&pEnumerator);
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
                    // Output the properties of this rule
                    // DumpFWRulesInCollection(pFwRule);

                    CComBSTR groupName;
                    if (SUCCEEDED(pFwRule->get_Grouping(&groupName)) && groupName)
                    {
                        const std::string s_groupName(_bstr_t(groupName, true));

                        if (this->_group_name == s_groupName)
                        {
                            CComBSTR ruleName;

                            if (SUCCEEDED(pFwRule->get_Name(&ruleName)) && ruleName)
                            {
                                const std::string s_ruleName(_bstr_t(ruleName, true));

                                VARIANT_BOOL __enabled;
                                if (FAILED(pFwRule->get_Enabled(&__enabled)))
                                    continue;

                                bool ruleEnabled;
                                ruleEnabled = (__enabled != VARIANT_FALSE);

                                for (auto& e : *endpoints)
                                {

                                    if (e.title == s_ruleName)
                                    {
                                        if (!e.active_desired_state != ruleEnabled && (!only_unblocks || (only_unblocks && (!e.active && e.active_desired_state))))
                                        {

                                            // if endpointDominant, set firewall to mirror endpoint state
                                            if (endpointDominant)
                                            {

                                                printf(std::format("({0}) firewall: {1}, ui: {2} . Setting firewall rule to match UI state\n", s_ruleName, ruleEnabled ? "block" : "allow", e.active ? "selected" : "not selected").c_str());
                                                if (FAILED(pFwRule->put_Enabled(e.active_desired_state ? VARIANT_FALSE : VARIANT_TRUE)))
                                                {
                                                    SysFreeString(groupName);
                                                    SysFreeString(ruleName);
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

                        }

                    }
                }
            }
        }
};

//std::wstring to_wstring(const std::string& stringToConvert)
//{
//    std::wstring wideString =
//        std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(stringToConvert);
//    return wideString;
//};

static void _DumpFWRulesInCollection(INetFwRule* FwRule) {
    variant_t InterfaceArray;
    variant_t InterfaceString;

    VARIANT_BOOL bEnabled;
    BSTR bstrVal;

    long lVal = 0;
    long lProfileBitmask = 0;

    NET_FW_RULE_DIRECTION fwDirection;
    NET_FW_ACTION fwAction;

    struct ProfileMapElement
    {
        NET_FW_PROFILE_TYPE2 Id;
        LPCWSTR Name;
    };

    ProfileMapElement ProfileMap[3];
    ProfileMap[0].Id = NET_FW_PROFILE2_DOMAIN;
    ProfileMap[0].Name = L"Domain";
    ProfileMap[1].Id = NET_FW_PROFILE2_PRIVATE;
    ProfileMap[1].Name = L"Private";
    ProfileMap[2].Id = NET_FW_PROFILE2_PUBLIC;
    ProfileMap[2].Name = L"Public";

    wprintf(L"---------------------------------------------\n");

    if (SUCCEEDED(FwRule->get_Name(&bstrVal)))
    {
        wprintf(L"Name:             %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Description(&bstrVal)))
    {
        wprintf(L"Description:      %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_ApplicationName(&bstrVal)))
    {
        wprintf(L"Application Name: %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_ServiceName(&bstrVal)))
    {
        wprintf(L"Service Name:     %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Protocol(&lVal)))
    {
        switch (lVal)
        {
        case NET_FW_IP_PROTOCOL_TCP:

            wprintf(L"IP Protocol:      %s\n", NET_FW_IP_PROTOCOL_TCP_NAME);
            break;

        case NET_FW_IP_PROTOCOL_UDP:

            wprintf(L"IP Protocol:      %s\n", NET_FW_IP_PROTOCOL_UDP_NAME);
            break;

        default:

            break;
        }

        if (lVal != NET_FW_IP_VERSION_V4 && lVal != NET_FW_IP_VERSION_V6)
        {
            if (SUCCEEDED(FwRule->get_LocalPorts(&bstrVal)))
            {
                wprintf(L"Local Ports:      %s\n", bstrVal);
            }

            if (SUCCEEDED(FwRule->get_RemotePorts(&bstrVal)))
            {
                wprintf(L"Remote Ports:      %s\n", bstrVal);
            }
        }
        else
        {
            if (SUCCEEDED(FwRule->get_IcmpTypesAndCodes(&bstrVal)))
            {
                wprintf(L"ICMP TypeCode:      %s\n", bstrVal);
            }
        }
    }

    if (SUCCEEDED(FwRule->get_LocalAddresses(&bstrVal)))
    {
        wprintf(L"LocalAddresses:   %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_RemoteAddresses(&bstrVal)))
    {
        wprintf(L"RemoteAddresses:  %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Profiles(&lProfileBitmask)))
    {
        // The returned bitmask can have more than 1 bit set if multiple profiles 
        //   are active or current at the same time

        for (int i = 0; i < 3; i++)
        {
            if (lProfileBitmask & ProfileMap[i].Id)
            {
                wprintf(L"Profile:  %s\n", ProfileMap[i].Name);
            }
        }
    }

    if (SUCCEEDED(FwRule->get_Direction(&fwDirection)))
    {
        switch (fwDirection)
        {
        case NET_FW_RULE_DIR_IN:

            wprintf(L"Direction:        %s\n", NET_FW_RULE_DIR_IN_NAME);
            break;

        case NET_FW_RULE_DIR_OUT:

            wprintf(L"Direction:        %s\n", NET_FW_RULE_DIR_OUT_NAME);
            break;

        default:

            break;
        }
    }

    if (SUCCEEDED(FwRule->get_Action(&fwAction)))
    {
        switch (fwAction)
        {
        case NET_FW_ACTION_BLOCK:

            wprintf(L"Action:           %s\n", NET_FW_RULE_ACTION_BLOCK_NAME);
            break;

        case NET_FW_ACTION_ALLOW:

            wprintf(L"Action:           %s\n", NET_FW_RULE_ACTION_ALLOW_NAME);
            break;

        default:

            break;
        }
    }

    if (SUCCEEDED(FwRule->get_Interfaces(&InterfaceArray)))
    {
        if (InterfaceArray.vt != VT_EMPTY)
        {
            SAFEARRAY* pSa = nullptr;

            pSa = InterfaceArray.parray;

            for (long index = pSa->rgsabound->lLbound; index < (long)pSa->rgsabound->cElements; index++)
            {
                SafeArrayGetElement(pSa, &index, &InterfaceString);
                wprintf(L"Interfaces:       %s\n", (BSTR)InterfaceString.bstrVal);
            }
        }
    }

    if (SUCCEEDED(FwRule->get_InterfaceTypes(&bstrVal)))
    {
        wprintf(L"Interface Types:  %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Enabled(&bEnabled)))
    {
        if (bEnabled)
        {
            wprintf(L"Enabled:          %s\n", NET_FW_RULE_ENABLE_IN_NAME);
        }
        else
        {
            wprintf(L"Enabled:          %s\n", NET_FW_RULE_DISABLE_IN_NAME);
        }
    }

    if (SUCCEEDED(FwRule->get_Grouping(&bstrVal)))
    {
        wprintf(L"Grouping:         %s\n", bstrVal);
    }

    if (SUCCEEDED(FwRule->get_EdgeTraversal(&bEnabled)))
    {
        if (bEnabled)
        {
            wprintf(L"Edge Traversal:   %s\n", NET_FW_RULE_ENABLE_IN_NAME);
        }
        else
        {
            wprintf(L"Edge Traversal:   %s\n", NET_FW_RULE_DISABLE_IN_NAME);
        }
    }
}

