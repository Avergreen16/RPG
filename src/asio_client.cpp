#pragma once

#include "asio_client.hpp"
#include "client_core.hpp"

namespace netwk {
    TCP_client::TCP_client(unsigned short int target_port) : resolver(io_context), socket(io_context) { // connects the socket to the server
        auto endpoint = resolver.resolve("192.168.1.6", std::to_string(target_port));
        asio::connect(socket, endpoint);
    }

    void TCP_client::receive_loop(Core& core, bool& game_running) { // recieves incoming packets and does things with them based on header contents
        if(!game_running) socket.close();
        else {
            std::shared_ptr<std::array<uint8_t, sizeof(packet_header)>> buffer(new std::array<uint8_t, sizeof(packet_header)>);
            socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
                [this, buffer, &core, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                    if(error_code) {
                        std::cout << "Error: " << error_code.message() << "\n";
                    } else {
                        packet_header header;
                        memcpy(&header, buffer->data(), bytes_transferred);

                        std::shared_ptr<std::vector<uint8_t>> buffer_body(new std::vector<uint8_t>(header.size_bytes));
                        socket.async_read_some(asio::buffer(buffer_body->data(), buffer_body->size()), 
                            [this, header_id = header.id, buffer_body, &core, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                                if(error_code) {
                                    std::cout << "Error: " << error_code.message() << "\n";
                                } else { // insert text into input collector to be processed by client     
                                    receive_loop(core, game_running); // primes next recieve, this does NOT stall while waiting for new data from the server

                                    switch(header_id) {
                                        case 0: { // insert text into input collector to be processed by client
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (chat message)\n";
                                            std::string message = make_string(reinterpret_cast<char*>(buffer_body->data()), bytes_transferred);
                                            core.insert_chat_message(message);

                                            break;
                                        }
                                        case 1: { // player joined
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (player joined)\n";
                                            player_join_packet_toclient recv_packet = from_byte_vector<player_join_packet_toclient>(buffer_body->data());

                                            text_struct txt_struct;
                                            txt_struct.set_values(make_string_array(recv_packet.name.data(), 256), 0x10000, 1);
                                            core.entity_map.emplace(recv_packet.entity_packet.entity_id, Entity(recv_packet.entity_packet.entity_id, core.texture_map[2].id, txt_struct, recv_packet.entity_packet.position, recv_packet.entity_packet.direction, core.loaded_chunks));

                                            break;
                                        }
                                        case 2: { // player position packets
                                            //std::cout << "Recieved " << bytes_transferred << " bytes from the server. (player position)\n";

                                            entity_movement_packet_toclient recv_packet = from_byte_vector<entity_movement_packet_toclient>(buffer_body->data());
                                            if(core.entity_map.contains(recv_packet.entity_id)) core.entity_map.at(recv_packet.entity_id).insert_position(recv_packet);
                                            else {
                                                core.entity_map.emplace(recv_packet.entity_id, Entity(recv_packet.entity_id, core.texture_map[2].id, text_struct{}, recv_packet.position, recv_packet.direction, core.loaded_chunks));
                                            }

                                            break;
                                        }
                                        case 3: { // chunk data and key
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (chunk data)\n";
                                            std::vector<std::pair<uint, Server_chunk>> chunk_vector(bytes_transferred / sizeof(std::pair<uint, Server_chunk>));
                                            memcpy(chunk_vector.data(), buffer_body->data(), bytes_transferred);

                                            for(auto& [key, chunk] : chunk_vector) {
                                                core.loaded_chunks.insert({key, Chunk_data(chunk)});
                                                //update_chunk_water(core.loaded_chunks, world_size_chunks, chunk_key_pair.first);
                                            }

                                            break;
                                        }
                                        default: { // the packet recieved did not have a valid header type if it reaches the default statement
                                            std::cout << "Recieved a packet with an invalid header type.\n";
                                        }
                                    }
                                }
                            }
                        );
                    }
                }
            );
        }
    }

    void TCP_client::start(Core& core, bool& game_running) { // starts the recieve loop
        recieve_thread = std::thread(
            [this, &core, &game_running]() {
                this->receive_loop(core, game_running);
                io_context.run();
            }
        );
    }

    void TCP_client::send(std::vector<uint8_t> packet) { // sends packet to server
        socket.async_write_some(asio::buffer(packet.data(), packet.size()), 
            [](const asio::error_code& error_code, size_t bytes) mutable {
                if(error_code) {
                    std::cout << "Failed to send message to the server.\n";
                }
            }
        );
    };
}