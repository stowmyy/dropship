#pragma once

// data
#include "components/Endpoint.h"
// managers
#ifdef _DEBUG
    #include "core/Debug.h"
#endif
#include "core/Settings.h"
#include "core/Dashboard.h"

#include "core/Firewall.h"
#include "core/Update.h"


namespace App
{
    void render(bool* p_open);

    void renderError(std::string error);
};
