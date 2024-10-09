#pragma once

// data
#include "components/Endpoint.h"
// managers
#ifdef _DEBUG
    #include "managers/Debug.h"
#endif
#include "managers/Settings.h"
#include "managers/Dashboard.h"

#include "managers/Firewall.h"
#include "managers/Update.h"


namespace App
{
    void render(bool* p_open);

    void renderError(std::string error);
};
