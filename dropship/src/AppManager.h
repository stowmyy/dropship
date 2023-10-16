#pragma once

#include "imgui.h"

#include <vector>
#include <string>
#include <format>

#include "manager.h"
#include "_windows_download.hpp"

#include "../src/sha512.hh"

struct OPTIONS
{
    bool auto_update;
};

struct AppDownloadState
{
    bool active; // is updating
    bool downloading; // is currently downloading a file
    float progress;
    std::string status;
};

class AppManager : public manager
{
    public:
        AppManager();
        // ~AppManager();
        void RenderInline(/* bool* p_open */);

    private:
        AppDownloadState downloadState;
        std::string appVersion;
};

extern AppManager appManager;

