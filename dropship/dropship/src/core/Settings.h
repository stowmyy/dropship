#pragma once


#include "components/Endpoint.h"

#include "core/Firewall.h"
#include "util/watcher/window.h"

#include "images.h"

using json = nlohmann::json;
// web https://github.com/nlohmann/json


namespace dropship::settings {

    struct unique_server {
        const std::string block;
    };


    struct unique_endpoint {
        const std::string description;
        const std::string ip_ping { "" };
        std::set<std::string> blocked_servers;
        
    };

    /* note - always update to_json and from_json if new options are added */
    struct dropship_app_settings {
        struct _dropship_app_settings__options {
            bool auto_update { false };
            bool ping_servers { true };
            bool tunneling{ true };
        };
        _dropship_app_settings__options options;

        struct _dropship_app_settings__config {
            std::set<std::string> blocked_endpoints;
            std::optional<std::filesystem::path> tunneling_path;
        };
        _dropship_app_settings__config config;
    };

    void to_json(json& j, const dropship_app_settings& p);

    void from_json(const json& j, dropship_app_settings& p);

    /* custom so i only store changes in firewall */
    json strip_diff_dropship_app_settings(const json& j_default, const json& j);
};


/* how to get gpc codes
    - https://www.gstatic.com/ipranges/cloud.json
    - ex. var find = (n) => data.prefixes.filter(x => x.hasOwnProperty("ipv4Prefix") && x.scope == n).map(x => x.ipv4Prefix).join(); find("europe-north1")
*/

static const std::string GPC_ASIA_SOUTHEAST1 { "34.1.128.0/20,34.1.192.0/20,34.2.16.0/20,34.2.128.0/17,34.21.128.0/17,34.87.0.0/17,34.87.128.0/18,34.104.58.0/23,34.104.106.0/23,34.124.42.0/23,34.124.128.0/17,34.126.64.0/18,34.126.128.0/18,34.128.44.0/23,34.128.60.0/23,34.142.128.0/17,34.143.128.0/17,34.152.104.0/23,34.153.40.0/23,34.153.232.0/23,34.157.82.0/23,34.157.88.0/23,34.157.210.0/23,34.177.72.0/23,35.185.176.0/20,35.186.144.0/20,35.187.224.0/19,35.197.128.0/19,35.198.192.0/18,35.213.128.0/18,35.220.24.0/23,35.234.192.0/20,35.240.128.0/17,35.242.24.0/23,35.247.128.0/18" };
static const std::string GPC_EUROPE_NORTH1 { "34.88.0.0/16,34.104.96.0/21,34.124.32.0/21,35.203.232.0/21,35.217.0.0/18,35.220.26.0/24,35.228.0.0/16,35.242.26.0/24" };
static const std::string GPC_SOUTHAMERICA_EAST1 { "34.39.128.0/17,34.95.128.0/17,34.104.80.0/21,34.124.16.0/21,34.151.0.0/18,34.151.192.0/18,35.198.0.0/18,35.199.64.0/18,35.215.192.0/18,35.220.40.0/24,35.235.0.0/20,35.242.40.0/24,35.247.192.0/18" };
static const std::string GPC_ASIA_NORTHEAST1 { "34.84.0.0/16,34.85.0.0/17,34.104.62.0/23,34.104.128.0/17,34.127.190.0/23,34.146.0.0/16,34.157.64.0/20,34.157.164.0/22,34.157.192.0/20,35.187.192.0/19,35.189.128.0/19,35.190.224.0/20,35.194.96.0/19,35.200.0.0/17,35.213.0.0/17,35.220.56.0/22,35.221.64.0/18,35.230.240.0/20,35.242.56.0/22,35.243.64.0/18,104.198.80.0/20,104.198.112.0/20" };
static const std::string GPC_ME_CENTRAL2 { "34.1.48.0/20,34.152.84.0/23,34.152.102.0/24,34.166.0.0/16,34.177.48.0/23,34.177.70.0/24" };


class Settings
{
    /* consts */
    private:

        const std::map<std::string, dropship::settings::unique_server> __ow2_servers {

            /* notes
                - irvine server is 24.105.0.0/18
                - netlimiter -> whois for cidr
                - filter 24.105. in netlimiter
                - go to https://ipinfo.io/ips/24.105.0.0/16 and scroll down and see the ones owned by blizzard
                - https://github.com/femueller/cloud-ip-ranges/blob/master/google-cloud-ip-ranges.json
                - https://www.gstatic.com/ipranges/cloud.json

                - when hanging, try unblocking all to wait for all pings to die and then find irvine servers with high port number. block those.
                  + this seems to list all ips! they are not saved to connection history though

                - 85.236.96.0 - 85.236.103.255 blocks voice chat. you get a message about it.

                - TROUBLESHOOT when i launch the game quickly and go to practice range, it tends to try syd2 first even if blocked.
                  + maybe missing an ip? can use old ones if needed

                - note going to custom game avoids that problem
            */

            /* auth
                - blocking a subset of 24.105.0.0/18 prevents login (americas)
                - blocking 103.4.114.0/23 in asia prevents login (asia)

                - americas uses irvine for auth?
                - asia uses gpc korea for auth?

            */

            // { "test", { .block = "" }}, // PEERINGDB

            /* ord1
                - the connection hangs when the irvine server ips are not blocked
                - ips are the following
                  + gameserver ip cidrs found through netlimiter
                  + irvine region (this may need to be tweaked)
            */
            { "blizzard/ord1", { .block = "24.105.40.0/21,137.224.0.0/16,137.222.0.0/15,137.221.0.0/16,64.224.0.0/21" }},
            
            /* las1
                - previous version also had some irvine servers. they do not appear to be necessary?
            */
            { "blizzard/las1", { .block = "64.224.24.0/21" } },

            /* gen1
                - (if needed) a more agressive gpc finland block: 34.88.0.0/16
            */
            { "google/europe-north1", { .block = GPC_EUROPE_NORTH1 } },

            /* gsg1
                - 
            */
            { "google/asia-southeast1", { .block = GPC_ASIA_SOUTHEAST1 } },

            /* gbr1
                -
            */
            { "google/southamerica-east1", { .block = GPC_SOUTHAMERICA_EAST1 } },

            /* gtk1
                -
            */
            { "google/asia-northeast1", { .block = GPC_ASIA_NORTHEAST1 } },

            /* gmec2
                -
            */
            { "google/me-central2", { .block = GPC_ME_CENTRAL2 } },

            /* icn1
                - 117.52.0.0/16 is irvine (lg dacom)
                - following are more AOE lg dacom blocks i connected to during testing
                - TROUBLESHOOTING: do i need to block all lg dacom cidrs? https://networksdb.io/ip-addresses-of/lg-dacom-corp
                - 202.9 is game server
            */
            { "blizzard/icn1", { .block = "117.52.0.0/16,121.254.0.0/16,202.9.66.0-202.9.67.255" } },

            /* syd2
                - TODO block syd irvine testing
                - ex. https://ipinfo.io/158.115.196.194
                - got it from unblocking all and watching irvine connections. they are not saved to history.
                - do that for all ips?
            */
            { "blizzard/syd2", { .block = "158.115.196.0/23" } },

            /* tpe1
                -
            */
            { "blizzard/tpe1", { .block = "5.42.160.0/24" } },

            /* ams1
                - TROUBLESHOOT: is this one still active? https://ipinfo.io/5.42.168.0
            */
            { "blizzard/ams1", { .block = "64.224.24.0/21,137.221.0.0/16,137.222.0.0/15,137.224.0.0/16" } },

            // note: netherlands appears to be down despite being available in-game

        };

        const std::map<std::string, dropship::settings::unique_endpoint> __ow2_endpoints {
            /*{" .test", {
                .description = "test",
                .ip_ping = "",
                .blocked_servers = { "test"},
            }},*/
            {"USA - Central", {
                .description = "ORD1",
                .ip_ping = "8.34.210.23",
                .blocked_servers = { "blizzard/ord1"},
            }},
            {"USA - West", {
                .description = "LAS1",
                .ip_ping = "34.16.128.42",
                .blocked_servers = { "blizzard/las1"},
            }},
            {"Finland", {
                .description = "GEN1",
                .ip_ping = "34.88.0.1",
                .blocked_servers = { "google/europe-north1"},
            }},
            {"Singapore", {
                .description = "GSG1",
                .ip_ping = "34.1.128.4",
                .blocked_servers = { "google/asia-southeast1"},
            }},
            {"Brazil", {
                .description = "GBR1",
                .ip_ping = "34.39.128.0",
                .blocked_servers = { "google/southamerica-east1"},
            }},
            {"Tokyo", {
                .description = "GTK1",
                .ip_ping = "34.84.0.0",
                .blocked_servers = { "google/asia-northeast1"},
            }},
            {"Saudi Arabia", {
                .description = "GMEC2",
                .ip_ping = "34.166.0.84",
                .blocked_servers = { "google/me-central2"},
            }},
            {"South Korea", {
                .description = "ICN1",
                .ip_ping = "34.22.64.10",
                .blocked_servers = { "blizzard/icn1"},
            }},
            {"Australia", {
                .description = "SYD2",
                .ip_ping = "34.40.128.34",
                .blocked_servers = { "blizzard/syd2"},
            }},
            {"Taiwan", {
                .description = "TPE1",
                .ip_ping = "34.80.0.0",
                .blocked_servers = { "blizzard/tpe1"},
            }},
            {"Netherlands", {
                .description = "AMS1",
                .ip_ping = "137.221.78.60",
                .blocked_servers = { "blizzard/ams1"},
            }},
        };

        const dropship::settings::dropship_app_settings __default_dropship_app_settings;

        dropship::settings::dropship_app_settings _dropship_app_settings;

    public:

        Settings();
        ~Settings();

        const dropship::settings::dropship_app_settings& getAppSettings();

        /* will not do anything on error */
        void tryLoadSettingsFromStorage();
    
        /* will not do anything on error */
        void tryWriteSettingsToStorage(bool force = false);

        void render();

        void renderWaitingStatus();


    public:

        void unblockAll();

        void toggleOptionAutoUpdate();
        void toggleOptionPingServers();
        void toggleOptionTunneling();

        void addBlockedEndpoint(std::string endpoint_title);
        void removeBlockedEndpoint(std::string endpoint_title);

        void setConfigTunnelingPath(std::optional<std::filesystem::path> path);

        //void syncEndpoint(std::shared_ptr<Endpoint2> endpoint);

    private:

        [[nodiscard]] std::optional<json> readStoragePatch__win_firewall();
        [[nodiscard]] std::optional<json> readStoragePatch();

        [[nodiscard]] std::string getAllBlockedAddresses();

        bool _waiting_for_config_write { false };
};
    