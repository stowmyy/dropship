#pragma once

#include "imgui.h"

#include <vector>
#include <string>
#include <format>

#include "_windows_download.hpp"

#include "../src/sha512.hh"

#include "util.hpp"

struct AppDownloadState
{
    bool active; // is updating
    bool downloading; // is currently downloading a file
    float progress;
    std::string status;
    std::string appVersion;
};

class AppManager
{
    public:
        AppManager();
        // ~AppManager();
        void RenderInline(/* bool* p_open */);

    private:
        AppDownloadState downloadState;
};

extern AppManager appManager;

