#pragma once

/*
simple process find logic
author: @cocomelonc
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>

#include <string>

#include <shellapi.h> // SHFILEINFOW
#include <shobjidl_core.h> // imagefactory

#include <iostream> // cout

#include "images.h" // icon image

#include <tchar.h> // _t_printf

#include <math.h>


struct Endpoint
{
    std::string title;
    std::string _ping_ip;
    std::string heading;
    std::string _firewall_rule_address;
    std::string _firewall_rule_description;

    bool _has_pinged = { false }; // if it has attempted a ping yet ((any point)).
    bool _has_pinged_successfully = { false };

    bool favorite = { false };
    /*
        0 (>) = fine
        -1 = fail (timed out)
        -2 = block (any error)
        -3 (>) = unknown
    */
    int ping = -9;
    //std::shared_ptr<int> ping = std::make_shared<int>(-9);
    int display_ping = 0;

    bool active;
    bool active_desired_state;

};

struct OPTIONS
{
    bool auto_update;
};


struct DashboardStore
{
    std::string title;
    std::string heading;
};

struct AppStore
{
    // window the overlay is injected into at the moment
    std::string _window_overlaying;
    DashboardStore dashboard;
    bool application_open;
};

// IMGUI_API void ImageTurner(ImTextureID tex_id, ImVec2 center, ImVec2 size, float* angle_, float round_sec = 0, ImDrawList* draw_list = 0);



// windows only
static std::string wide_string_to_string(const std::wstring &wide_string)
{
    if (wide_string.empty())
    {
        return "";
    }

    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
    }

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
    return result;
}

static bool is_window_focused(std::string window_name)
{
    for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
    {
        bool found = false;

        if (!IsWindowVisible(hwnd))
            continue;

        int length = GetWindowTextLength(hwnd);
        if (length == 0)
            continue;

        WCHAR* title = new WCHAR[length + 1];
        GetWindowText(hwnd, title, length + 1);

        std::string s_title = wide_string_to_string(title);

        
        if (s_title == window_name)
        {
            std::cout << "HWND: " << hwnd << " Title: " << s_title << std::endl;
            // window pixels
            // return PWINDOWINFO
            found = true;
        }

        delete[] title;
        
        // this works because this is in the wrong place
       return found;
    }
}

static HWND find_window(std::string window_name)
{
    HWND found = 0;

    for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
    {

        int length = GetWindowTextLength(hwnd);
        if (length == 0)
            continue;

        WCHAR* title = new WCHAR[length + 1];
        GetWindowText(hwnd, title, length + 1);

        std::string s_title = wide_string_to_string(title);


        if (s_title == window_name)
        {
            // std::cout << "HWND: " << hwnd << " Title: " << s_title << std::endl;

            // window pixels
            // return PWINDOWINFO
            found = hwnd;
            break;
        }

        delete[] title;

    }
    
    return found;
}

/**
 * gets the bitmap data for a module icon from a running process
 *
 * @param processId id of process to examine
 * @param module within process `processId` to extract icon from executable
 * @return void
 */
static bool get_module (int process_id, std::string module_name)
{
    std::string _exe_path;

    /*
        GET MODULE INFO FOR PROCESS
            > https://learn.microsoft.com/en-us/windows/win32/toolhelp/traversing-the-module-list

            - base address
            - exe path
            - ..
    */
    {
        HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
        MODULEENTRY32 me32;

        //  Take a snapshot of all modules in the specified process. 
        hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);
        if (hModuleSnap == INVALID_HANDLE_VALUE)
        {
            printf("CreateToolhelp32Snapshot (of modules)");
            return(FALSE);
        }

        //  Set the size of the structure before using it. 
        me32.dwSize = sizeof(MODULEENTRY32);

        //  Retrieve information about the first module, 
        //  and exit if unsuccessful 
        if (!Module32First(hModuleSnap, &me32))
        {
            printf("error module32first");
            CloseHandle(hModuleSnap);     // Must clean up the snapshot object! 
            return(FALSE);
        }

        //  Now walk the module list of the process, 
        //  and display information about each module 
        do
        {
            // std::string moduleName(me32.szModule);

            // std::wcout << me32.szModule << L" != " << procname << std::endl;

            if (strcmp(wide_string_to_string(me32.szModule).c_str(), module_name.c_str()) == 0)
            {
                _exe_path = wide_string_to_string(me32.szExePath);

                {
                    _tprintf(TEXT("\n"));
                    _tprintf(TEXT("\n     MODULE NAME:     %s"), me32.szModule);
                    _tprintf(TEXT("\n     executable     = %s"), me32.szExePath);
                    _tprintf(TEXT("\n     process ID     = 0x%08X"), me32.th32ProcessID);
                    _tprintf(TEXT("\n     ref count (g)  =     0x%04X"), me32.GlblcntUsage);
                    _tprintf(TEXT("\n     ref count (p)  =     0x%04X"), me32.ProccntUsage);
                    _tprintf(TEXT("\n     base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
                    _tprintf(TEXT("\n     base size      = %d"), me32.modBaseSize);
                    _tprintf(TEXT("\n"));
                }
            }

        } while (Module32Next(hModuleSnap, &me32));

        _tprintf(TEXT("\n"));

        //  Do not forget to clean up the snapshot object. 
        CloseHandle(hModuleSnap);
    }
}

/**
 * gets the bitmap data for a module icon from a running process
 *
 * @param processId id of process to examine
 * @param module within process `processId` to extract icon from executable
 * @return int process id. returns 0 if not found
 */
static int find_process (std::string procname)
{

    /*
        FIND A PROCESS WITH NAME
        @https://cocomelonc.github.io/pentest/2021/09/29/findmyprocess.html

            - ..
    */
    HANDLE hSnapshot;
    PROCESSENTRY32 pe;
    int pid = 0;
    BOOL hResult;

    // snapshot of all processes in the system
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

    // initializing size: needed for using Process32First
    pe.dwSize = sizeof(PROCESSENTRY32);

    // info about first process encountered in a system snapshot
    hResult = Process32First(hSnapshot, &pe);

    // retrieve information about the processes
    // and exit if unsuccessful
    while (hResult) {
        // if we find the process: return process ID

        // std::wcout << pe.szExeFile << L" != " << procname.c_str() << std::endl;

        if (strcmp(procname.c_str(), wide_string_to_string(pe.szExeFile).c_str()) == 0) {
            pid = pe.th32ProcessID;
            break;
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    // closes an open handle (CreateToolhelp32Snapshot)
    CloseHandle(hSnapshot);
    return pid;
}
