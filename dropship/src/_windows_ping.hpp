#pragma once

// todo remove
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <stdio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#include "_windows_domain.hpp"

#include <string>
#include <format>

#include <iostream> // cout (this_thread)


//static std::mutex s_pingMutex;



/*

    tracert 24.105.30.129

      6    13 ms    11 ms    11 ms  eqix-ix-ch1.blizzard.com [208.115.136.85]
      7     *        *        *     Request timed out.
      8     *        *        *     Request timed out.
      9    57 ms    53 ms    52 ms  137.221.68.91
     10    59 ms    57 ms    61 ms  lax-eqla1-ia-bons-03.as57976.net [137.221.66.7]
     11    58 ms    57 ms    55 ms  24.105.30.129


     https://tools.keycdn.com/traceroute

     // NOTES
        > eqix-ix-ch1.blizzard.com [208.115.136.85] // (outbound) blizzard chicago data center // https://www.pch.net/ixp/details/268 ("blizzard" tenant)
        > lax-eqla1-ia-bons-03.as57976.net [137.221.66.7] // (inbound) blizzard lax data center // https://bgp.he.net/AS57976 (blizzard domain)

        > 137.221.68.91 // https://ipinfo.io/137.221.68.91

*/

// gives verdad when succeeded
bool _windows_ping(std::string hostname, int* ping, int timeout = 2000)
{

    //printf(std::format("thread: {0}  ", std::to_string(std::this_thread::get_id()).c_str()).c_str());

    // std::cout << "thread: " << std::this_thread::get_id() << "  ";

    {
        if (validIPv4(hostname))
        {
           // printf(std::format("{0} is a valid ipv4 address\n", hostname).c_str());
        }
        else
        {

            std::string new_hostname;
            _windows_domain_to_ip(hostname, &new_hostname);

            //printf(std::format("(alias) {0} >> {1}\n", new_hostname, hostname).c_str());
        }

    }


    auto& ip = hostname;


    // Declare and initialize variables.
    HANDLE hIcmpFile;
    unsigned long ipaddr = INADDR_NONE;
    DWORD dwRetVal = 0;
    DWORD dwError = 0;
    char SendData[] = "Data Buffer";
    LPVOID ReplyBuffer = NULL;
    DWORD ReplySize = 0;

    ipaddr = inet_addr(ip.c_str());
    if (ipaddr == INADDR_NONE) {
        printf("missing ip\n");
        return false;
    }

    hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        printf("\tUnable to open handle.\n");
        printf("IcmpCreatefile returned error: %ld\n", GetLastError());
        return false;
    }

    // Allocate space for a single reply.
    ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
    ReplyBuffer = (VOID*)malloc(ReplySize);
    if (ReplyBuffer == NULL) {
        printf("\tUnable to allocate memory for reply buffer\n");
        return false;
    }

    dwRetVal = IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
        ipaddr, SendData, sizeof(SendData), NULL,
        ReplyBuffer, ReplySize, timeout);
    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
        struct in_addr ReplyAddr;
        ReplyAddr.S_un.S_addr = pEchoReply->Address;
        //printf("\t>> to %s\n", ip.c_str());

            //printf("\t<< %ld message(s) from %s\n", dwRetVal, inet_ntoa(ReplyAddr));
            //printf("\tInformation from this response:\n");
 
        //printf("\t  Received from %s\n", inet_ntoa(ReplyAddr));
        //printf("\t  Status = %ld  ", pEchoReply->Status);
        switch (pEchoReply->Status) {
            case IP_DEST_HOST_UNREACHABLE:
                printf("(Destination host was unreachable)\n");
                break;
            case IP_DEST_NET_UNREACHABLE:
                printf("(Destination Network was unreachable)\n");
                break;
            case IP_REQ_TIMED_OUT:
                printf("(Request timed out)\n");
                break;
            default:
                //printf("\n");
                break;
        }

        /*printf("\t%ldms\n\n",
            pEchoReply->RoundTripTime);*/

        // printf(std::format("(icmp) {0}ms {1}\n", pEchoReply->RoundTripTime, inet_ntoa(ReplyAddr)).c_str());


        //std::lock_guard<std::mutex> lock(s_pingMutex);
        *ping = pEchoReply->RoundTripTime;
    }
    else {
        // ping -1 = failed
        // ping -2 = timed out (aka anything)
        //*ping = 0;
        printf("Call to IcmpSendEcho2 failed.\n");
        dwError = GetLastError();
        switch (dwError) {
            case IP_BUF_TOO_SMALL:
                printf("\tReplyBufferSize too small\n");
                break;
            case IP_REQ_TIMED_OUT:
                printf("\tRequest timed out D:\n");
                *ping = -1;
                break;
            default:
                printf("\tExtended error returned: %ld\n", dwError);
                *ping = -2;
                break;
        }
        return false;
    }
    return true;
}
