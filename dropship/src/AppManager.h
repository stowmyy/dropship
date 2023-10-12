#pragma once

#include "imgui.h"

#include <vector>
#include <string>
#include <format>

#include "manager.h"

struct AppDownloadState
{
    bool downloading;
    float progress;
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

