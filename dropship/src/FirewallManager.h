#pragma once

#include <string>


// TODO prune
#include <windows.h>
#include <stdio.h>
#include <comutil.h>
#include <atlcomcli.h>
#include <netfw.h>


//#include "debug.h"

//int __cdecl main_func();

//extern std::string global_message;
//extern Debug debug;
#include "failable.h"

#include "imgui.h"

#include "_WindowsFirewallUtil.h"

// windows 10

class FirewallManager : public failable
{
    public:
        FirewallManager();
        ~FirewallManager();

        void flushRules(std::vector<Endpoint>* endpoints);

        void RenderInline(/* bool* p_open */);

        bool AddFirewallRule(Endpoint* e, bool enabled = false);


        //bool isWindowsFirewallEnabled();
        void sync(std::vector<Endpoint>* endpoints);

        // mirrors endpoint state
        void _syncEndpointsWithFirewall(std::vector<Endpoint>* endpoints);

        // mirrors firewall state
        void _syncFirewallWithEndpoints(std::vector<Endpoint>* endpoints);

    private:

        _WindowsFirewallUtil* _windowsFirewall;



        // NOTE if i ever dare to change this, i gotta implement something to destroy old rules with this name
        // stormy.gg felt too professional
        // #define GROUP_NAME L"stormy";

        const std::string _FW_GROUP = "stormy.gg";
        const NET_FW_PROFILE_TYPE2_ _FW_PROFILE = NET_FW_PROFILE2_ALL;

        bool group_enabled;
        int stormyggCount;

        bool systemFirewallOn;

        bool miniModal = false;

        /*void load_rules();
        void add_rule(NET_FW_PROFILE_TYPE2_ profile);

        void toggle_group()*/;

        //bool windows_firewall_enabled;


        // props here need to persist past initlization
        // otherwise delete prop at end of firewallmanager()
        //HRESULT hrComInit;
        //INetFwRule* pFwRule;
};

extern FirewallManager firewallManager;
