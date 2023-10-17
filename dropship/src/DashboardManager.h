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

#include "util.h"


struct Endpoint {
    std::string name;
    std::string hostname;
    std::string iata_code;

    /*
        0 (>) = fine
        -1 = fail (timed out)
        -2 = block (any error)
        -3 (>) = unknown
    */
    int ping = -9;
    //std::shared_ptr<int> ping = std::make_shared<int>(-9);
    int display_ping = 0;

    bool selected = { true };

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

        void loadAssets();

        //float options_alpha = ImGui::GetStyle().Alpha;

        bool show_all = { false };

        //const ImU32 text_color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]); /*ImGui::ColorConvertFloat4ToU32({ .9, .9, .9, 1 });*/
        const ImU32 text_color = ImGui::ColorConvertFloat4ToU32({ .9, .9, .9, 1 });
        const ImU32 title_color = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 1 });

        THEME theme = THEME::dark;

        struct ImageTexture cover_texture = { ImageTexture{ nullptr, 0, 0 } };

        struct ImageTexture icon_options = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_maple_leaf = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_chain_slash = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_bolt = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_arrow = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_angle = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_outside_window = { ImageTexture{ nullptr, 0, 0 } };

        struct ImageTexture icon_wifi_slash = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_wifi_poor = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_wifi_fair = { ImageTexture{ nullptr, 0, 0 } };
        struct ImageTexture icon_wifi = { ImageTexture{ nullptr, 0, 0 } };

        struct ImageTexture background_texture = { ImageTexture{ nullptr, 0, 0 } };

        std::thread t_pinging;
        bool pinging; // checkbox
        //std::atomic_bool pinging; // checkbox
        //std::shared_ptr<std::atomic_bool> pinging; // checkbox
        //std::shared_ptr<bool> pinging; // checkbox

        void startPinging(int interval);

        //std::vector<Endpoint> endpoints;
        std::vector<std::shared_ptr<Endpoint>> endpoints;

        //std::vector<std::future<bool>> ping_futures;

};

extern DashboardManager dashboardManager;
