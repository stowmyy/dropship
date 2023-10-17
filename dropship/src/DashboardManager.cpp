

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
    printf("hhhh\n");

    //::global_message = done ? "done cover_browser.jpg" : "D: failed.";
}

void DashboardManager::loadAssets() {

    loadPicture("icon_options", "png", &(icon_options.texture), &(icon_options.width), &(icon_options.height));
    loadPicture("icon_maple_leaf", "png", &(icon_maple_leaf.texture), &(icon_maple_leaf.width), &(icon_maple_leaf.height));
    loadPicture("icon_chain_slash", "png", &(icon_chain_slash.texture), &(icon_chain_slash.width), &(icon_chain_slash.height));
    loadPicture("icon_bolt", "png", &(icon_bolt.texture), &(icon_bolt.width), &(icon_bolt.height));
    loadPicture("icon_outside_window", "png", &(icon_outside_window.texture), &(icon_outside_window.width), &(icon_outside_window.height));

    loadPicture("icon_wifi_slash", "png", &(icon_wifi_slash.texture), &(icon_wifi_slash.width), &(icon_wifi_slash.height));
    loadPicture("icon_wifi_poor", "png", &(icon_wifi_poor.texture), &(icon_wifi_poor.width), &(icon_wifi_poor.height));
    loadPicture("icon_wifi_fair", "png", &(icon_wifi_fair.texture), &(icon_wifi_fair.width), &(icon_wifi_fair.height));
    loadPicture("icon_wifi", "png", &icon_wifi.texture, &icon_wifi.width, &icon_wifi.height);

    loadPicture("icon_arrow", "png", &(icon_arrow.texture), &(icon_arrow.width), &(icon_arrow.height));
    loadPicture("icon_angle", "png", &(icon_angle.texture), &(icon_angle.width), &(icon_angle.height));

    //loadPicture("2022_owl_flat_background", "png", &background_texture.texture, &background_texture.width, &background_texture.height);
    loadPicture("image_background", "png", &(background_texture.texture), &(background_texture.width), &(background_texture.height));
}



void DashboardManager::RenderInline(/* bool* p_open */)
{

    if (this->background_texture.texture == nullptr && ImGui::GetCurrentContext() != nullptr)
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

        ImVec2 const windowPos = ImGui::GetWindowPos();
        ImVec2 const windowSize = ImGui::GetWindowSize();

        ImDrawList* list = ImGui::GetWindowDrawList();
        ImDrawList* bg_list = ImGui::GetBackgroundDrawList();
        //ImDrawList* fg_list = ImGui::GetForegroundDrawList();
        float const scrollY = ImGui::GetScrollY();

        const auto color_button = ImColor::HSV(0, 0.4f, 1, style.Alpha);
        const auto color_button_hover = ImColor::HSV(0, 0.25f, 1, style.Alpha);

        // background texture
        {
            // TODO RED IF ERROR
            //const auto color = ImColor::HSV((ImGui::GetFrameCount() % 600) / 600.0f, .2, 1, style.Alpha);

            bg_list->AddImageRounded(this->background_texture.texture, windowPos, windowPos + windowSize, ImVec2(0, 0), ImVec2(1, 1), white, 9);
        }

        // top part
        {
            auto const size = 24;
            const auto widgetPos = ImGui::GetCursorPos();

            
            ImGui::BeginGroup();
            {
                // version
                //list->AddText(NULL, 14, windowPos + widgetPos, (ImColor) ImVec4({ .8, .8, .8, 1 }), "v0.0");
                //ImGui::Dummy({ 0, 0 });

                // title
                ImGui::PushFont(font_title);
                ImGui::PushStyleColor(ImGuiCol_Text, this->title_color);
                ImGui::Text("DROPSHIP");
                ImGui::PopStyleColor();
                ImGui::PopFont();

                // subtitle
                ImGui::Text("CHOOSE SERVERS");
            }
            ImGui::EndGroup();
            
            ImGui::SameLine();
            ImGui::Dummy({ ImGui::GetContentRegionAvail().x - size - 4 - size - 12, size });
            ImGui::SameLine(NULL, 0);

            const auto offset = ImVec2(0, 18);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);

            //auto const hovered = ImGui::IsMouseHoveringRect({ pos.x, pos.y }, { pos.x + size.x, pos.y + size.y });

            // socials
            {
                ImGui::Dummy({ size, size });
                list->AddImage((void*)icon_bolt.texture, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

                if (ImGui::IsItemClicked())
                    ImGui::OpenPopup("socials");
            }

            ImGui::SameLine(NULL, 12);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);

            // options
            {
                ImGui::Dummy({ size, size });
                list->AddImage((void*)icon_options.texture, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

                if (ImGui::IsItemClicked())
                    ImGui::OpenPopup("options");
            }
        }

        ImGui::Dummy({ 0, 20 });

        ImU32 const color_text = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, style.Alpha });
        ImU32 const color_text_secondary = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .8f * style.Alpha });

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

                    ImGui::Dummy({ 54, 54 });

                    static auto padding = ImVec2(16, 16);
                    static auto offset = ImVec2(-8, 8);

                    list->AddImage(this->icon_arrow.texture, ImGui::GetItemRectMin() + padding + offset, ImGui::GetItemRectMin() + ImVec2(this->icon_arrow.width, this->icon_arrow.height) - padding + offset, ImVec2(0, 0), ImVec2(1, 1), selected ? color_secondary : color_secondary_faded);

                    ImGui::SameLine();
                    ImGui::Dummy({ ImGui::GetContentRegionAvail().x, 73 });

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

                    // display 1
                    auto pos = ImGui::GetItemRectMin() + style.FramePadding - ImVec2(0, 4);
                    list->AddText(font_title, 35, pos, selected ? color_text : color, endpoint->name.c_str());

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
                        ImageTexture icon = this->icon_wifi_slash;
                        static ImVec2 frame = ImVec2(26, 26);

                        if (0 < endpoint->ping)
                        {
                            if (endpoint->display_ping > 120)
                                icon = this->icon_wifi_poor;
                            else if (endpoint->display_ping > 60)
                                icon = this->icon_wifi_fair;
                            else
                                icon = this->icon_wifi;
                        }

                        auto pos = ImGui::GetItemRectMax() - ImVec2(frame.x, ImGui::GetItemRectSize().y) + (style.FramePadding * ImVec2(-1, 1)) + ImVec2(-2, 1);
                        list->AddImage(icon.texture, pos, pos + frame, ImVec2(0, 0), ImVec2(1, 1), selected ? color_text : color);

                    }

                    // display 4
                    {
                        auto const text = std::to_string(endpoint->display_ping);

                        auto text_size = font_subtitle->CalcTextSizeA(24, FLT_MAX, 0.0f, text.c_str());
                        auto pos = ImGui::GetItemRectMax() - style.FramePadding - text_size;

                        list->AddText(font_subtitle, 24, pos, selected ? color_text_secondary : color_secondary, text.c_str());
                    }

                    i += 1;
                }
            }

            // bottom gap
            if (!this->show_all)
                ImGui::Dummy({ 0, ImGui::GetContentRegionAvail().y - 48 - 10 });

            // bottom part
            {
                ImGui::Dummy({ ImGui::GetContentRegionAvail().x, ImGui::GetFont()->FontSize + (style.FramePadding.y * 2) });
                list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_text, 5);
                list->AddText(ImGui::GetItemRectMin() + style.FramePadding + ImVec2(2, 0), (ImU32)ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]), "SHOW OTHERS");

                static ImVec2 frame = ImVec2(24, 24);
                auto const pos = ImVec2(ImGui::GetItemRectMax() - frame - ImVec2(style.FramePadding.x + 4, 14));
                auto const uv_min = !this->show_all ? ImVec2(0, 0) : ImVec2(0, 1);
                auto const uv_max = !this->show_all ? ImVec2(1, 1) : ImVec2(1, 0);
                list->AddImage(this->icon_angle.texture, pos, pos + frame, uv_min, uv_max, (ImU32)ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]));

                if (ImGui::IsItemClicked())
                {
                    this->show_all = !this->show_all;
                }

                if (this->show_all)
                {
                    ImGui::BeginChild("#show_all");
                    ImGui::BeginGroup();
                    ImGui::Indent(style.FramePadding.x);
                    {
                        ImGui::Text("Unavailable in this version.");
                    }
                    ImGui::Unindent();
                    ImGui::EndGroup();
                    //list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), (ImU32)ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 }));
                    ImGui::EndChild();
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
                        list->AddImage(this->icon_outside_window.texture, ImGui::GetItemRectMin() + offset + offset2, ImGui::GetItemRectMin() + offset + offset2 + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);
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
                        list->AddImage(this->icon_outside_window.texture, ImGui::GetItemRectMin() + offset + offset2, ImGui::GetItemRectMin() + offset + offset2 + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);
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

                    #ifdef _DEBUG
                    {
                        ImGui::SliderFloat("alpha", &(style.Alpha), 0.4f, 1.0f, "%.2f");
                    }
                    #endif

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
                    list->AddImage(this->icon_outside_window.texture, ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                    ImGui::Spacing();

                    // twitch
                    if (ImGui::MenuItem("twitch", "stormyy_ow", nullptr, true)) {
                        system("start https://twitch.tv/stormyy_ow");
                    }
                    list->AddImage(this->icon_outside_window.texture, ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                    // twitter
                    if (ImGui::MenuItem("twitter", "stormyy_ow", nullptr, true)) {
                        system("start https://twitter.com/stormyy_ow");
                    }
                    list->AddImage(this->icon_outside_window.texture, ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                    // github
                    if (ImGui::MenuItem("github", "code", nullptr, true)) {
                        system("start https://github.com/stowmyy");
                    }
                    list->AddImage(this->icon_outside_window.texture, ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                }
                ImGui::PopFont();


                ImGui::PopItemFlag();
                ImGui::EndPopup();
            }
        }

        //ImGui::End();
    }
}
