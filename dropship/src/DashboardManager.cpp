

// for std::min
#define NOMINMAX

// needs to be included before browser
#include "_windows_ping.hpp"
#include "_windows_domain.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS // https://github.com/ocornut/imgui/issues/2832
#include "DashboardManager.h"

std::condition_variable cv; // for stopping pinging early
std::mutex cv_m; // for stopping pinging early

extern ImFont* font_title;
extern ImFont* font_subtitle;
extern ImFont* font_text;

extern OPTIONS options;
extern AppStore appStore;
extern AppStore __default__appStore;

extern FirewallManager firewallManager;

// this value is combined with the new ping.
static const unsigned char ping_offset = 10;

void DashboardManager::startPinging(int interval = 9000)
{
    std::thread([&, interval]()
    {
        std::cout << "(ping) started pinging..\n" << std::endl;
 
        while (this->pinging)
        {

            // send an imcp ping for each endpoint
            for (auto& endpoint : this->endpoints)
            {
                if (endpoint._ping_ip.empty())
                    continue;

                std::thread([&endpoint]()
                {
                    int ping;

                    if (_windows_ping(endpoint._ping_ip, &ping, 4000) && !(endpoint._has_pinged_successfully))
                        endpoint._has_pinged_successfully = true;

                    endpoint._has_pinged = true;

                    if (ping > 0)
                        endpoint.ping = ping + ping_offset;
                }).detach();
            }
            
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

        std::cout << "(ping) stopped pinging..\n" << std::endl;
    }).detach();
}

DashboardManager::~DashboardManager()
{
    this->pinging = false;
}


/*
    current way i'm getting ips:
        > tracert any edge ip
        > get AS ip, 2nd from last?
*/
DashboardManager::DashboardManager() : 
    
    ips({


        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_NA_central.txt
        { "na/central", "24.105.40.0-24.105.47.255,64.224.0.0/21,8.34.210.0/24,8.34.212.0/22,8.34.216.0/22,8.35.192.0/21,23.236.48.0/20,23.251.144.0/20,34.0.225.0/24,34.16.0.0/17,34.27.0.0/16,34.28.0.0/14,34.33.0.0/16,34.41.0.0/16,34.42.0.0/16,34.44.0.0/15,34.46.0.0/16,34.66.0.0/15,34.68.0.0/14,34.72.0.0/16,34.118.200.0/21,34.121.0.0/16,34.122.0.0/15,34.128.32.0/22,34.132.0.0/14,34.136.0.0/16,34.153.48.0/21,34.153.240.0/21,34.157.84.0/23,34.157.96.0/20,34.157.212.0/23,34.157.224.0/20,34.170.0.0/15,34.172.0.0/15,34.177.52.0/22,35.184.0.0/16,35.188.0.0/17,35.188.128.0/18,35.188.192.0/19,35.192.0.0/15,35.194.0.0/18,35.202.0.0/16,35.206.64.0/18,35.208.0.0/15,35.220.64.0/19,35.222.0.0/15,35.224.0.0/15,35.226.0.0/16,35.232.0.0/16,35.238.0.0/15,35.242.96.0/19,104.154.16.0/20,104.154.32.0/19,104.154.64.0/19,104.154.96.0/20,104.154.113.0/24,104.154.114.0/23,104.154.116.0/22,104.154.120.0/23,104.154.128.0/17,104.155.128.0/18,104.197.0.0/16,104.198.16.0/20,104.198.32.0/19,104.198.64.0/20,104.198.128.0/17,107.178.208.0/20,108.59.80.0/21,130.211.112.0/20,130.211.128.0/18,130.211.192.0/19,130.211.224.0/20,146.148.32.0/19,146.148.64.0/19,146.148.96.0/20,162.222.176.0/21,173.255.112.0/21,199.192.115.0/24,199.223.232.0/22,199.223.236.0/24,34.22.0.0/19,35.186.0.0/17,35.186.128.0/20,35.206.32.0/19,35.220.46.0/24,35.242.46.0/24,107.167.160.0/20,108.59.88.0/21,173.255.120.0/21" },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_NA_West.txt
        { "na/west", "64.224.24.0/23,24.105.8.0-24.105.15.255,35.247.0.0/17,35.236.0.0/17,35.235.64.0/18,34.102.0.0/17,34.94.0.0/16,34.19.0.0/17,34.82.0.0/15,34.105.0.0/17,34.118.192.0/21,34.127.0.0/17,34.145.0.0/17,34.157.112.0/21,34.157.240.0/21,34.168.0.0/15,35.185.192.0/18,35.197.0.0/17,35.199.144.0/20,35.199.160.0/19,35.203.128.0/18,35.212.128.0/17,35.220.48.0/21,35.227.128.0/18,35.230.0.0/17,35.233.128.0/17,35.242.48.0/21,35.243.32.0/21,35.247.0.0/17,104.196.224.0/19,104.198.0.0/20,104.198.96.0/20,104.199.112.0/20,34.20.128.0/17,34.94.0.0/16,34.102.0.0/17,34.104.64.0/21,34.108.0.0/16,34.118.248.0/23,35.215.64.0/18,35.220.47.0/24,35.235.64.0/18,35.236.0.0/17,35.242.47.0/24,35.243.0.0/21,34.22.32.0/19,34.104.52.0/24,34.106.0.0/16,34.127.180.0/24,35.217.64.0/18,35.220.31.0/24,35.242.31.0/24,34.16.128.0/17,34.104.72.0/22,34.118.240.0/22,34.124.8.0/22,34.125.0.0/16,35.219.128.0/18,34.124.0.0/21,34.37.0.0/16,34.128.46.0/23,34.128.62.0/23,34.53.0.0/17" },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_NA_East.txt
        { "na/east", "34.124.0.0/21,35.236.192.0-35.236.255.255,35.199.0.0-35.199.63.255,34.86.0.0-34.86.255.255,35.245.0.0-35.245.255.255,35.186.160.0-35.186.191.255,34.145.128.0-34.145.255.255,34.150.128.0-34.150.255.255,34.85.128.0-34.85.255.255,34.23.0.0/16,34.24.0.0/15,34.26.0.0/16,34.73.0.0/16,34.74.0.0/15,34.98.128.0/21,34.118.250.0/23,34.138.0.0/15,34.148.0.0/16,35.185.0.0/17,35.190.128.0/18,35.196.0.0/16,35.207.0.0/18,35.211.0.0/16,35.220.0.0/20,35.227.0.0/17,35.229.16.0/20,35.229.32.0/19,35.229.64.0/18,35.231.0.0/16,35.237.0.0/16,35.242.0.0/20,35.243.128.0/17,104.196.0.0/18,104.196.65.0/24,104.196.66.0/23,104.196.68.0/22,104.196.96.0/19,104.196.128.0/18,104.196.192.0/19,162.216.148.0/22,34.21.0.0/17,34.85.128.0/17,34.86.0.0/16,34.104.60.0/23,34.104.124.0/23,34.118.252.0/23,34.124.60.0/23,34.127.188.0/23,34.145.128.0/17,34.150.128.0/17,34.157.0.0/21,34.157.16.0/20,34.157.128.0/21,34.157.144.0/20,35.186.160.0/19,35.188.224.0/19,35.194.64.0/19,35.199.0.0/18,35.212.0.0/17,35.220.60.0/22,35.221.0.0/18,35.230.160.0/19,35.234.176.0/20,35.236.192.0/18,35.242.60.0/22,35.243.40.0/21,35.245.0.0/16,34.157.32.0/22,34.157.160.0/22,34.162.0.0/16,34.104.56.0/23,34.127.184.0/23,34.161.0.0/16,35.206.10.0/23,34.152.72.0/21,34.177.40.0/21,34.48.0.0/16,34.1.16.0/20" },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_EU.txt
        { "eu", "64.224.26.0/23,104.155.0.0/17,104.199.0.0/18,104.199.66.0/23,104.199.68.0/22,104.199.72.0/21,104.199.80.0/20,104.199.96.0/20,130.211.48.0/20,130.211.64.0/19,130.211.96.0/20,146.148.112.0/20,146.148.16.0/20,146.148.2.0/23,146.148.4.0/22,146.148.8.0/21,192.158.28.0/22,23.251.128.0/20,34.104.110.0/23,34.104.112.0/23,34.104.126.0/23,34.104.96.0/21,34.105.128.0/17,34.107.0.0/17,34.118.244.0/22,34.118.254.0/23,34.124.32.0/21,34.124.46.0/23,34.124.48.0/23,34.124.62.0/23,34.127.186.0/23,34.140.0.0/16,34.141.0.0/17,34.141.128.0/17,34.142.0.0/17,34.147.0.0/17,34.147.128.0/17,34.154.0.0/16,34.155.0.0/16,34.157.12.0/22,34.157.136.0/23,34.157.140.0/22,34.157.168.0/22,34.157.176.0/20,34.157.208.0/23,34.157.220.0/22,34.157.36.0/22,34.157.40.0/22,34.157.48.0/20,34.157.8.0/23,34.157.80.0/23,34.157.92.0/22,34.159.0.0/16,34.163.0.0/16,34.65.0.0/16,34.76.0.0/14,34.88.0.0/16,34.89.0.0/17,34.89.128.0/17,34.90.0.0/15,35.187.0.0/17,35.187.160.0/19,35.189.192.0/18,35.189.64.0/18,35.190.192.0/19,35.195.0.0/16,35.197.192.0/18,35.198.128.0/18,35.198.64.0/18,35.203.210.0/23,35.203.212.0/22,35.203.216.0/22,35.203.232.0/21,35.204.0.0/16,35.205.0.0/16,35.206.128.0/18,35.207.128.0/18,35.207.64.0/18,35.210.0.0/16,35.214.0.0/17,35.214.128.0/17,35.216.128.0/17,35.217.0.0/18,35.219.224.0/19,35.220.16.0/23,35.220.18.0/23,35.220.20.0/22,35.220.26.0/24,35.220.44.0/24,35.220.96.0/19,35.228.0.0/16,35.230.128.0/19,35.233.0.0/17,35.234.128.0/19,35.234.160.0/20,35.234.64.0/18,35.235.216.0/21,35.235.32.0/20,35.235.48.0/20,35.240.0.0/17,35.241.128.0/17,35.242.128.0/18,35.242.16.0/23,35.242.18.0/23,35.242.192.0/18,35.242.20.0/22,35.242.26.0/24,35.242.44.0/24,35.242.64.0/19,35.246.0.0/17,35.246.128.0/17,5.42.168.0-5.42.175.255,5.42.184.0-5.42.191.255,8.34.208.0/23,8.34.211.0/24,8.34.220.0/22,34.22.128.0/17,34.104.116.0/22,34.116.128.0/17,34.118.0.0/17,34.124.52.0/22,34.157.44.0/23,34.157.172.0/23,34.164.0.0/16,34.175.0.0/16,34.22.112.0/20,34.17.0.0/16,34.157.124.0/23,34.157.250.0/23,34.0.160.0/19,34.157.121.0/24,34.157.249.0/24,34.0.192.0/19,34.0.240.0/20,34.34.128.0/18,34.38.0.0/16,34.32.0.0/17,34.152.80.0/23,34.177.36.0/23,34.39.0.0/17,34.128.52.0/22,34.0.224.0/24,34.0.226.0/24,34.40.0.0/17,34.32.128.0/17,34.34.0.0/17,34.1.0.0/20,34.1.160.0/20,34.1.224.0/19,34.153.38.0/24,34.153.230.0/24"  },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_AS_Japan.txt
        { "japan", "34.85.0.0-34.85.127.255,34.84.0.0-34.84.255.255,35.190.224.0-35.190.239.255,35.194.96.0-35.194.255.255,35.221.64.0-35.221.255.255,34.146.0.0-34.146.255.255,34.84.0.0/16,34.85.0.0/17,34.104.62.0/23,34.104.128.0/17,34.127.190.0/23,34.146.0.0/16,34.157.64.0/20,34.157.164.0/22,34.157.192.0/20,35.187.192.0/19,35.189.128.0/19,35.190.224.0/20,35.194.96.0/19,35.200.0.0/17,35.213.0.0/17,35.220.56.0/22,35.221.64.0/18,35.230.240.0/20,35.242.56.0/22,35.243.64.0/18,104.198.80.0/20,104.198.112.0/20,34.97.0.0/16,34.104.49.0/24,34.127.177.0/24,35.217.128.0/17,35.220.45.0/24,35.242.45.0/24,35.243.56.0/21"  },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_Australia.txt
        { "australia", "158.115.196.0/23,37.244.42.0-37.244.42.255,34.87.192.0/18,34.104.104.0/23,34.116.64.0/18,34.124.40.0/23,34.151.64.0/18,34.151.128.0/18,35.189.0.0/18,35.197.160.0/19,35.201.0.0/19,35.213.192.0/18,35.220.41.0/24,35.234.224.0/20,35.242.41.0/24,35.244.64.0/18,34.104.122.0/23,34.124.58.0/23,34.126.192.0/20,34.129.0.0/16,34.0.16.0/20,34.40.128.0/17,34.128.36.0/24,34.128.48.0/24,34.1.176.0/20" },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_AS_Korea.txt
        { "south_korea", "202.9.66.0/23,34.64.0.0-34.64.255.255,117.52.0.0-117.52.255.255,121.254.0.0-121.254.255.255,34.0.96.0/19,34.64.32.0/19,34.64.64.0/22,34.64.68.0/22,34.64.72.0/21,34.64.80.0/20,34.64.96.0/19,34.64.128.0/22,34.64.132.0/22,34.64.136.0/21,34.64.144.0/20,34.64.160.0/19,34.64.192.0/18,35.216.0.0/17,34.22.64.0/19,34.22.96.0/20,34.47.64.0/18,34.50.0.0/18" },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_AS_Singapore.txt
        { "singapore", "34.124.0.0-34.124.255.255,34.124.42.0-34.124.43.255,34.142.128.0-34.142.255.255,35.185.176.0-35.185.191.255,35.186.144.0-35.186.159.255,35.247.128.0-35.247.191.255,34.87.0.0-34.87.191.255,34.143.128.0-34.143.255.255,34.124.128.0-34.124.255.255,34.126.64.0-34.126.191.255,35.240.128.0-35.240.255.255,35.198.192.0-35.198.255.255,34.21.128.0-34.21.255.255,34.104.58.0-34.104.59.255,34.124.41.0-34.124.42.255,34.157.82.0-34.157.83.255,34.157.88.0-34.157.89.255,34.157.210.0-34.157.211.255,35.187.224.0-35.187.255.255,35.197.128.0-35.197.159.255,35.213.128.0-35.213.191.255,35.220.24.0-35.220.25.255,35.234.192.0-35.234.207.255,35.242.24.0-35.242.25.255,34.126.128.0/18,34.87.128.0/18,34.21.128.0/17,34.87.0.0/17,34.87.128.0/18,34.104.58.0/23,34.104.106.0/23,34.124.42.0/23,34.124.128.0/17,34.126.64.0/18,34.126.128.0/18,34.142.128.0/17,34.143.128.0/17,34.157.82.0/23,34.157.88.0/23,34.157.210.0/23,35.185.176.0/20,35.186.144.0/20,35.187.224.0/19,35.197.128.0/19,35.198.192.0/18,35.213.128.0/18,35.220.24.0/23,35.234.192.0/20,35.240.128.0/17,35.242.24.0/23,35.247.128.0/18,34.101.18.0/24,34.101.20.0/22,34.101.24.0/22,34.101.32.0/19,34.101.64.0/18,34.101.128.0/17,34.128.64.0/18,35.219.0.0/17,34.128.44.0/23,34.128.60.0/23,34.1.128.0/20,34.1.192.0/20,34.153.40.0/23,34.153.232.0/23" },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_AS_Taiwan.txt
        { "taiwan", "5.42.160.0-5.42.160.255,35.221.128.0/17,34.80.0.0/15,34.137.0.0/16,35.185.128.0/19,35.185.160.0/20,35.187.144.0/20,35.189.160.0/19,35.194.128.0/17,35.201.128.0/17,35.206.192.0/18,35.220.32.0/21,35.229.128.0/17,35.234.0.0/18,35.235.16.0/20,35.236.128.0/18,35.242.32.0/21,104.155.192.0/19,104.155.224.0/20,104.199.128.0/18,104.199.192.0/19,104.199.224.0/20,104.199.242.0/23,104.199.244.0/22,104.199.248.0/21,107.167.176.0/20,130.211.240.0/20"  },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_Brazil.txt
        { "brazil", "34.95.128.0/17,34.104.80.0/21,34.124.16.0/21,34.151.0.0/18,34.151.192.0/18,35.198.0.0/18,35.199.64.0/18,35.215.192.0/18,35.220.40.0/24,35.235.0.0/20,35.242.40.0/24,35.247.192.0/18,34.104.50.0/23,34.127.178.0/23,34.176.0.0/16,34.95.208.0/20,34.39.128.0/17,35.199.96.0/20"  },

        // https://github.com/foryVERX/Overwatch-Server-Selector/blob/main/ip_lists/Ip_ranges_ME.txt
        { "middle_east", "157.175.0.0-157.175.255.255,15.185.0.0-15.185.255.255,15.184.0.0-15.184.255.255,16.24.0.0/16,34.1.48.0/20,34.152.84.0/23,34.166.0.0/16,34.177.48.0/23,34.1.32.0/20,34.18.0.0/16,34.157.126.0/23,34.157.252.0/23,34.0.64.0/19,34.157.90.0/23,34.157.216.0/23,34.165.0.0/16,34.1.48.0/20,34.177.48.0/23,34.0.64.0/19,34.157.90.0/23,34.157.216.0/23,34.165.0.0/16"  },

        // 
        // { "",  },


        //TODO: custom combos. not as reliable apparently
        //{ "ord1", "24.105.40.0-24.105.47.255,64.224.0.0/21,8.34.210.0/24,8.34.212.0/22,8.34.216.0/22,8.35.192.0/21,23.236.48.0/20,23.251.144.0/20,34.0.225.0/24,34.16.0.0/17,34.27.0.0/16,34.28.0.0/14,34.33.0.0/16,34.41.0.0/16,34.42.0.0/16,34.44.0.0/15,34.46.0.0/16,34.66.0.0/15,34.68.0.0/14,34.72.0.0/16,34.118.200.0/21,34.121.0.0/16,34.122.0.0/15,34.128.32.0/22,34.132.0.0/14,34.136.0.0/16,34.153.48.0/21,34.153.240.0/21,34.157.84.0/23,34.157.96.0/20,34.157.212.0/23,34.157.224.0/20,34.170.0.0/15,34.172.0.0/15,34.177.52.0/22,35.184.0.0/16,35.188.0.0/17,35.188.128.0/18,35.188.192.0/19,35.192.0.0/15,35.194.0.0/18,35.202.0.0/16,35.206.64.0/18,35.208.0.0/15,35.220.64.0/19,35.222.0.0/15,35.224.0.0/15,35.226.0.0/16,35.232.0.0/16,35.238.0.0/15,35.242.96.0/19,104.154.16.0/20,104.154.32.0/19,104.154.64.0/19,104.154.96.0/20,104.154.113.0/24,104.154.114.0/23,104.154.116.0/22,104.154.120.0/23,104.154.128.0/17,104.155.128.0/18,104.197.0.0/16,104.198.16.0/20,104.198.32.0/19,104.198.64.0/20,104.198.128.0/17,107.178.208.0/20,108.59.80.0/21,130.211.112.0/20,130.211.128.0/18,130.211.192.0/19,130.211.224.0/20,146.148.32.0/19,146.148.64.0/19,146.148.96.0/20,162.222.176.0/21,173.255.112.0/21,199.192.115.0/24,199.223.232.0/22,199.223.236.0/24,34.22.0.0/19,35.186.0.0/17,35.186.128.0/20,35.206.32.0/19,35.220.46.0/24,35.242.46.0/24,107.167.160.0/20,108.59.88.0/21,173.255.120.0/21" },
        //{ "guw2", "35.247.0.0/17,35.236.0.0/17,35.235.64.0/18,34.102.0.0/17,34.94.0.0/16,34.19.0.0/17,34.82.0.0/15,34.105.0.0/17,34.118.192.0/21,34.127.0.0/17,34.145.0.0/17,34.157.112.0/21,34.157.240.0/21,34.168.0.0/15,35.185.192.0/18,35.197.0.0/17,35.199.144.0/20,35.199.160.0/19,35.203.128.0/18,35.212.128.0/17,35.220.48.0/21,35.227.128.0/18,35.230.0.0/17,35.233.128.0/17,35.242.48.0/21,35.243.32.0/21,35.247.0.0/17,104.196.224.0/19,104.198.0.0/20,104.198.96.0/20,104.199.112.0/20,34.20.128.0/17,34.94.0.0/16,34.102.0.0/17,34.104.64.0/21,34.108.0.0/16,34.118.248.0/23,35.215.64.0/18,35.220.47.0/24,35.235.64.0/18,35.236.0.0/17,35.242.47.0/24,35.243.0.0/21,34.22.32.0/19,34.104.52.0/24,34.106.0.0/16,34.127.180.0/24,35.217.64.0/18,35.220.31.0/24,35.242.31.0/24,34.16.128.0/17,34.104.72.0/22,34.118.240.0/22,34.124.8.0/22,34.125.0.0/16,35.219.128.0/18,34.124.0.0/21" },
        //{ "lax1", "24.105.8.0-24.105.15.255,34.124.0.0/21" },
        //{ "las1", "64.224.24.0/23" },
        //{ "ams1", "5.42.168.0-5.42.175.255,64.224.26.0/23" },
        //{ "cdg1", "5.42.184.0-5.42.191.255" },
        //{ "gen1", "34.88.0.0/16,34.104.96.0/21,34.124.32.0/21,35.203.232.0/21,35.217.0.0/18,35.220.26.0/24,35.228.0.0/16,35.242.26.0/24" },
        //{ "gbr1", "34.95.128.0/17,34.104.80.0/21,34.124.16.0/21,34.151.0.0/18,34.151.192.0/18,35.198.0.0/18,35.199.64.0/18,35.215.192.0/18,35.220.40.0/24,35.235.0.0/20,35.242.40.0/24,35.247.192.0/18,34.104.50.0/23,34.127.178.0/23,34.176.0.0/16" },
        //{ "gtk1", "34.85.0.0-34.85.127.255,34.84.0.0-34.84.255.255,35.190.224.0-35.190.239.255,35.194.96.0-35.194.255.255,35.221.64.0-35.221.255.255,34.146.0.0-34.146.255.255,34.84.0.0/16,34.85.0.0/17,34.104.62.0/23,34.104.128.0/17,34.127.190.0/23,34.146.0.0/16,34.157.64.0/20,34.157.164.0/22,34.157.192.0/20,35.187.192.0/19,35.189.128.0/19,35.190.224.0/20,35.194.96.0/19,35.200.0.0/17,35.213.0.0/17,35.220.56.0/22,35.221.64.0/18,35.230.240.0/20,35.242.56.0/22,35.243.64.0/18,104.198.80.0/20,104.198.112.0/20,34.97.0.0/16,34.104.49.0/24,34.127.177.0/24,35.217.128.0/17,35.220.45.0/24,35.242.45.0/24,35.243.56.0/21" },
        //{ "gsg1", "34.124.0.0-34.124.255.255,34.124.42.0-34.124.43.255,34.142.128.0-34.142.255.255,35.185.176.0-35.185.191.255,35.186.144.0-35.186.159.255,35.247.128.0-35.247.191.255,34.87.0.0-34.87.191.255,34.143.128.0-34.143.255.255,34.124.128.0-34.124.255.255,34.126.64.0-34.126.191.255,35.240.128.0-35.240.255.255,35.198.192.0-35.198.255.255,34.21.128.0-34.21.255.255,34.104.58.0-34.104.59.255,34.124.41.0-34.124.42.255,34.157.82.0-34.157.83.255,34.157.88.0-34.157.89.255,34.157.210.0-34.157.211.255,35.187.224.0-35.187.255.255,35.197.128.0-35.197.159.255,35.213.128.0-35.213.191.255,35.220.24.0-35.220.25.255,35.234.192.0-35.234.207.255,35.242.24.0-35.242.25.255,34.126.128.0/18,34.87.128.0/18,34.21.128.0/17,34.87.0.0/17,34.87.128.0/18,34.104.58.0/23,34.104.106.0/23,34.124.42.0/23,34.124.128.0/17,34.126.64.0/18,34.126.128.0/18,34.142.128.0/17,34.143.128.0/17,34.157.82.0/23,34.157.88.0/23,34.157.210.0/23,35.185.176.0/20,35.186.144.0/20,35.187.224.0/19,35.197.128.0/19,35.198.192.0/18,35.213.128.0/18,35.220.24.0/23,35.234.192.0/20,35.240.128.0/17,35.242.24.0/23,35.247.128.0/18,34.101.18.0/24,34.101.20.0/22,34.101.24.0/22,34.101.32.0/19,34.101.64.0/18,34.101.128.0/17,34.128.64.0/18,35.219.0.0/17" },
        //{ "icn1", "121.254.0.0-121.254.255.255,117.52.0.0-117.52.255.255,202.9.66.0/23" },
        //{ "gan3", "34.64.0.0-34.64.255.255,121.254.0.0-121.254.255.255,34.0.96.0/19,34.64.32.0/19,34.64.64.0/22,34.64.68.0/22,34.64.72.0/21,34.64.80.0/20,34.64.96.0/19,34.64.128.0/22,34.64.132.0/22,34.64.136.0/21,34.64.144.0/20,34.64.160.0/19,34.64.192.0/18,35.216.0.0/17,34.22.64.0/19,34.22.96.0/20" },
        //{ "tpe1", "5.42.160.0-5.42.160.255,35.221.128.0/17" },
        //{ "gmec1", "34.1.32.0/20,34.18.0.0/16,34.157.126.0/23,34.157.252.0/23" },
        //{ "gmec2", "34.1.48.0/20,34.152.84.0/23,34.166.0.0/16,34.177.48.0/23" },
        //{ "syd2", "158.115.196.0/23,37.244.42.0-37.244.42.255,34.87.192.0/18,34.104.104.0/23,34.116.64.0/18,34.124.40.0/23,34.151.64.0/18,34.151.128.0/18,35.189.0.0/18,35.197.160.0/19,35.201.0.0/19,35.213.192.0/18,35.220.41.0/24,35.234.224.0/20,35.242.41.0/24,35.244.64.0/18,34.104.122.0/23,34.124.58.0/23,34.126.192.0/20,34.129.0.0/16,34.0.16.0/20" },

    }),
    
    endpoints({

        /*js
            document.body.innerText.trim().replaceAll('\n\n', ',').replaceAll('\n', ',');
        */

        // https://ipinfo.io/AS57976/137.221.68.0/24
        // code ips.at("lax1") + "," + ips.at("las1") + "," + ips.at("guw2")    



        // TODO fix prop spacing

        // web https://www.reddit.com/r/Overwatch/comments/6blbkj/comment/dhnpq7k/

        {
            .title=                   "NA - CENTRAL",
            ._ping_ip=                "137.221.69.29",
            .heading=                 "ORD1",
            ._firewall_rule_address=  ips.at("na/central"),
            ._firewall_rule_description =             "Blocks USA - CENTRAL",
            .favorite=                true,
        },

        {
            .title = "NA - WEST",
            ._ping_ip = "137.221.68.83",
            .heading = "LAX1 and LAS1", // guw2 ??
            ._firewall_rule_address = ips.at("na/west"),
            ._firewall_rule_description = "Blocks USA - SOUTHWEST",
            .favorite = true,
        },

        {
            .title = "EUROPE",
            ._ping_ip = "137.221.78.69",
            .heading = "AMS1, CDG1", // Amsterdam, Paris
            ._firewall_rule_address = ips.at("eu"),
            ._firewall_rule_description = "Blocks EU",
            .favorite = true,
        },

        {
            .title = "JAPAN",
            ._ping_ip = "52.94.8.94",
            .heading = "GTK1", // Tokyo
            ._firewall_rule_address = ips.at("japan"),
            ._firewall_rule_description = "Blocks JAPAN",
            .favorite = true,
        },

        {
            .title = "SOUTH KOREA",
            ._ping_ip = "137.221.65.65",
            .heading = "ICN1", // Seoul
            ._firewall_rule_address = ips.at("south_korea"),
            ._firewall_rule_description = "Blocks SOUTH KOREA",
            .favorite = true,
        },

        {
            .title = "AUSTRALIA",
            ._ping_ip = "137.221.85.67",
            .heading = "SYD2", // Sydney
            ._firewall_rule_address = ips.at("australia"),
            ._firewall_rule_description = "Blocks AUSTRALIA",
            .favorite = true,
        },

        {
            .title = "SINGAPORE",
            ._ping_ip = "35.71.118.14", // dynamo db
            .heading = "GSG1",
            ._firewall_rule_address = ips.at("singapore"),
            ._firewall_rule_description = "Blocks SINGAPORE",
            .favorite = true,
        },

        {
            .title = "TAIWAN",
            ._ping_ip = "137.221.112.69",
            .heading = "TPE1", // Taipei
            ._firewall_rule_address = ips.at("taiwan"),
            ._firewall_rule_description = "Blocks TAIWAN",
            .favorite = true,
        },

        {
            .title = "BRAZIL",
            ._ping_ip = "52.94.7.202",
            .heading = "GBR1",
            ._firewall_rule_address = ips.at("brazil"),
            ._firewall_rule_description = "Blocks BRAZIL",
            .favorite = true,
        },

        {
            .title = "MIDDLE EAST",
            ._ping_ip = "13.248.66.130",
            .heading = "GMEC2",
            ._firewall_rule_address = ips.at("middle_east"),
            ._firewall_rule_description = "Blocks MIDDLE EAST",
            .favorite = true,
        },

    }),

    pinging(true)
{
    this->processes["Overwatch.exe"] = {
        .pid = 0,
        .on = false,
        .icon = { ImageTexture{ nullptr, 0, 0 } },
        .window = 0,
    };

    std::thread([&]()
    {

        // wait until imgui is loaded
        while (ImGui::GetCurrentContext() == nullptr)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

        while (true)
        {
            bool __previous__application_open = appStore.application_open;

            static const std::string process_name = "Overwatch.exe";
            static const std::string module_name = "Overwatch.exe";

            int pid = find_process (process_name);
            HWND window = find_window ("Overwatch");

            appStore.application_open = window;
            this->processes["Overwatch.exe"].on = window;
            this->processes["Overwatch.exe"].window = window;

            if (__previous__application_open && !appStore.application_open)
            {
                std::thread([&]() {
                    firewallManager.sync(&(this->endpoints));
                }).detach();

                appStore.dashboard.heading = __default__appStore.dashboard.heading;

                this->game_restart_required = false;
            }

            if (!__previous__application_open && appStore.application_open)
            {
                appStore.dashboard.heading = "Blocking new servers won't take effect until Overwatch is restarted.";
            }

            if (this->processes[process_name].icon.texture == nullptr)
            {
                // just prints stuff for now
                get_module(pid, module_name);
                            
                loadPicture("process_icon_overwatch", "png", &(this->processes[process_name].icon.texture), &(this->processes[process_name].icon.width), &(this->processes[process_name].icon.height));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(900));
        }
    }).detach();


    std::thread([&]()
    {
        // 1) load rules from wf.msc
        firewallManager._syncFirewallWithEndpoints(&(this->endpoints));

        // 2) reset all rules
        firewallManager.flushRules(&(this->endpoints));

        // 3) sync with endpoints
        firewallManager._syncEndpointsWithFirewall(&(this->endpoints));
    }
    ).detach();

}

void DashboardManager::loadAssets() {

    static const std::vector<std::string> textures = {
        "icon_options.png",
        "icon_maple_leaf.png",
        "icon_chain_slash.png",
        "icon_bolt.png",
        "icon_skull.png",
        "icon_heart.png",
        "icon_outside_window.png",

        "icon_wifi_slash.png",
        "icon_wifi_poor.png",
        "icon_wifi_fair.png",
        "icon_wifi.png",

        "icon_allow.png",
        "icon_block.png",
        "icon_wall_fire.png",

        "icon_angle.png",

        "background_app.png",
        "background_diagonal.png",
    };

    std::thread([]()
    {
        for (std::string texture : textures)
            _add_texture(texture.substr(0, texture.find(".")), texture.substr(texture.find(".") + 1));

        std::cout << "<" << std::hex << std::this_thread::get_id() << "> loaded " << std::dec << textures.size() << " textures." << std::endl;
    }).detach();
}


void DashboardManager::RenderInline()
{

    if (ImGui::GetCurrentContext() != nullptr && ImGui::IsWindowAppearing())
        this->loadAssets();

    {
        // move the numbers around slightly in between pings >:p
        // tricky tricky
        if (this->pinging && (ImGui::GetFrameCount() % 200) == 0)
        {
            static const int max = 1;
            static const int min = -2;
            static const int range = max - min + 1;

            for (auto& endpoint : endpoints)
                if (!((endpoint.display_ping != endpoint.ping) || endpoint.ping < 0))
                    endpoint.display_ping = std::max(0, endpoint.display_ping + (rand() % range + min));
        }

        for (auto& endpoint : endpoints)
            if (endpoint.display_ping != endpoint.ping)
            {
                static const float min_delay = 1.0f;
                static const float max_delay = 15.0f;

                if (endpoint.ping > 0 && endpoint.display_ping <= 0)
                {
                    endpoint.display_ping = endpoint.ping;
                    continue;
                }

                float param = fmin((float) std::abs(endpoint.ping - endpoint.display_ping) / 64.0f, 1.0f);

                if (ImGui::GetFrameCount() % (int)fmax(min_delay, max_delay - (max_delay * param * half_pi)) != 0)
                    continue;

                if (endpoint.display_ping < endpoint.ping)
                    endpoint.display_ping ++;
                else
                    endpoint.display_ping --;

                endpoint.display_ping = std::max(0, endpoint.display_ping);
            }
    }

    {
        if (ImGui::IsWindowAppearing())
            this->startPinging();

        static auto &style = ImGui::GetStyle();
        ImU32 const white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, style.Alpha });
        // static ImU32 const transparent = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0 });

        ImVec2 const windowPos = ImGui::GetWindowPos();
        ImVec2 const windowSize = ImGui::GetWindowSize();

        ImDrawList* list = ImGui::GetWindowDrawList();
        ImDrawList* bg_list = ImGui::GetBackgroundDrawList();
        //ImDrawList* fg_list = ImGui::GetForegroundDrawList();
        float const scrollY = ImGui::GetScrollY();

        const auto color_button = ImColor::HSV(0, 0.4f, 1, style.Alpha);
        const auto color_button_hover = ImColor::HSV(0, 0.25f, 1, style.Alpha);

        static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
        // ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled])
        const ImU32 color_text_secondary = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .8f * style.Alpha });

        // background texture
        {
            // TODO dark mode if deactivated?
            bg_list->AddRectFilled(windowPos, windowPos + ImGui::GetWindowSize(), white, 9);
            bg_list->AddImageRounded(_get_texture("background_app"), windowPos - ImVec2(0, ImGui::GetScrollY() / 4), windowPos - ImVec2(0, ImGui::GetScrollY() / 4) + ImVec2(ImGui::GetWindowSize().x, 430), ImVec2(0, 0), ImVec2(1, 1), white, 9);
        }

        ImGui::Spacing();

        // notice
        /* {
            ImGui::Dummy({ 0, 0 });
            list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMin() + ImVec2(ImGui::GetContentRegionAvail().x, 190), color_button, 5.0f);

            ImGui::Indent();
            ImGui::BeginGroup();
            {
                list->AddText(font_title, font_title->FontSize, ImGui::GetCursorScreenPos() - ImVec2(1, 0), white, "NOTICE");
                ImGui::Dummy({ 0, 32 });

                //ImGui::Bullet();
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::BeginChild("notice_text", ImVec2(ImGui::GetContentRegionAvail().x - 16, 140), false, ImGuiWindowFlags_NoScrollbar);
                ImGui::TextWrapped("blizzard added more servers today for the venture playtest. this means you are more likely to DC. i'd advise not blocking any servers today. this notice will go away when the issue is fixed.\n\nhttps://twitter.com/stormyy_ow\n ");
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            ImGui::EndGroup();
            ImGui::Unindent();
            //ImGui::Dummy({ ImGui::GetContentRegionAvail().x, 16 });
        } */

        // header
        {
            auto const size = 24;
            const auto widgetPos = ImGui::GetCursorScreenPos();

            // dashboard copy
            ImGui::BeginGroup();
            {
                list->AddText(font_title, font_title->FontSize, widgetPos - ImVec2(1, 0), color_text, appStore.dashboard.title.c_str());
                ImGui::Dummy({ 0, font_title->FontSize - 6 });

                //ImGui::Bullet();
                ImGui::TextWrapped(appStore.dashboard.heading.c_str());
            }
            ImGui::EndGroup();


            // buttons
            {
                ImGui::SameLine();
                ImGui::Dummy({ ImGui::GetContentRegionAvail().x - size - 4 - size - 12, size });
                ImGui::SameLine(NULL, 0);

                static const auto offset = ImVec2(0, 10);
                ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);

                // socials
                {
                    ImGui::Dummy({ size, size });
                    list->AddImage((void*)_get_texture("icon_heart"), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

                    if (ImGui::IsItemClicked())
                        ImGui::OpenPopup("socials");
                }

                ImGui::SameLine(NULL, 16);
                ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);

                // options
                {
                    ImGui::Dummy({ size, size });
                    list->AddImage((void*)_get_texture("icon_options"), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

                    if (ImGui::IsItemClicked())
                        ImGui::OpenPopup("options");
                }
            }
        }

        {

            static const auto color = ImGui::ColorConvertFloat4ToU32({ .4f, .4f, .4f, 1.0f });
            // static const auto color_2 = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0.6f });
            static const std::string text = "GAME RESTART REQUIRED";

            static const auto font = font_subtitle;
            static const auto font_size = font->CalcTextSizeA(font_subtitle->FontSize, FLT_MAX, 0.0f, text.c_str());


            if (this->game_restart_required)
            {
                ImGui::Dummy({ ImGui::GetContentRegionAvail().x, font_size.y + 16 });
                
                list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 5.0f);

                {
                    static const auto color = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.09f });
                    const auto pos = ImGui::GetItemRectMin();

                    static const auto image = _get_image("background_diagonal");

                    list->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
                    list->AddImage(image.texture, pos, pos + ImVec2(image.width, image.height), ImVec2(0, 0), ImVec2(1, 1), color);
                    list->PopClipRect();
                }

                const auto pos = ImGui::GetItemRectMin() + ImVec2((ImGui::GetItemRectSize().x - font_size.x) / 2, 8 - 2);
                list->AddText(font_subtitle, font_subtitle->FontSize, pos, white, text.c_str());
            }

            /*else
            {
                list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_2, 5.0f, 0);

                static const std::string tips[3] = {
                    "TIP: Don't queue with other people",
                    "tip 2",
                };

                list->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                list->AddText(font_subtitle, font_subtitle->FontSize, ImGui::GetItemRectMin() + ImVec2(style.FramePadding.x, 8 - 1), white, tips[((int) (ImGui::GetTime() / 9)) % 2].c_str());
                list->PopClipRect();
            }*/

            else
            {
                /*static const auto n_buttons = 3;
                static const auto button_width = (ImGui::GetContentRegionAvail().x / n_buttons);
                static const auto button_height = 36;

                ImGui::PushFont(font_subtitle);
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { style.FramePadding.x, style.FramePadding.y - 4 });
                {
                    ImGui::Button("ALL ON", { button_width, button_height });
                    ImGui::SameLine();
                    ImGui::Button("RESET APP", { button_width, button_height });
                    ImGui::SameLine();
                    ImGui::Button("TEST", { button_width, button_height });
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::PopFont();*/

                // static const auto color_2 = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0.9f });

                /*{
                    ImGui::PushID(1);
                    ImGui::PushStyleColor(ImGuiCol_Header, button);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, button_hovered);
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, button_active);
                    ImGui::PushStyleColor(ImGuiCol_NavHighlight, NULL);
                    bool action = (ImGui::Selectable("##end", &(this->all_selected), ImGuiSelectableFlags_SelectOnClick, { button_width, button_height }));
                    ImGui::PopStyleColor(4);
                    ImGui::PopID();

                    if (!this->all_selected) list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_2, 5.0f);
                    
                    static const std::string text = "ALL ON";
                    list->AddText(font_subtitle, font_subtitle->FontSize, ImGui::GetItemRectMin() + ImVec2(style.FramePadding.x, 8 - 1), white, text.c_str());

                    if (action)
                    {
                    }
                }*/
            }

        }

        /*for (auto const& [_p, process] : this->processes)
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

                    static const auto color = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.6f });
                    static const auto color_2 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.4f });
                    static const auto text_color_2 = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0.4f * style.Alpha });
                    static const auto icon_color_2 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.4f * style.Alpha });


                    list->AddRectFilled(p_min, p_max, hovered ? white : (!process.on ? color_2 : color), 9);
                    if (hovered)
                        list->AddRect(p_min, p_max, white, 14, NULL, 4);

                    const auto frame_pos = p_min + ImVec2(8, 8);
                    list->AddImage((void*)process.icon.texture, frame_pos, frame_pos + frame, ImVec2(0, 0), ImVec2(1, 1), !process.on ? icon_color_2 : white);

                    const auto text_pos = p_min + ImVec2(frame.x + 16, 6);
                    list->AddText(text_pos, !process.on ? text_color_2 : color_text, "overwatch.exe");
                }

                if(!process.on)
                    ImGui::EndDisabled();
            }
            ImGui::EndGroup();*/

            /*{
                static const auto width = 140;
                
                bool selected;

                ImGui::SameLine(NULL, 0);
                ImGui::Dummy({ ImGui::GetContentRegionAvail().x - width , 20 });
                ImGui::SameLine(NULL, 0);
                ImGui::Selectable("disable", &selected, 0, { width, 40 });

                static const auto color_2 = ImGui::ColorConvertFloat4ToU32({ .4, .4, .4, 1.0f });

                list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_2, 9);

                auto const pos = ImGui::GetItemRectMin() + style.FramePadding;
                
                list->AddImage(_get_texture("icon_skull"), pos, pos + ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), white);

                list->AddText(pos + style.FramePadding + ImVec2(24, -8), white, "KILLSWITCH");
            }*/

            /*

            if (ImGui::IsItemClicked() && process.window)
            {
                // focus window
                SetForegroundWindow(process.window);

                // maximize window
                PostMessage(process.window, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
        }*/

        ImGui::Spacing();

        {
            ImGui::BeginChild("endpoints_scrollable", ImVec2(ImGui::GetContentRegionAvail().x, 540), false);
            ImGui::Spacing();
            {
                int i = 0;
                for (auto& endpoint : this->endpoints)
                {
                    auto const disabled = endpoint._ping_ip.empty() || (endpoint._has_pinged && !(endpoint.ping > 0) && !endpoint._has_pinged_successfully);

                    auto const unsynced = (endpoint.active != endpoint.active_desired_state);

                    // 0.4f looks quite good
                    static const ImU32 color_disabled = ImGui::ColorConvertFloat4ToU32({ .8f, .8f, .8f, 1.0f });
                    static const ImU32 color_disabled_secondary = ImGui::ColorConvertFloat4ToU32({ .88f, .88f, .88f, 1.0f });
                    static const ImU32 color_disabled_secondary_faded = ImGui::ColorConvertFloat4ToU32({ .95f, .95f, .95f, 1.0f });

                    ImU32 const color = disabled ? color_disabled : (ImU32) ImColor::HSV(i / 14.0f, 0.4f, 1.0f, style.Alpha);
                    ImU32 const color_secondary = disabled ? color_disabled_secondary : (ImU32) ImColor::HSV(i / 14.0f, 0.3f, 1.0f, style.Alpha);
                    ImU32 const color_secondary_faded = disabled ? color_disabled_secondary_faded : (ImU32) ImColor::HSV(i / 14.0f, 0.2f, 1.0f, 0.4f * style.Alpha);

                    auto const w_list = ImGui::GetWindowDrawList();


                    ImGui::Dummy({ 0 ,0 });
                    ImGui::SameLine(NULL, 16);
                    
                    // ImGui::Dummy({ ImGui::GetContentRegionAvail().x - 16, 73 });
                    ImGui::PushID(i);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, endpoint.active ? color_secondary : color_secondary_faded);
                    ImGui::PushStyleColor(ImGuiCol_Header, endpoint.active ? color_secondary : color_secondary_faded);
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, endpoint.active ? color_secondary : color_secondary_faded);
                    ImGui::PushStyleColor(ImGuiCol_NavHighlight, NULL);
                    bool action = (ImGui::Selectable("##end", &(endpoint.active_desired_state), ImGuiSelectableFlags_SelectOnClick, { ImGui::GetContentRegionAvail().x - 16, 74 - 9 }));
                    ImGui::PopStyleColor(4);
                    ImGui::PopID();

                    ImGui::Spacing();

                    auto hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

                    // background
                    if (hovered)
                        if (endpoint.active)
                            w_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_secondary, 5, 0, 8);

                    if (!hovered && endpoint.active)
                        w_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 5, NULL);

                    // unsynced background
                    if (unsynced)
                    {
                        const auto& color = color_secondary_faded;
                        const auto offset = (ImGui::GetFrameCount() / 4) % 40;
                        const auto offset_vec = ImVec2((float) offset, (float) offset);
                        const auto pos = ImGui::GetItemRectMin() - ImVec2(40, 40) + offset_vec;

                        static const auto image = _get_image("background_diagonal");

                        w_list->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
                        w_list->AddImage(image.texture, pos, pos + ImVec2(image.width, image.height), ImVec2(0, 0), ImVec2(1, 1), color);
                        w_list->PopClipRect();
                    }

                    // icon
                    const auto icon_frame = ImVec2({ ImGui::GetItemRectSize().y, ImGui::GetItemRectSize().y });
                    static auto padding = ImVec2(21, 21);
                    const auto icon = !unsynced ? (endpoint.active ? _get_texture("icon_allow") : _get_texture("icon_block")) : _get_texture("icon_wall_fire");
                    w_list->AddImage(icon, ImGui::GetItemRectMin() + padding, ImGui::GetItemRectMin() + icon_frame - padding, ImVec2(0, 0), ImVec2(1, 1), endpoint.active ? color_text_secondary : color /*color_secondary_faded*/);

                    // display 1
                    auto pos = ImGui::GetItemRectMin() + ImVec2(icon_frame.x, 4) + ImVec2(-2, 0);
                    w_list->AddText(font_title, 35, pos, endpoint.active ? white : color, endpoint.title.c_str());

                    // display 2
                    pos += ImVec2(1, ImGui::GetItemRectSize().y - 24 - 16);
                    w_list->AddText(font_subtitle, 24, pos, endpoint.active ? color_text_secondary : color_secondary, endpoint.heading.c_str());

                    // popup
                    /*{
                        std::string key = std::format("endpoint{0}", i);
                        if (hovered || ImGui::IsPopupOpen(key.c_str()))
                            if (ImGui::BeginPopupContextItem(key.c_str()))
                            {
                                {
                                    if (ImGui::MenuItem("all but this one")) {
                                        
                                    }
                                    
                                }
                                 
                                if (!hovered && !ImGui::IsWindowHovered())
                                    ImGui::CloseCurrentPopup();

                                ImGui::EndPopup();
                            }

                        ImGui::OpenPopupOnItemClick(key.c_str());
                    }*/

                    // action
                    if (action)
                    {
                        // std::thread([&]() {
                            firewallManager.sync(&(this->endpoints), appStore.application_open);

                            bool pending_actions = false;
                            for (auto& e : this->endpoints)
                                if (e.active != e.active_desired_state)
                                    pending_actions = true;

                            this->game_restart_required = pending_actions;
                        //}).detach();

                        // fill missing frame
                        w_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 5, NULL);
                    }


                    // display 3 / (icon wifi)
                    if (!endpoint._ping_ip.empty() || endpoint._has_pinged)
                    {
                        auto icon = _get_texture("icon_wifi_slash");
                        static ImVec2 frame = ImVec2(26, 26);

                        if (0 < endpoint.ping)
                        {
                            if (endpoint.display_ping > 120)
                                icon = _get_texture("icon_wifi_poor");
                            else if (endpoint.display_ping > 60)
                                icon = _get_texture("icon_wifi_fair");
                            else
                                icon = _get_texture("icon_wifi");
                        }

                        auto pos = ImGui::GetItemRectMax() - ImVec2(frame.x, ImGui::GetItemRectSize().y) + (style.FramePadding * ImVec2(-1, 1)) + ImVec2(-8, 1);
                        w_list->AddImage(icon, pos, pos + frame, ImVec2(0, 0), ImVec2(1, 1), endpoint.active ? white : color);

                    }

                    // display 4
                    if (!endpoint._ping_ip.empty() || endpoint._has_pinged)
                    {
                        auto const text = std::to_string(endpoint.display_ping);
                        auto text_size = font_subtitle->CalcTextSizeA(24, FLT_MAX, 0.0f, text.c_str());
                        auto pos = ImGui::GetItemRectMax() - style.FramePadding - text_size + ImVec2(-8, -4);

                        w_list->AddText(font_subtitle, 24, pos, endpoint.active ? color_text_secondary : color_secondary, text.c_str());
                    }

                    // ImGui::Text("active: %d, desired: %d", endpoint.active, endpoint.active_desired_state);

                    i += 1;
                }
            }
            ImGui::EndChild();

            ImGui::Spacing();

            // bottom part
            {
                ImGui::Dummy({ ImGui::GetContentRegionAvail().x, ImGui::GetFont()->FontSize + (style.FramePadding.y * 2) });
                list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), white, 5);
                list->AddText(ImGui::GetItemRectMin() + style.FramePadding + ImVec2(2, 0), color_text, "Advanced");

                static ImVec2 frame = ImVec2(24, 24);
                auto const pos = ImVec2(ImGui::GetItemRectMax() - frame - style.FramePadding);
                auto const uv_min = !this->show_all ? ImVec2(0, 0) : ImVec2(0, 1);
                auto const uv_max = !this->show_all ? ImVec2(1, 1) : ImVec2(1, 0);
                list->AddImage(_get_texture("icon_angle"), pos, pos + frame, uv_min, uv_max, color_text);

                if (ImGui::IsItemClicked())
                    this->show_all = !this->show_all;

                if (this->show_all)
                {
                    ImGui::BeginGroup();
                    {
                        ImGui::Indent(style.FramePadding.x);
                        {
                            ImGui::Text("Unavailable in this version.");
                            // static bool test;
                            // ToggleButton("test", &test);
                        }
                        ImGui::Unindent();
                    }
                    ImGui::EndGroup();
                    
                }
            }
        }

        // todo: COMPONENT
        {
            if (ImGui::BeginPopupModal("options", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoResize))
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
                    /*if (ImGui::MenuItem("PINGING", this->pinging ? "on" : "off")) {
                        this->pinging = !(this->pinging);

                        if (this->pinging)
                            this->startPinging();
                        else
                            cv.notify_all();
                    }*/

                    // network settings
                    static auto frame = ImVec2(18, 18);
                    static auto offset = ImVec2(14, 8);
                    static auto offset2 = ImVec2(130, 0);

                    {
                        static auto text = "network settings";
                        if (ImGui::MenuItem(text, ""))
                        {
                            system("start windowsdefender://network");
                        }
                        list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset + offset2, ImGui::GetItemRectMin() + offset + offset2 + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);
                        ImGui::SetItemTooltip("windowsdefender://network");
                    }

                    {
                        static auto text = "firewall rules";
                        if (ImGui::MenuItem("firewall rules", ""))
                        {
                            system("start wf.msc");
                        }
                        list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset + offset2, ImGui::GetItemRectMin() + offset + offset2 + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);
                        ImGui::SetItemTooltip("wf.msc");
                    }

                    /*if (ImGui::MenuItem("AUTO UPDATE", options.auto_update ? "on" : "off", nullptr, true))
                    {
                        options.auto_update = !options.auto_update;
                    }*/

                    /*if (ImGui::MenuItem("SAVE FILE", "off", nullptr, false))
                    {
                    }
                    ImGui::SetItemTooltip("create a save file? you will\nhave the option to delete it\nif you uninstall.\n\ndropship needs this to keep\noptions");*/
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
                ImGui::Text("SOCIALS");
                ImGui::PopFont();

                ImGui::PushFont(font_subtitle);
                {
                    static auto frame = ImVec2(18, 18);
                    static auto offset = ImVec2(180, 8);

                    // discord
                    if (ImGui::MenuItem("discord\n ", "suggestions\nhelp", nullptr, true)) {
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
                    if (ImGui::MenuItem("github\n ", "guide\ncode", nullptr, true)) {
                        system("start https://github.com/stowmyy/dropship-test");
                    }
                    list->AddImage(_get_texture("icon_outside_window"), ImGui::GetItemRectMin() + offset, ImGui::GetItemRectMin() + offset + frame, ImVec2(0, 0), ImVec2(1, 1), color_button);

                }
                ImGui::PopFont();
                ImGui::PopItemFlag();
                ImGui::EndPopup();
            }
        }
    }

    #ifdef _DEBUG
    {
        ImGui::Begin("debug");
        if (ImGui::CollapsingHeader("dashboard.c", ImGuiTreeNodeFlags_None))
        {
            {
                if (ImGui::Button("new", { 60, 0 }))
                {
                    firewallManager.AddFirewallRule(&(this->endpoints.at(0)), true);
                }
                ImGui::SameLine();
                if (ImGui::Button("flush", { 60, 0 }))
                {
                    firewallManager.flushRules(&(this->endpoints));
                }

                if (ImGui::Button("sync (apply all)", { 200, 0 }))
                {

                    firewallManager.sync(&(this->endpoints));
                }

                ImGui::SameLine();

                if (ImGui::Button("sync endpoints", { 200, 0 }))
                    firewallManager._syncEndpointsWithFirewall(&(this->endpoints));

                if (ImGui::Button("sync firewall", { 200, 0 }))
                    firewallManager._syncFirewallWithEndpoints(&(this->endpoints));
            }
        }
        ImGui::End();
    }
    #endif
}
