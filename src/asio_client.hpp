#pragma once
#include <iostream>
#include <array>
#include <list>
#include <memory>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

#include "entities.cpp"
#include "asio_packets.cpp"
#include "client_world.cpp"

using asio::ip::tcp;

struct Core;

namespace netwk {
    struct TCP_client {
        asio::io_context io_context;
        tcp::resolver resolver;
        tcp::socket socket;
        std::thread recieve_thread;

        TCP_client(uint16_t target_port);

        void receive_loop(Core& core, bool& game_running);

        void start(Core& core, bool& game_running);

        void send(std::vector<uint8_t> packet);
    };
}