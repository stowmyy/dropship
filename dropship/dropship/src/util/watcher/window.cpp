#include "pch.h"

#include "window.h"

/* functional */
namespace util::watcher::window {
    std::optional<::HWND> findWindow(std::string& window_name) {
        std::optional<::HWND> match = std::nullopt;

        for (::HWND hwnd = ::GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) {
            int length = GetWindowTextLength(hwnd);
            if (length == 0) continue;

            char* title = new char[length + 1];
            {
                ::GetWindowTextA(hwnd, title, length + 1);

                if (strcmp(title, window_name.c_str()) == 0)
                {
                    // std::cout << "HWND: " << hwnd << " Title: " << s_title << std::endl;
                    match = std::make_optional<::HWND>(hwnd);
                    break;
                }
            }
            delete[] title;
        }

        return match;
    }

    bool isWindowOpen(std::string& window_name) {
        auto window = findWindow(window_name);
        return window && ::IsWindow(window.value());
    }
}

namespace util::watcher::window {

    WindowWatcher::WindowWatcher(std::string window_name) :
        _window_name(window_name)
    {
        this->start();
    }

    WindowWatcher::~WindowWatcher() {
        this->stop();
    }

    bool WindowWatcher::isActive() { return this->_window_open; }

    void WindowWatcher::stop() {
        this->_watching = false;
        this->__watcher_future_condition_variable.notify_all();
        if (this->_watcher_future.valid()) this->_watcher_future.get();
    }

    void WindowWatcher::start() {
        this->_watching = true;

        this->_watcher_future = std::async(std::launch::async, [this] {
            try {
                while (this->_watching) {

                    auto open = util::watcher::window::isWindowOpen(this->_window_name);
                    if (this->_window_open != open) {
                        this->_window_open = open;
                    }

#ifdef _DEBUG
                    //std::println("window '{}' open: {}", this->_window_name, this->_window_open ? "true" : "false");
#endif

                    std::unique_lock<std::mutex> lock(this->__watcher_future_condition_variable_mutex);
                    /* alternative: extra condition on notify
                    *
                        if (cv.wait_for(lk, watcher_delay, [this] { return !this->watching; }))
                        {
                            println("early exit");
                        }
                        else
                        {
                            println("no early exit");
                        }
                    */

                    /* wait for notification
                    *
                        auto res = cv.wait_for(lk, this->watcher_delay);
                        if (res == std::cv_status::timeout) {
                            println("no early exit");
                        }
                        else if (res == std::cv_status::no_timeout) {
                            println("early exit");
                        }
                    */
                    //this->__watcher_future_condition_variable.wait_for(lock, this->watcher_delay);
                    this->__watcher_future_condition_variable.wait_for(lock, watcher_delay, [this] { return !this->_watching; });
                }
            }
            catch (const std::exception& e)
            {
                std::println("watcher error: {}", e.what());
                this->stop();
                // todo notify
            }
            catch (...)
            {
                std::println("watcher error: {}", "unknown");
                this->stop();
                // todo notify
            }
        });
    }
}

/*
    bool __previous__application_open = appStore.application_open;

    static const std::string process_name = "Overwatch.exe";
    static const std::string module_name = "Overwatch.exe";

    int pid = find_process(process_name);
    HWND window = find_window("Overwatch");

    appStore.application_open = window;
    this->processes["Overwatch.exe"].on = window;
    this->processes["Overwatch.exe"].window = window;

    if (__previous__application_open && !appStore.application_open)
    {
        // V2 COMMENTED
        std::jthread([&]() {
            firewallManager.sync(&(this->endpoints));
            }).detach();

            appStore.dashboard.heading = __default__appStore.dashboard.heading;

            this->game_restart_required = false;
    }

    if (!__previous__application_open && appStore.application_open)
    {
        // appStore.dashboard.heading = "Blocking new servers won't take effect until Overwatch is restarted.";
    }

    if (this->processes[process_name].icon.texture == nullptr)
    {
        // just prints stuff for now
        get_module(pid, module_name);

        loadPicture("process_icon_overwatch", "png", &(this->processes[process_name].icon.texture), &(this->processes[process_name].icon.width), &(this->processes[process_name].icon.height));
    }
*/
