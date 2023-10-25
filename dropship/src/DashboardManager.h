#pragma once

#include <string>
#include <vector>

#define half_pi 1.57079632679
#include <math.h> // sin
#include <chrono> // delay
#include <unordered_map>

#include <thread> // multithreading without need for return variable
#include <mutex> // waiting until
#include <condition_variable> // waiting until

// https://github.com/ocornut/imgui/issues/5216
#include "imgui.h"
#include "imgui_internal.h" // (Apr 20, 2022) pushItemFlag, popItemFlag

#include "images.h" // cover image
#include "theme.h" // theme, setTheme
#include "manager.h" // : manager
#include "util.hpp"

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
        void RenderInline(/* bool* p_open */);

    private:
        bool active = { false };
        double __date_new_selection = 0;
        bool show_all = { false };
        bool pinging;

        const std::unordered_map<std::string, std::string> ips;
        std::vector<Endpoint> endpoints;

        std::unordered_map<std::string, Process> processes;
        std::thread t_pinging;
        THEME theme = THEME::dark;

        void loadAssets();
        void startPinging(int interval);
};

extern DashboardManager dashboardManager;
