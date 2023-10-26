#pragma once

// https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo

// #undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")


#include <string>
#include <format>


bool validIPv4(std::string ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return result > 0;
}


// gives true if successful
//bool domain_to_ip (std::string hostname, int port = 0)
bool _windows_domain_to_ip(std::string _ping_ip, std::string* ipv4_outside, int port = 0)
{

    //-----------------------------------------
    // Declare and initialize variables
    WSADATA wsaData;
    int iResult;
    INT iRetval;

    DWORD dwRetval;

    int i = 1;

    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;

    struct sockaddr_in* sockaddr_ipv4;
    //    struct sockaddr_in6 *sockaddr_ipv6;
    LPSOCKADDR sockaddr_ip;

    char ipstringbuffer[46];
    DWORD ipbufferlength = 46;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }

    //--------------------------------
    // Setup the hints address info structure
    // which is passed to the getaddrinfo() function
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //printf("Calling getaddrinfo with following parameters:\n");
    //printf("\tnodename = %s\n", hostname.c_str());
    //printf("\tservname (or port) = %d\n\n", port);

    //--------------------------------
    // Call getaddrinfo(). If the call succeeds,
    // the result variable will hold a linked list
    // of addrinfo structures containing response
    // information
    dwRetval = getaddrinfo(_ping_ip.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (dwRetval != 0) {
        printf("getaddrinfo failed with error: %d\n", dwRetval);
        WSACleanup();
        return false;
    }

    //printf("getaddrinfo returned success\n");

    // Retrieve each address and print out the hex bytes
    for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

        //printf("getaddrinfo response %d\n", i++);
        //printf("\tFlags: 0x%x\n", ptr->ai_flags);
        //printf("\tFamily: ");
        switch (ptr->ai_family) {
        case AF_UNSPEC:
            printf("Unspecified\n");
            break;
        case AF_INET:
            //printf("AF_INET (IPv4)\n");
            sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
            //printf("\tIPv4 address %s\n",
            //    inet_ntoa(sockaddr_ipv4->sin_addr));
                *ipv4_outside = inet_ntoa(sockaddr_ipv4->sin_addr);

            break;
        case AF_INET6:
            printf("AF_INET6 (IPv6)\n");
            // the InetNtop function is available on Windows Vista and later
            // sockaddr_ipv6 = (struct sockaddr_in6 *) ptr->ai_addr;
            // printf("\tIPv6 address %s\n",
            //    InetNtop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipstringbuffer, 46) );

            // We use WSAAddressToString since it is supported on Windows XP and later
            sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
            // The buffer length is changed by each call to WSAAddresstoString
            // So we need to set it for each iteration through the loop for safety
            ipbufferlength = 46;
            iRetval = WSAAddressToString(sockaddr_ip, (DWORD)ptr->ai_addrlen, NULL,
                ipstringbuffer, &ipbufferlength);
            if (iRetval)
                printf("WSAAddressToString failed with %u\n", WSAGetLastError());
            else
                printf("\tIPv6 address %s\n", ipstringbuffer);
            break;
        case AF_NETBIOS:
            printf("AF_NETBIOS (NetBIOS)\n");
            break;
        default:
            printf("Other %ld\n", ptr->ai_family);
            break;
        }
        /*printf("\tSocket type: ");
        switch (ptr->ai_socktype) {
        case 0:
            printf("Unspecified\n");
            break;
        case SOCK_STREAM:
            printf("SOCK_STREAM (stream)\n");
            break;
        case SOCK_DGRAM:
            printf("SOCK_DGRAM (datagram) \n");
            break;
        case SOCK_RAW:
            printf("SOCK_RAW (raw) \n");
            break;
        case SOCK_RDM:
            printf("SOCK_RDM (reliable message datagram)\n");
            break;
        case SOCK_SEQPACKET:
            printf("SOCK_SEQPACKET (pseudo-stream packet)\n");
            break;
        default:
            printf("Other %ld\n", ptr->ai_socktype);
            break;
        }
        printf("\tProtocol: ");
        switch (ptr->ai_protocol) {
        case 0:
            printf("Unspecified\n");
            break;
        case IPPROTO_TCP:
            printf("IPPROTO_TCP (TCP)\n");
            break;
        case IPPROTO_UDP:
            printf("IPPROTO_UDP (UDP) \n");
            break;
        default:
            printf("Other %ld\n", ptr->ai_protocol);
            break;
        }
        printf("\tLength of this sockaddr: %d\n", ptr->ai_addrlen);
        printf("\tCanonical name: %s\n", ptr->ai_canonname);*/
    }

    freeaddrinfo(result);
    WSACleanup();

    return true;
}

