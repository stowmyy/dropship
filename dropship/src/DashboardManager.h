#pragma once


#include "imgui.h"

//#include <stdio.h>


#include <string>
#include <vector>
#include <future>

#define half_pi 1.57079632679
#include <math.h> // sin
#include <chrono> // delay

#include "images.h" // cover image

#include <memory> // shared ptr

#include <thread> // multithreading without need for return variable
#include <mutex> // waiting until
#include <condition_variable> // waiting until


#include "theme.h" // theme, setTheme

// https://github.com/ocornut/imgui/issues/5216
#include "imgui_internal.h" // (Apr 20, 2022) pushItemFlag, popItemFlag

#include "manager.h" // : manager

#include "util.hpp"

#include <unordered_map>

#include "FirewallManager.h" // extern firewallManager


struct Process
{
    int pid;
    bool on; // is process running?
    ImageTexture icon;
    HWND window;
};


class DashboardManager : public manager
{
    public:
        DashboardManager();
        ~DashboardManager();
        //void RenderUI(/* bool* p_open */);
        void RenderInline(/* bool* p_open */);

    private:

        bool active = { false };

        float __date_new_selection = 0;

        std::unordered_map<std::string, Process> processes;

        void loadAssets();

        //float options_alpha = ImGui::GetStyle().Alpha;

        bool show_all = { false };

        //const ImU32 text_color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]); /*ImGui::ColorConvertFloat4ToU32({ .9, .9, .9, 1 });*/
        //const ImU32 text_color = ImGui::ColorConvertFloat4ToU32({ .9, .9, .9, 1 });
        //const ImU32 title_color = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 1 });

        THEME theme = THEME::dark;


        // other apps
        // struct ImageTexture _other_party_app_icon_0 = { ImageTexture{ nullptr, 0, 0 } };

        std::thread t_pinging;
        bool pinging; // checkbox
        //std::atomic_bool pinging; // checkbox
        //std::shared_ptr<std::atomic_bool> pinging; // checkbox
        //std::shared_ptr<bool> pinging; // checkbox

        void startPinging(int interval);

        //std::vector<Endpoint> endpoints;


        const std::unordered_map<std::string, std::string> ips;

        // TODO why are these pointers?
        std::vector<Endpoint> endpoints;

        //std::vector<std::future<bool>> ping_futures;

};

extern DashboardManager dashboardManager;
