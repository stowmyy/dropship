
#include "FirewallManager.h"

#include "DebugManager.h"

#include "theme.h" // test

// FirewallManager firewallManager;

void FirewallManager::RenderInline(/* bool* p_open */) {

    // TODO move to threaded scheduler?
    if (ImGui::GetFrameCount() % (60 * 9) == 0)
    {
        this->_windowsFirewall->refetchStatus();
    }

    {
        this->_windowsFirewall->_RenderUI();
    }

    {
        if (this->isFailed())
            ImGui::OpenPopup("failed");
        if (ImGui::BeginPopupModal("failed", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        {

            ImGui::TextColored(ImVec4{ 1, 0, 0, 1 }, "error:");
            ImGui::SameLine();
            ImGui::Text(this->isFailedReason().c_str());
            ImGui::Text("debugging %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);

            
            ImGui::EndPopup();
        }
    }



    #ifdef _DEBUG
    {
        ImGui::Begin("debug");
        if (ImGui::CollapsingHeader("firewall.h", ImGuiTreeNodeFlags_None))
        {
            //ImGui::BeginChild("firewall.h", { 0, 200 }, true);
            //ImGui::SeparatorText("firewall.h");
            ImGui::Indent();


            //ImGui::BeginChild("dump", { 0, 90 }, true);
            ImGui::Indent();

            DebugManager::_imgui_debug_status_text(this->systemFirewallOn, this->systemFirewallOn ? "system firewall is on" : "system firewall is off");

            ImGui::Unindent();
            //ImGui::EndChild();

            ImGui::SeparatorText("actions");
            ImGui::Indent();

            {
                if (ImGui::Button("fail", { 60, 0 }))
                {
                    this->fail("test failure");
                }

                ImGui::SameLine();
                if (ImGui::Button("new", { 60, 0 }))
                {
                    //this->add_rule(NET_FW_PROFILE2_ALL);
                    this->_windowsFirewall->add_rule(this->_FW_GROUP, NET_FW_PROFILE2_ALL);
                }

                ImGui::SameLine();
                if (ImGui::Button("wf.msc", { 60, 0 }))
                {
                    system("wf.msc");
                }
            }

            ImGui::Unindent();

            ImGui::Unindent();

            //ImGui::EndChild();
        }


        ImGui::End();
    }
    #endif
}



// Forward declarations

// TODO move to util
//HRESULT     WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2);



//bool FirewallManager::isWindowsFirewallEnabled()
//{
//    return this->windows_firewall_enabled;
//}



//void FirewallManager::add_rule()
//{
//    //if (SUCCESS(util.add_rule(..))
//    // or if we don't what HRSELUTL in this class, bool it and succes in util.
//}

FirewallManager::~FirewallManager() {

    // TODO destroy util
    delete this->_windowsFirewall;

}

//void FirewallManager::load_rules()
//{
//    
//
//}


FirewallManager::FirewallManager() : _windowsFirewall(new _WindowsFirewallUtil()), systemFirewallOn(false), stormyggCount(0)
// , windows_firewall_enabled(false)
{

    //this->_windowsFirewall =;



    // todo consolidate profile value
    //this->_windowsFirewall->WindowsFirewallIsOn(NET_FW_PROFILE2_ALL, &(this->systemFirewallOn));

    if (this->_windowsFirewall->isFailed())
    {
        debugManager.print(this->_windowsFirewall->isFailedReason());
        // delete firewall;
    }
    /*std::unique_ptr<_WindowsFirewallUtil> (new _WindowsFirewallUtil())*/;

    wprintf(L"done\n\n");

}

