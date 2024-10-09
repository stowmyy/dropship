#include "pch.h"

#include "process.h"



/**
 * gets the bitmap data for a module icon from a running process
 *
 * @param processId id of process to examine
 * @param module within process `processId` to extract icon from executable
 * @return int process id. returns 0 if not found
 */
int find_process(std::wstring procname)
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

        if (wcscmp(procname.c_str(), pe.szExeFile) == 0) {
            pid = pe.th32ProcessID;
            break;
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    // closes an open handle (CreateToolhelp32Snapshot)
    CloseHandle(hSnapshot);
    return pid;
}
