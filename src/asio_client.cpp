#pragma once
#include <iostream>
#include <array>
#include <list>
#include <memory>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

#include "entities.cpp"
#include "asio_packets.cpp"

using asio::ip::tcp;

namespace netwk {
    struct TCP_client {
        asio::io_context io_context;
        tcp::resolver resolver;
        tcp::socket socket;
        std::thread recieve_thread;

        TCP_client(unsigned short int target_port);

        void receive_loop(std::vector<std::string>& input_collector, std::unordered_map<uint64_t, Entity>& player_map, bool& game_running);

        void start(std::vector<std::string>& input_collector, std::unordered_map<uint64_t, Entity>& player_map, bool& game_running);

        void send(int id, std::vector<uint8_t>& body);
    };

    TCP_client::TCP_client(unsigned short int target_port) : resolver(io_context), socket(io_context) { // connects the socket to the server
        auto endpoint = resolver.resolve("127.0.0.1", std::to_string(target_port));
        asio::connect(socket, endpoint);
    }

    void TCP_client::receive_loop(std::vector<std::string>& input_collector, std::unordered_map<uint64_t, Entity>& player_map, bool& game_running) { // recieves incoming packets and does things with them based on header contents
        if(!game_running) socket.close();
        else {
            std::shared_ptr<std::array<char, sizeof(packet_header)>> buffer(new std::array<char, sizeof(packet_header)>);
            socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
                [this, buffer, &input_collector, &player_map, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                    if(error_code) {
                        std::cout << "Error: " << error_code.message() << "\n";
                    } else {
                        packet_header header;
                        memcpy(&header, buffer->data(), bytes_transferred);

                        std::shared_ptr<std::vector<uint8_t>> buffer_body(new std::vector<uint8_t>(header.size_bytes));
                        socket.async_read_some(asio::buffer(buffer_body->data(), buffer_body->size()), 
                            [this, header_type = header.type, buffer_body, &input_collector, &player_map, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                                if(error_code) {
                                    std::cout << "Error: " << error_code.message() << "\n";
                                } else { // insert text into input collector to be processed by client
                                    switch(header_type) {
                                        case 0: { // insert text into input collector to be processed by client
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (chat message)\n";
                                            std::string message = make_string(reinterpret_cast<char*>(buffer_body->data()), bytes_transferred);
                                            input_collector.push_back(message);

                                            break;
                                        }
                                        case 1: { // player joined
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (player joined)\n";
                                            player_join_packet_toclient recv_packet = from_byte_vector<player_join_packet_toclient>(buffer_body->data());

                                            text_struct txt_struct;
                                            txt_struct.set_values(make_string_array(recv_packet.name.data(), 256), 0x10000, 1);
                                            player_map.emplace(recv_packet.entity_packet.entity_id, Entity(recv_packet.entity_packet.entity_id, bandit1_spritesheet, txt_struct, recv_packet.entity_packet.position, recv_packet.entity_packet.direction));

                                            break;
                                        }
                                        case 2: { // player position packets
                                            //std::cout << "Recieved " << bytes_transferred << " bytes from the server. (player position)\n";

                                            entity_movement_packet_toclient recv_packet = from_byte_vector<entity_movement_packet_toclient>(buffer_body->data());
                                            /*if(player_map.contains(recv_packet.entity_id)) {
                                                player_map.at(recv_packet.entity_id).position = recv_packet.position;
                                                player_map.at(recv_packet.entity_id).active_sprite[0] = recv_packet.direction;
                                            }*/
                                            if(player_map.contains(recv_packet.entity_id)) player_map.at(recv_packet.entity_id).insert_position(recv_packet.position, recv_packet.direction);

                                            break;
                                        }
                                        default: { // the packet recieved did not have a valid header type if it reaches the default statement
                                            std::cout << "Recieved a packet with an invalid header type.\n";
                                        }
                                    }
                                    receive_loop(input_collector, player_map, game_running);
                                }
                            }
                        );
                    }
                }
            );
        }
    }

    void TCP_client::start(std::vector<std::string>& input_collector, std::unordered_map<uint64_t, Entity>& player_map, bool& game_running) { // starts the recieve loop
        recieve_thread = std::thread(
            [this, &input_collector, &player_map, &game_running]() {
                this->receive_loop(input_collector, player_map, game_running);
                io_context.run();
            }
        );
    }

    void TCP_client::send(int id, std::vector<uint8_t>& body) { // sends packet to server
        packet_header header = {id, body.size()};
        std::vector<uint8_t> packet_buffer(sizeof(packet_header) + body.size());
        memcpy(packet_buffer.data(), &header, sizeof(packet_header));
        memcpy(packet_buffer.data() + sizeof(packet_header), body.data(), body.size());
        socket.async_write_some(asio::buffer(packet_buffer.data(), packet_buffer.size()), 
            [](const asio::error_code& error_code, size_t bytes) mutable {
                if(error_code) {
                    std::cout << "Failed to send message to the server.\n";
                } /*else {
                    std::cout << "Sent " << bytes << " bytes of data to the server.\n";
                }*/
            }
        );
    };
}