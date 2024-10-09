#pragma once

#include <asio/asio.hpp>
#include <istream>
#include <iostream>
#include <ostream>

#include "icmp_header.hpp"
#include "ipv4_header.hpp"

using asio::ip::icmp;
using asio::steady_timer;
namespace chrono = asio::chrono;

class AsioPinger
{
public:
    //AsioPinger(const AsioPinger&) = delete;
    //AsioPinger& operator= (const AsioPinger&) = delete;

    AsioPinger(asio::io_context& io_context, const char* destination, std::unique_ptr<std::optional<int>> const& ping);

private:
    void start_send();
    void handle_timeout();
    void start_receive();
    void handle_receive(std::size_t length);
    static unsigned short get_identifier();

    icmp::resolver resolver_;
    icmp::endpoint destination_;
    icmp::socket socket_;
    steady_timer timer_;
    unsigned short sequence_number_;
    chrono::steady_clock::time_point time_sent_;
    asio::streambuf reply_buffer_;
    std::size_t num_replies_;

    std::unique_ptr<std::optional<int>> const& ping;
};
