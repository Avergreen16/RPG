#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <ctime>
#include <thread>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

#include "asio_packets.cpp"

namespace netwk {
    using asio::ip::tcp;

    struct Server_entity {
        uint64_t id;
        std::array<double, 2> position;
        directions direction_facing;
    };

    std::unordered_map<uint64_t, Server_entity> entity_map;
    uint64_t entity_id_counter = 0;

    struct TCP_server;

    struct TCP_connection {
        tcp::socket socket;

        std::string message = "This is client ";
        uint client_id;
        uint64_t associated_entity_id;
        bool break_connection = false;
        std::string player_name;
        std::thread send_loop_thread;

        TCP_connection(asio::io_context& io_context, uint client_id);

        void start(TCP_server* parent_server);

        void recieve_loop(TCP_server* parent_server);
        
        void send_loop();

        static std::shared_ptr<TCP_connection> create(asio::io_context& io_context, uint client_id);
    };

    struct connection_thread {
        std::shared_ptr<TCP_connection> connection;
        std::thread thread;
    };

    struct TCP_server {
        uint total_connections = 0;
        uint connection_id_counter = 0;

        uint16_t port;
        asio::io_context io_context;
        tcp::acceptor acceptor;
        std::unordered_map<uint, connection_thread> connection_map;

        TCP_server(uint16_t port);

        void start_accept();

        int run();

        void broadcast(uint id, std::vector<uint8_t> body);

        void broadcast(uint id, std::vector<uint8_t> body, uint origin_client);
    };

    TCP_connection::TCP_connection(asio::io_context& io_context, uint client_id) : socket(io_context) {
        message += std::to_string(client_id) + "\n";
        this->client_id = client_id;
    }

    void TCP_connection::start(TCP_server* parent_server) {
        recieve_loop(parent_server);
        send_loop();
        while(true) {
            if(break_connection) {
                socket.close();
                break;
            }
        }
    }

    void TCP_connection::recieve_loop(TCP_server* parent_server) {
        std::shared_ptr<std::array<char, sizeof(packet_header)>> buffer(new std::array<char, sizeof(packet_header)>);
        socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
            [this, buffer, parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                if(error_code == asio::error::eof) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected (no error).\n";
                    std::string message = this->player_name + " \\cfffquit the game.";
                    std::vector<uint8_t> body(message.size());
                    memcpy(body.data(), message.data(), message.size());
                    parent_server->broadcast(0, body, this->client_id);
                    break_connection = true;
                } else if(error_code) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";
                    std::string message = this->player_name + " \\cfffquit the game.";
                    std::vector<uint8_t> body(message.size());
                    memcpy(body.data(), message.data(), message.size());
                    parent_server->broadcast(0, body, this->client_id);
                    break_connection = true;
                } else {
                    packet_header header;
                    memcpy(&header, buffer->data(), bytes_transferred);

                    std::shared_ptr<std::vector<uint8_t>> buffer_body(new std::vector<uint8_t>(header.size_bytes));
                    this->socket.async_read_some(asio::buffer(buffer_body->data(), buffer_body->size()), 
                        [this, header_type = header.type, buffer_body, parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                            if(error_code) { // if there is an error, disconnect player and tell other clients
                                std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";

                                std::string message = this->player_name + " \\cfffquit the game.";
                                std::vector<uint8_t> body(message.size());
                                memcpy(body.data(), message.data(), message.size());
                                parent_server->broadcast(0, body, this->client_id);

                                break_connection = true;
                            } else { // if there isn't an error, do things with the recieved data depending on what the header's id is
                                switch(header_type) {
                                    case 0: { // player logged in
                                        std::cout << "Recieved " << bytes_transferred << " bytes from client " + std::to_string(client_id) + ". (player logged in)\n";
                                        player_join_packet_toserv recv_packet = from_byte_vector<player_join_packet_toserv>(buffer_body->data());

                                        this->player_name = make_string_array(recv_packet.name.data(), 64);
                                        this->associated_entity_id = entity_id_counter++;
                                        entity_map.emplace(this->associated_entity_id, Server_entity{this->associated_entity_id, recv_packet.entity_packet.position, recv_packet.entity_packet.direction});

                                        player_join_packet_toclient send_packet{recv_packet.name, {this->associated_entity_id, recv_packet.entity_packet.position, recv_packet.entity_packet.direction}};
                                        std::vector<uint8_t> join_packet_body = to_byte_vector(send_packet);
                                        parent_server->broadcast(1, join_packet_body, this->client_id);

                                        std::string message = this->player_name + " \\cfffjoined the game.";
                                        std::vector<uint8_t> body(message.size());
                                        memcpy(body.data(), message.data(), message.size());
                                        parent_server->broadcast(0, body);
                                        break;
                                    }
                                    case 1: { // chat message
                                        std::cout << "Recieved " << bytes_transferred << " bytes from client " + std::to_string(client_id) + ". (chat message)\n";
                                        std::string message = this->player_name + " \\cfff: " + make_string(reinterpret_cast<char*>(buffer_body->data()), bytes_transferred);
                                        std::vector<uint8_t> body(message.size());
                                        memcpy(body.data(), message.data(), message.size());
                                        parent_server->broadcast(0, body);
                                        break;
                                    }
                                    case 2: { // player position
                                        //std::cout << "Recieved " << bytes_transferred << " bytes from client " + std::to_string(client_id) + ". (player position)\n";
                                        entity_movement_packet_toserv recv_packet = from_byte_vector<entity_movement_packet_toserv>(buffer_body->data());
                                        entity_map[this->associated_entity_id].position = recv_packet.position;
                                        entity_map[this->associated_entity_id].direction_facing = recv_packet.direction;
                                        break;
                                    }
                                    default: { // the header on the packet didn't match any header id
                                        std::cout << "Recieved a packet with an invalid header type from client " + std::to_string(this->client_id) + "\n";
                                    }
                                }
                                recieve_loop(parent_server);
                            }
                        }
                    );
                }
            }
        );
    }

    void TCP_connection::send_loop() {
        send_loop_thread = std::thread(
            [this]() {
                time_t time_container = clock();
                while(this->socket.is_open()) {
                    time_t time_tick = clock();
                    if(time_tick - time_container >= 40) {
                        time_container = time_tick;

                        for(auto& [id, entity] : entity_map) {
                            if(id != this->associated_entity_id) {
                                packet<entity_movement_packet_toclient> entity_packet(2, {id, entity.position, entity.direction_facing});
                                std::vector<uint8_t> send_vector = to_byte_vector<packet<entity_movement_packet_toclient>>(entity_packet);
                                this->socket.async_send(asio::buffer(send_vector.data(), send_vector.size()), 
                                    [this](const asio::error_code& error_code, size_t bytes_transferred) {
                                        if(error_code) {
                                            std::cout << "Failed to send message to client " << this->client_id << ", error: " << error_code.message() << "\n";
                                        }
                                        //std::cout << "Sent " << bytes_transferred << " bytes of data to client " + std::to_string(this->client_id) + ". (entity data)\n";
                                    }
                                );
                            }
                        }
                    }
                }
            }
        );
    }

    /*void TCP_connection::send_loop() {
        if(break_connection) break;
        if(clock() - time_container >= 4000) {
            asio::async_write(socket, asio::buffer(message), 
                [this](const asio::error_code& error, size_t bytes) {
                    if(error) {
                        std::cout << "Failed to send message to client " + std::to_string(client_id) + ".\n";
                    } else {
                        std::cout << "Sent " << bytes << " bytes of data to client " + std::to_string(client_id) + ".\n";
                    }
                }
            );
            time_container = clock();
        }
    }*/

    std::shared_ptr<TCP_connection> TCP_connection::create(asio::io_context& io_context, uint client_id) {
        return std::shared_ptr<TCP_connection>(new TCP_connection(io_context, client_id));
    }

    TCP_server::TCP_server(uint16_t port) : acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
        this->port = port;
    }

    void TCP_server::start_accept() {
        // create connection
        auto connection = TCP_connection::create(io_context, connection_id_counter);
        

        // asynchronously accept connection
        acceptor.async_accept(connection->socket,
            [this, connection](const asio::error_code& error) {
                if(!error) {
                    std::cout << "Connection " + std::to_string(connection_id_counter) + " accepted.\n";
                    this->connection_map.insert({connection_id_counter, connection_thread{connection, std::thread(
                        [this, connection]() {
                            connection->start(this);
                        }
                    )}});
                    ++total_connections;
                    ++connection_id_counter;
                }

                start_accept();
            }
        );
    }

    int TCP_server::run() {
        try {
            std::cout << "Ready to accept connections.\n";
            start_accept();
            io_context.run();
        } catch(std::exception& e) {
            std::cout << e.what() << std::endl;
            return -1;
        }
        
        return 0;
    }

    void TCP_server::broadcast(uint id, std::vector<uint8_t> body) {
        packet_header header(id, body.size());
        std::vector<std::byte> packet_buffer(sizeof(packet_header) + body.size());
        memcpy(packet_buffer.data(), &header, sizeof(packet_header));
        memcpy(packet_buffer.data() + sizeof(packet_header), body.data(), body.size());
        for(auto& [key, c_thread] : connection_map) {
            if(c_thread.connection->socket.is_open()) {
                async_write(c_thread.connection->socket, asio::buffer(packet_buffer.data(), packet_buffer.size()), 
                    [key](const asio::error_code& error, size_t bytes) {
                        if(error) {
                            std::cout << "Failed to send message to client " + std::to_string(key) + ".\n";
                        } else {
                            std::cout << "Sent " << bytes << " bytes of data to client " + std::to_string(key) + ".\n";
                        }
                    }
                );
            }
        }
    }

    void TCP_server::broadcast(uint id, std::vector<uint8_t> body, uint origin_client) {
        packet_header header(id, body.size());
        std::vector<std::byte> packet_buffer(sizeof(packet_header) + body.size());
        memcpy(packet_buffer.data(), &header, sizeof(packet_header));
        memcpy(packet_buffer.data() + sizeof(packet_header), body.data(), body.size());
        for(auto& [key, c_thread] : connection_map) {
            if(key != origin_client && c_thread.connection->socket.is_open()) {
                async_write(c_thread.connection->socket, asio::buffer(packet_buffer.data(), packet_buffer.size()), 
                    [key](const asio::error_code& error, size_t bytes) {
                        if(error) {
                            std::cout << "Failed to send message to client " + std::to_string(key) + ".\n";
                        } else {
                            std::cout << "Sent " << bytes << " bytes of data to client " + std::to_string(key) + ".\n";
                        }
                    }
                );
            }
        }
    }
}