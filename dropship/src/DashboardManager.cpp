

// for std::min
#define NOMINMAX

// needs to be included before browser
#include "_windows_ping.hpp"
#include "_windows_domain.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS // https://github.com/ocornut/imgui/issues/2832
#include "DashboardManager.h"
// DashboardManager dashboardManager;

std::condition_variable cv; // for stopping pinging early
std::mutex cv_m; // for stopping pinging early

//extern ImFont* font_industry_bold;
//extern ImFont* font_industry_medium;

extern ImFont* font_title;
extern ImFont* font_subtitle;
extern ImFont* font_text;

extern OPTIONS options;
extern AppStore appStore;

void DashboardManager::startPinging(int interval = 9000)
{

    std::thread thread = std::thread([&, interval]()
    {
        while (this->pinging)
        {
            for (auto& endpoint : this->endpoints)
            {
                std::thread([&endpoint]()
                {
                    _windows_ping(endpoint->hostname, &(endpoint->ping), 2000);
                }).detach();

            }
            
            //std::this_thread::sleep_for(std::chrono::milliseconds(interval));

            std::unique_lock<std::mutex> lk(cv_m);
            if (cv.wait_for(lk, 1 * std::chrono::milliseconds(interval), [&] { return this->pinging == false; }))
            {
                //printf(std::format("ep1 loaded in time with ping {0}\n", pinging ? "true" : "false").c_str());
            }
            else
            {
                //printf(std::format("ep1 timed out before loading ping {0}\n", pinging ? "true" : "false").c_str());
            }
        }

        printf("done pinging\n");

    });

    thread.detach();


}

DashboardManager::~DashboardManager()
{
    this->pinging = false;
    //this->pinging->store(false);
}


/*
    current way i'm getting ips:
        > tracert any edge ip
        > get router ip, 2nd from last?
*/
DashboardManager::DashboardManager() : endpoints({

            //{ "USA - EAST", "", "gue1"},
            //{ "USA - CENTRAL", "24.105.62.129" },
            // ord1, chicago


            /*
            std::make_shared<Endpoint>("NETHERLANDS", "9.9.9.9", "ord1"),
            std::make_shared<Endpoint>("FRANCE", "9.9.9.9 - 9.9.9.9 (+4)", "ord1"),
            std::make_shared<Endpoint>("BRAZIL 2", "9.9.9.9", "ord1"),
            std::make_shared<Endpoint>("FINLAND 2", "9.9.9.9", "ord1"),
            std::make_shared<Endpoint>("SINGAPORE 2", "9.9.9.9", "ord1"),
            std::make_shared<Endpoint>("JAPAN 2", "9.9.9.9", "ord1"),
            std::make_shared<Endpoint>("SOUTH KOREA", "211.234.110.1", ""),
            std::make_shared<Endpoint>("USA - WEST", "24.105.30.129", "lax1"), // lax-eqla1-ia-bons-03.as57976.net
            std::make_shared<Endpoint>("USA - CENTRAL", "24.105.62.129", "ord1"),
            std::make_shared<Endpoint>("AUSTRALIA 3", "9.9.9.9", "ord1"),
            std::make_shared<Endpoint>("TAIWAN", "9.9.9.9", "ord1"),
            */

            std::make_shared<Endpoint>("USA - WEST", "24.105.30.129", "lax1"),
            std::make_shared<Endpoint>("USA - CENTRAL", "24.105.62.129", "ord1"),


            // these two are same
            //{ "west", "lax-eqla1-ia-bons-03.as57976.net" },

            //{ "‚¢‚í‚«Žs", "dynamodb.ap-northeast-1.amazonaws.com" },
            //{ "JAPAN 2", "dynamodb.ap-northeast-1.amazonaws.com" },
            //std::make_shared<Endpoint>("JAPAN 2", "dynamodb.ap-northeast-1.amazonaws.com"),
            //std::make_shared<Endpoint>("TAIWAN", "203.66.81.98"),
            //std::make_shared<Endpoint>("GERMANY", "127.0.0.1", "gew3"),

    }),
    pinging(true)
    //pinging(std::make_shared<std::atomic_bool>(true))
    //pinging(std::make_shared<bool>(true))
{

    //bool done = loadPicture("cover_browser", "jpg", &cover_texture.texture, &cover_texture.width, &cover_texture.height);

    this->startPinging();

    Process p = {
        .pid = 0,
        .on = false,
        .icon = { ImageTexture{ nullptr, 0, 0 } },
        .window = 0,
    };

    this->processes["Overwatch.exe"] = p;

    std::thread([&]()
        {

            // wait until imgui is loaded
            while (ImGui::GetCurrentContext() == nullptr)
                std::this_thread::sleep_for(std::chrono::milliseconds(16));

            while (true)
            {

                {

                    static const std::string process_name = "Overwatch.exe";
                    static const std::string module_name = "Overwatch.exe";

                    int pid = find_process (process_name);
                    HWND window = find_window ("Overwatch");

                    std::cout << window << std::endl;


                    {
                        if (this->processes[process_name].icon.texture == nullptr)
                        {

                            // TODO
                            // try to extract png icon from exe again. this was really hard, find a library if possible.

                            //std::vector<unsigned char> buffer;
                            //ZeroMemory(&bmp, sizeof(bmp));

                            /*if (get_module_icon(pid, module_name, &buffer))
                            {
                                bool worked = _loadPicture(buffer.data(), sizeof(buffer.data()), &(this->_other_party_app_icon_0.texture), &(this->_other_party_app_icon_0.width), &(this->_other_party_app_icon_0.height));
                                printf(worked ? "image loaded\n" : "image failed\n");
                            }*/


                            // just prints stuff for now
                            get_module(pid, module_name);
                            
                            loadPicture("process_icon_overwatch", "png", &(this->processes[process_name].icon.texture), &(this->processes[process_name].icon.width), &(this->processes[process_name].icon.height));

                        }
                    }

                    if (pid)
                        printf("PID = %d\n", pid);
                    else
                        printf("(process) no pid\n");


                    if (window)
                        printf("window found\n");
                    else
                        printf("no window\n");

                    this->processes[process_name].on = window;
                    this->processes[process_name].window = window;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
        }).detach();

    //::global_message = done ? "done cover_browser.jpg" : "D: failed.";
}

void DashboardManager::loadAssets() {


    static const std::vector<std::string> textures = {
        "icon_options.png",
        "icon_maple_leaf.png",
        "icon_chain_slash.png",
        "icon_bolt.png",
        "icon_outside_window.png",

        "icon_wifi_slash.png",
        "icon_wifi_poor.png",
        "icon_wifi_fair.png",
        "icon_wifi.png",

        "icon_allow.png",
        "icon_block.png",

        //"icon_arrow.png",
        "icon_angle.png",

        "image_background.png",
    };

    for (std::string texture : textures)
    {
        std::string name = texture.substr(0, texture.find("."));
        std::string type = texture.substr(texture.find(".") + 1);

        bool loaded = _add_texture(name, type);

        std::cout << (loaded ? "worked" : "failed") << " " << name << "." << type << std::endl;
    }
}



void DashboardManager::RenderInline(/* bool* p_open */)
{

    if (ImGui::GetCurrentContext() != nullptr && ImGui::IsWindowAppearing())
    {
        this->loadAssets();
    }

    {
        /*if (!ImGui::IsWindowAppearing() && (!this->active && !ImGui::IsPopupOpen("deactivated")))
            ImGui::OpenPopup("deactivated");

        static const auto transparent = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0 });
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, transparent);
        if (ImGui::BeginPopupModal("deactivated", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground))
        {
            ImGui::TextColored(ImColor::HSV(0, .2, 1), "deactivated");
            ToggleButton("test", &(this->active));
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(1);*/
    }

    if (ImGui::GetFrameCount() == 8)
    {
        
    }

    {

        // makeshift initial sorting
        //if (ImGui::GetFrameCount() == 20)
            //ImGui

        // move the numbers around slightly in between pings >:p
        // tricky tricky
        if (this->pinging && (ImGui::GetFrameCount() % 200) == 0)
        {
            const int max = 1;
            const int min = -2;

            for (auto& endpoint : endpoints)
            {
                if (endpoint->display_ping != endpoint->ping)
                    continue;

                if (endpoint->ping < 0)
                    continue;

                const int range = max - min + 1;
                const int num = rand() % range + min;

                //endpoint.display_ping += num;
                endpoint->display_ping = std::max(0, endpoint->display_ping + num);
            }
        }

        for (auto& endpoint : endpoints)
        {
            if (endpoint->display_ping != endpoint->ping)
            {

                if (endpoint->ping > 0 && endpoint->display_ping <= 0)
                {
                    endpoint->display_ping = endpoint->ping;
                    continue;
                }

                const int diff = std::abs(endpoint->ping - endpoint->display_ping);


                // 90 is domain of diff
                float param = fmin((float)diff / 64.0f, 1.0f);


                const float min_delay = 1.0f;
                const float max_delay = 15.0f;


                if (ImGui::GetFrameCount() % (int)fmax(min_delay, max_delay - (max_delay * param * half_pi)) != 0)
                    continue;

                if (endpoint->display_ping < endpoint->ping)
                {
                    endpoint->display_ping ++;
                }
                else
                {
                    endpoint->display_ping --;
                }

                endpoint->display_ping = std::max(0, endpoint->display_ping);
            }
        }
    }

    //TODO transparency
    //if (ImGui::IsWindowHovered) pushstyle alpha .9f

    {
        /*ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;
        ImGui::SetNextWindowSize(ImVec2(418, 450), ImGuiCond_Once);
        //ImGui::SetNextWindowSize(ImVec2(418, 0));
        //ImGui::SetNextWindowPos(ImVec2(1920 - 400 - 90, 150), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(1920 - 400 - 90, 150), ImGuiCond_Once);
        // ImGui::Begin("dashboard", p_open, window_flags);

        ImGui::Begin("dashboard", &(this->window_open), window_flags);*/

        static auto &style = ImGui::GetStyle();
        ImU32 const white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, style.Alpha });
        //ImU32 const white = _UI32_MAX;

        ImVec2 const windowPos = ImGui::GetWindowPos();
        ImVec2 const windowSize = ImGui::GetWindowSize();

        ImDrawList* list = ImGui::GetWindowDrawList();
        ImDrawList* bg_list = ImGui::GetBackgroundDrawList();
        //ImDrawList* fg_list = ImGui::GetForegroundDrawList();
        float const scrollY = ImGui::GetScrollY();

        const auto color_button = ImColor::HSV(0, 0.4f, 1, style.Alpha);
        const auto color_button_hover = ImColor::HSV(0, 0.25f, 1, style.Alpha);

        static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
        const ImU32 color_text_secondary = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .8f * style.Alpha });

        // background texture
        {
            // TODO RED IF ERROR
            //const auto color = ImColor::HSV((ImGui::GetFrameCount() % 600) / 600.0f, .2, 1, style.Alpha);

            bg_list->AddImageRounded(_get_texture("image_background"), windowPos, windowPos + windowSize, ImVec2(0, 0), ImVec2(1, 1), white, 9);
        }

        ImGui::Spacing();

        // top part
        {
            auto const size = 24;
            const auto widgetPos = ImGui::GetCursorScreenPos();

            
            ImGui::BeginGroup();
            {
                // ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
                // version
                //list->AddText(NULL, 14, windowPos + widgetPos, (ImColor) ImVec4({ .8, .8, .8, 1 }), "v0.0");
                //ImGui::Dummy({ 0, 0 });

                // title
                /*ImGui::PushFont(font_title);
                ImGui::PushStyleColor(ImGuiCol_Text, this->title_color);
                ImGui::Text("DASHBOARD // title");
                ImGui::PopStyleColor();
                ImGui::PopFont();*/

                list->AddText(font_title, font_title->FontSize, widgetPos - ImVec2(1, 0), color_text, appStore.dashboard.title.c_str());
                ImGui::Dummy({ 0, font_title->FontSize - 6 });

                // subtitle
                ImGui::TextWrapped(appStore.dashboard.heading.c_str());

                // ImGui::PopStyleVar();
            }
            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Dummy({ ImGui::GetContentRegionAvail().x - size - 4 - size - 12, size });
            ImGui::SameLine(NULL, 0);

            const auto offset = ImVec2(0, 10);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);

            //auto const hovered = ImGui::IsMouseHoveringRect({ pos.x, pos.y }, { pos.x + size.x, pos.y + size.y });

            // socials
            {
                ImGui::Dummy({ size, size });
                list->AddImage((void*) _get_texture("icon_bolt"), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

                if (ImGui::IsItemClicked())
                    ImGui::OpenPopup("socials");
            }

            ImGui::SameLine(NULL, 10);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);

            // options
            {
                ImGui::Dummy({ size, size });
                list->AddImage((void*) _get_texture("icon_options"), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

                if (ImGui::IsItemClicked())
                    ImGui::OpenPopup("options");
            }
        }

        // ImGui::Spacing();

        for (auto const& [_p, process] : this->processes)
        {

            // TODO current window
            if (appStore._window_overlaying == "Overwatch")
                continue;

            ImGui::BeginGroup();
            {
                if (!process.on)
                    ImGui::BeginDisabled();

                {
                    ImGui::Dummy({ 184, 40 });

                    const auto hovered = ImGui::IsItemHovered();

                    const auto p_min = ImGui::GetItemRectMin();
                    const auto p_max = ImGui::GetItemRectMax();

                    static const auto frame = ImVec2(24, 24);

                    static const auto color_2 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.6f });
                    static const auto text_color_2 = ImGui::ColorConvertFloat4ToU32({ .4, .4, .4, style.Alpha });

                    list->AddRectFilled(p_min, p_max, hovered ? white : color_2, 9);
                    if (hovered)
                        list->AddRect(p_min, p_max, white, 14, NULL, 4);

                    const auto frame_pos = p_min + ImVec2(8, 8);
                    list->AddImage((void*)process.icon.texture, frame_pos, frame_pos + frame, ImVec2(0, 0), ImVec2(1, 1), !process.on ? color_2 : white);

                    const auto text_pos = p_min + ImVec2(frame.x + 16, 6);
                    list->AddText(text_pos, !process.on ? text_color_2 : color_text, "overwatch.exe");
                }

                if(!process.on)
                    ImGui::EndDisabled();
            }
            ImGui::EndGroup();

            if (ImGui::IsItemClicked() && process.window)
            {
                // focus window
                SetForegroundWindow(process.window);

                // maximize window
                PostMessage(process.window, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
        }        

        ImGui::Spacing();

        //ImGui::Dummy({ 0, 20 });

        {
            //ImGui::BeginChild("endpoints_scrollable", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 260), false);

            {
                int i = 0;
                for (auto& endpoint : this->endpoints)
                {
                    ImU32 const color = ImColor::HSV(fmod(i - 0.02f, 1.0f) / 14.0f, 0.4f, 1.0f, style.Alpha);
                    ImU32 const color_secondary = ImColor::HSV(fmod(i - 0.02f, 1.0f) / 14.0f, 0.3f, 1.0f, style.Alpha);
                    ImU32 const color_secondary_faded = ImColor::HSV(fmod(i - 0.02f, 1.0f) / 14.0f, 0.2f, 1.0f, 0.4f * style.Alpha);

                    auto const disabled = !(endpoint->ping > 0);
                    auto const &selected = endpoint->selected;

                    ImGui::Dummy({ 0 ,0 });
                    ImGui::SameLine(NULL, 16);
                    ImGui::Dummy({ ImGui::GetContentRegionAvail().x - 16, 73 });

                    auto hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

                    // background
                    if (hovered)
                    {
                        if (selected)
                        {
                            list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_secondary, 5, 0, 8);
                            list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_secondary, 5, NULL);
                        }
                        else
                        {
                            list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_secondary_faded, 5, NULL);
                        }
                    }
                    else
                    {
                        if (selected)
                        {
                            list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 5, NULL);
                        }
                    }

                    // icon
                    const auto icon_frame = ImVec2({ ImGui::GetItemRectSize().y, ImGui::GetItemRectSize().y });

                    static auto padding = ImVec2(21, 21);

                    const auto icon = selected ? _get_texture("icon_allow") : _get_texture("icon_block");

                    list->AddImage(icon, ImGui::GetItemRectMin() + padding, ImGui::GetItemRectMin() + icon_frame - padding, ImVec2(0, 0), ImVec2(1, 1), selected ? color_text_secondary : color /*color_secondary_faded*/);


                    // display 1
                    //auto pos = ImGui::GetItemRectMin() + style.FramePadding - ImVec2(0, 4);
                    auto pos = ImGui::GetItemRectMin() + ImVec2(icon_frame.x, 4);
                    list->AddText(font_title, 35, pos, selected ? white : color, endpoint->name.c_str());

                    // display 2
                    pos += ImVec2(2, ImGui::GetItemRectSize().y - 24 - 14);
                    list->AddText(font_subtitle, 24, pos, selected ? color_text_secondary : color_secondary, endpoint->hostname.c_str());

                    // popup
                    /*{
                        std::string key = std::format("endpoint{0}", i);
                        if (hovered || ImGui::IsPopupOpen(key.c_str()))
                            if (ImGui::BeginPopupContextItem(key.c_str()))
                            {
                                {
                                    ImGui::MenuItem(endpoint->name.c_str(), NULL, false, false);
                                    if (ImGui::MenuItem("disable")) {}
                                    if (ImGui::MenuItem("block", "wip")) {}

                                    //ImGui::SeparatorText("xx");
                                }

                                if (!hovered && !ImGui::IsWindowHovered())
                                    ImGui::CloseCurrentPopup();

                                ImGui::EndPopup();
                            }

                        ImGui::OpenPopupOnItemClick(key.c_str());
                    }*/

                    // action
                    if (ImGui::IsItemClicked())
                    {
                        int total = 0;
                        for (auto &endpoint : this->endpoints)
                            if (endpoint->selected)
                                total++;

                        if (total > 1 || !endpoint->selected)
                            endpoint->selected = !endpoint->selected;
                    }

                    // display 3 / (icon wifi)
                    {
                        auto icon = _get_texture("icon_wifi_slash");
                        static ImVec2 frame = ImVec2(26, 26);

                        if (0 < endpoint->ping)
                        {
                            if (endpoint->display_ping > 120)
                                icon = _get_texture("icon_wifi_poor");
                            else if (endpoint->display_ping > 60)
                                icon = _get_texture("icon_wifi_fair");
                            else
                                icon = _get_texture("icon_wifi");
                        }

                        auto pos = ImGui::GetItemRectMax() - ImVec2(frame.x, ImGui::GetItemRectSize().y) + (style.FramePadding * ImVec2(-1, 1)) + ImVec2(-4, 1);
                        list->AddImage(icon, pos, pos + frame, ImVec2(0, 0), ImVec2(1, 1), selected ? white : color);

                    }

                    // display 4
                    {
                        auto const text = std::to_string(endpoint->display_ping);

                        auto text_size = font_subtitle->CalcTextSizeA(24, FLT_MAX, 0.0f, text.c_str());
                        auto pos = ImGui::GetItemRectMax() - style.FramePadding - text_size + ImVec2(-4, 0);

                        list->AddText(font_subtitle, 24, pos, selected ? color_text_secondary : color_secondary, text.c_str());
                    }

                    i += 1;
                }
            }

            //ImGui::TextWrapped("Changes will be applied after closing the game");


            // bottom gap
            /*if (!this->show_all)
                ImGui::Dummy({ 0, ImGui::GetContentRegionAvail().y - 48 - 10 });*/

            ImGui::Spacing();

            // bottom part
            {
                ImGui::Dummy({ ImGui::GetContentRegionAvail().x, ImGui::GetFont()->FontSize + (style.FramePadding.y * 2) });
                list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), white, 5);
                list->AddText(ImGui::GetItemRectMin() + style.FramePadding + ImVec2(2, 0), color_text, "SHOW OTHERS");

                static ImVec2 frame = ImVec2(24, 24);
                auto const pos = ImVec2(ImGui::GetItemRectMax() - frame - ImVec2(style.FramePadding.x + 4, 14));
                auto const uv_min = !this->show_all ? ImVec2(0, 0) : ImVec2(0, 1);
                auto const uv_max = !this->show_all ? ImVec2(1, 1) : ImVec2(1, 0);
                list->AddImage(_get_texture("icon_angle"), pos, pos + frame, uv_min, uv_max, color_text);

                if (ImGui::IsItemClicked())
                {
                    this->show_all = !this->show_all;
                }

                if (this->show_all)
                {
                    //ImGui::BeginChild("#show_all");
                    ImGui::BeginGroup();
                    ImGui::Indent(style.FramePadding.x);
                    {
                        ImGui::Text("Unavailable in this version.");
                    }
                    ImGui::Unindent();
                    ImGui::EndGroup();
                    //list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), (ImU32)ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 }));
                    //ImGui::EndChild();
                    
                }
            }

            //ImGui::EndChild();
        }
        //ImGui::EndChild();

        //ImGui::Button("apply", { ImGui::GetContentRegionAvail().x, 32 });


        {
            if (ImGui::BeginPopupModal("options", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoResize))
            {
                ImDrawList* list = ImGui::GetWindowDrawList();

                ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);

                // handle close
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered() && !ImGui::IsWindowAppearing())
                    ImGui::CloseCurrentPopup();

                /*ImGui::SetWindowFontScale(1.4f);
                ImGui::MenuItem("options", NULL, false, false);
                ImGui::SetWindowFontScale(1.0f);*/

                // title
                ImGui::PushFont(font_title);
                ImGui::Text("OPTIONS");
                ImGui::PopFont();

                //list->AddRectFilled(ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax(), ImGui::ColorConvertFloat4ToU32({ 1, 0, 0, .4f }));
                //ImGui::Text(ImGui::IsMouseHoveringRect(ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax()) ? "hovering" : "not");

                // pinging
                /*if (ImGui::Checkbox("pinging", &(this->pinging)))
                {
                    if (this->pinging)
                        this->startPinging();
                    else
                        cv.notify_all();
                }*/

                ImGui::PushFont(font_subtitle);
                {
                    /*if (ImGui::MenuItem("PINGING", this->pinging ? "on" : "off")) {
                        this->pinging = !(this->pinging);

                        if (this->pinging)
                            this->startPinging();
                        else
                            cv.notify_all();
                    }
                    ImGui::SetItemTooltip("constantly query server status?");*/

                    // TODO wip
                    /*if (ImGui::MenuItem("THEME", this->theme == THEME::dark ? "dark" : "light", nullptr, true)) {
                        setTheme((this->theme == THEME::dark) ? THEME::light : THEME::dark);
                        this->theme = (this->theme == THEME::dark) ? THEME::light : THEME::dark;
                    }*/

                    // network settings
                    static auto frame = ImVec2(18, 18);
                    static auto offset = ImVec2(14, 8);
                    static auto offset2 = ImVec2(130, 0);
                    {
                        static auto text = "network settings";
                        //static auto offset2 = ImGui::CalcTextSize(text) * ImVec2(1, 0);
                        if (ImGui::MenuItem(text, ""))
                        {
                            system("start windowsdefender://network");
                        }
                        list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset + offset2, ImGui::GetItemRectMin() + offset + offset2 + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);
                        ImGui::SetItemTooltip("windowsdefender://network");
                    }

                    // firewall rules
                    {
                        static auto text = "firewall rules";
                        //static auto offset2 = ImGui::CalcTextSize(text) * ImVec2(1, 0);
                        if (ImGui::MenuItem("firewall rules", ""))
                        {
                            system("start wf.msc");
                        }
                        list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset + offset2, ImGui::GetItemRectMin() + offset + offset2 + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);
                        ImGui::SetItemTooltip("wf.msc");
                    }

                    if (ImGui::MenuItem("AUTO UPDATE", options.auto_update ? "on" : "off", nullptr, true))
                    {
                        options.auto_update = !options.auto_update;
                    }

                    // TODO wip
                    if (ImGui::MenuItem("SAVE FILE", "off", nullptr, false))
                    {
                    }
                    ImGui::SetItemTooltip("create a save file? you will\nhave the option to delete it\nif you uninstall.\n\ndropship needs this to keep\noptions");

                    /*if (ImGui::Button("ADVANCED"))
                    {
                        // open popup
                    }*/

                }
                ImGui::PopFont();

                ImGui::PopItemFlag();
                ImGui::EndPopup();
            }
        }

        {
            if (ImGui::BeginPopupModal("socials", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
            {
                ImDrawList* list = ImGui::GetWindowDrawList();

                ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);

                // handle close
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered() && !ImGui::IsWindowAppearing())
                    ImGui::CloseCurrentPopup();


                ImGui::PushFont(font_title);
                ImGui::Text("OPTIONS");
                ImGui::PopFont();

                ImGui::PushFont(font_subtitle);
                {
                    static auto frame = ImVec2(18, 18);
                    static auto offset = ImVec2(180, 8);

                    // discord
                    if (ImGui::MenuItem("discord", "suggestions\nhelp", nullptr, true)) {
                        system("start https://discord.stormy.gg");
                    }
                    list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                    ImGui::Spacing();

                    // twitch
                    if (ImGui::MenuItem("twitch", "stormyy_ow", nullptr, true)) {
                        system("start https://twitch.tv/stormyy_ow");
                    }
                    list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                    // twitter
                    if (ImGui::MenuItem("twitter", "stormyy_ow", nullptr, true)) {
                        system("start https://twitter.com/stormyy_ow");
                    }
                    list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                    // github
                    if (ImGui::MenuItem("github", "code", nullptr, true)) {
                        system("start https://github.com/stowmyy");
                    }
                    list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                }
                ImGui::PopFont();


                ImGui::PopItemFlag();
                ImGui::EndPopup();
            }
        }

        //ImGui::End();
    }
}
