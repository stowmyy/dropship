#pragma once

#include <filesystem>

#include "core/Settings.h"

#include "util/sha512.hh"

#include "util/win/win_download/download_file.h"

// TODO use zoe and remove from project
#include "util/win/win_download/download_txt.h"

#include "util/trim.h"

const std::string __UPDATE_VERSION_URI = "https://github.com/stowmyy/dropship/releases/latest/download/version.txt";
const std::string __UPDATE_EXE_URI = "https://github.com/stowmyy/dropship/releases/latest/download/dropship.exe";

//struct AppDownloadState
//{
//    bool active; // is updating
//    bool downloading; // is currently downloading a file
//    float progress;
//    std::string status;
//    std::string appVersion;
//};

class Updater
{
    public:
        Updater();
        ~Updater();
        void render(/* bool* p_open */);

    private:

        // todo
        //inline static const char* __updating_popup_name { "#updating" };

        //bool display;
        bool downloading;
        float progress;
        std::string version;
        std::string description;

        bool update_available;


        //AppDownloadState downloadState;
        void getMeta();
        bool checkForUpdate();
        void update();

        std::future<void> update_future;
        std::future<void> version_future;

        std::wstring __exe_path;
        std::string __exe_hash;
};

//extern UpdateManager updateManager;

