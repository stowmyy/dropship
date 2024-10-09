#pragma once


#include "components/Endpoint.h"
#include "components/PopupButton.h"

#include "managers/Settings.h"
#include "util/watcher/window.h"

//const PopupButton header_actions[2] = { "Volvo", "BMW", "Ford", "Mazda" };aaaaaa

class Dashboard
{
public:
    Dashboard();
    ~Dashboard();
    void render(/* bool* p_open */);

private:

    bool footer = { false };

    //std::vector<PopupButton> const header_actions;
    //std::vector<PopupButton*> header_actions;
    std::vector<std::unique_ptr<PopupButton>> header_actions;
    //std::unique_ptr<std::vector<std::shared_ptr<PopupButton>>> const& header_actions;
};