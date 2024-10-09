#pragma once

#include <optional>
#include <chrono>

#include "../vendor/clsocket/ActiveSocket.h"       // Include header for active socket object definition


std::optional<int> _tcp_ping(std::string destination, uint16 port, CSimpleSocket::CSocketType type = CSimpleSocket::SocketTypeTcp) {
    CActiveSocket socket(type);       // Instantiate active socket object (defaults to TCP).
    char          time[50];

    memset(&time, 0, 50);

    //--------------------------------------------------------------------------
    // Initialize our socket object 
    //--------------------------------------------------------------------------
    socket.Initialize();


    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();


    //--------------------------------------------------------------------------
    // Create a connection to the time server so that data can be sent
    // and received.
    //--------------------------------------------------------------------------
    if (socket.Open(destination.c_str(), port))
    {
        //----------------------------------------------------------------------
        // Send a requtest the server requesting the current time.
        //----------------------------------------------------------------------

        const auto ping = "Ping : Request (8)\n";
        if (socket.Send((const uint8*) ping, sizeof(ping)))
        {
            //----------------------------------------------------------------------
            // Receive response from the server.
            //----------------------------------------------------------------------
            socket.Receive(32);
            memcpy(&time, socket.GetData(), 28);
            printf("%s\n", time);

            //----------------------------------------------------------------------
            // Close the connection.
            //----------------------------------------------------------------------
            socket.Close();

            std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
            std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            return std::optional<int>{ (int)diff.count() };
        }


        socket.Receive();
        printf("\nno\n");


        //return std::optional<int>{ socket.GetTotalTimeMs() };
    }

    return std::nullopt;
}