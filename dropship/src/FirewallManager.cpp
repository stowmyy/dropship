
#include "FirewallManager.h"

#ifdef _DEBUG
#include "DebugManager.h"
#endif

#include "DashboardManager.h"

#include "theme.h" // test

// FirewallManager firewallManager;

bool FirewallManager::AddFirewallRule(Endpoint* e, bool enabled)
{
    return this->_windowsFirewall->add_rule(e, enabled, NET_FW_PROFILE2_ALL);
}


// mirrors endpoint state
void FirewallManager::_syncEndpointsWithFirewall(std::vector<Endpoint>* endpoints)
{
    this->_windowsFirewall->syncFirewallEndpointState(endpoints, true);
}

// mirrors firewall state
void FirewallManager::_syncFirewallWithEndpoints(std::vector<Endpoint>* endpoints)
{
    this->_windowsFirewall->syncFirewallEndpointState(endpoints, false);
}

/*
    0) set firewall state to mirror endpoint state
    2) ..
    4) then set endpoint state to mirror firewall state
*/
void FirewallManager::sync(std::vector<Endpoint>* endpoints)
{
    printf("sync()\n");
    this->_syncEndpointsWithFirewall(endpoints);
    this->_syncFirewallWithEndpoints(endpoints);
}

// destroys previous rules
// puts in new rules and disables them
void FirewallManager::flushRules(std::vector<Endpoint>* endpoints)
{
    this->_windowsFirewall->removeAllRulesForGroup();

    printf("Adding rules..\n");
    for (auto &e : *endpoints)
    {
        this->_windowsFirewall->add_rule(&e, false);
    }
    printf("Added %d rules.\n", endpoints->size());

}

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


FirewallManager::~FirewallManager() {

    // TODO destroy util
    delete this->_windowsFirewall;

}

//void FirewallManager::load_rules()
//{
//    
//
//}


FirewallManager::FirewallManager() : /*_windowsFirewall(new _WindowsFirewallUtil()), */systemFirewallOn(false), stormyggCount(0)
// , windows_firewall_enabled(false)
{

    //this->_windowsFirewall =;
    /*std::thread([&]()
    {*/
        this->_windowsFirewall = new _WindowsFirewallUtil();
    //}).detach();



    // todo consolidate profile value
    //this->_windowsFirewall->WindowsFirewallIsOn(NET_FW_PROFILE2_ALL, &(this->systemFirewallOn));

    /*if (this->_windowsFirewall->isFailed())
    {
        debugManager.print(this->_windowsFirewall->isFailedReason());
        // delete firewall;
    }*/
    /*std::unique_ptr<_WindowsFirewallUtil> (new _WindowsFirewallUtil())*/;

    wprintf(L"done\n\n");

}

