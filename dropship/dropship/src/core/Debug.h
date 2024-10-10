#pragma once

#include "images.h"


class Debug
{

    public:
        Debug();
        void render(/* bool* p_open */);
        void print(std::string text);

        static void _imgui_debug_status_text(bool happy, std::string text);

    private:
        std::vector<std::string> items;
        bool demo_window_open;

};

//extern DebugManager debugManager;

