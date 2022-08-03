#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <ctime>
#include <thread>
#include <chrono>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

#include "asio_packets.cpp"
#include "pathfinding.cpp"

time_t __attribute__((always_inline)) get_time() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

double get_tiled_distance(std::array<double, 2> pos_0, std::array<double, 2> pos_1) {
    double dist_x = abs(pos_0[0] - pos_1[0]);
    double dist_y = abs(pos_0[1] - pos_1[1]);
    return (dist_x + dist_y) - 0.6 * std::min(dist_x, dist_y);
}

double get_mht_distance(std::array<double, 2> pos_0, std::array<double, 2> pos_1) {
    return abs(pos_0[0] - pos_1[0]) + abs(pos_0[1] - pos_1[1]);
}

namespace netwk {
    using asio::ip::tcp;

    struct Server_entity {
        uint64_t id;
        std::array<double, 2> position;
        directions direction_facing;
    };

    std::unordered_map<uint64_t, Server_entity> entity_map;

    struct Server_enemy {
        std::array<double, 2> position = {13150 + 0.5, 8110 + 0.5};

        states state = IDLE;
        directions direction_moving = SOUTH;
        directions direction_facing = SOUTH;

        bool pathfind_bool = true;

        uint64_t target_id = UINT64_MAX;

        Server_enemy(std::array<double, 2> position, directions direction_facing) {
            this->position = position;
            this->direction_facing = direction_facing;
        }

        void pathfind() {
            double x_within_tile = position[0] - int(position[0]);
            double y_within_tile = position[1] - int(position[1]);
            if(x_within_tile > 0.45 && x_within_tile < 0.55 && y_within_tile > 0.45 && y_within_tile < 0.55) {
                if(pathfind_bool) {
                    directions dir = NONE;
                    if(entity_map.contains(target_id)) {
                        directions dir = Astar_pathfinding({7, 7}, {int(entity_map[target_id].position[0] - position[0]) + 7, int(entity_map[target_id].position[1] - position[1]) + 7}, empty_9x9_array);
                    }
                    if(dir != NONE) {
                        pathfind_bool = false;
                        state = WALKING;
                        direction_moving = dir;
                        switch(dir) {
                            case NORTH:
                                direction_facing = NORTH;
                                break;

                            case NORTHEAST:
                            case SOUTHEAST:
                            case EAST:
                                direction_facing = EAST;
                                break;
                            
                            case SOUTH:
                                direction_facing = SOUTH;
                                break;

                            case NORTHWEST:
                            case SOUTHWEST:
                            case WEST:
                                direction_facing = WEST;
                                break;
                        }
                    } else {
                        state = IDLE;
                    }
                }
            } else {
                pathfind_bool = true;
            }
        }

        void tick(uint delta) {
            if(target_id == UINT64_MAX || !entity_map.contains(target_id) || get_mht_distance(position, entity_map[target_id].position) > 8) {
                uint64_t id_container = UINT64_MAX;
                double distance_container;
                for(auto& [id, entity] : entity_map) {
                    if(id_container != UINT64_MAX) {
                        double distance_to_entity = get_tiled_distance(position, entity.position);
                        if(distance_to_entity > distance_container) {
                            id_container = id;
                            distance_container = distance_to_entity;
                        }
                    } else {
                        id_container = id;
                        distance_container = get_tiled_distance(position, entity.position);
                    }
                }
                target_id = id_container;
            }
            
            pathfind();

            if(state != IDLE) {
                std::cout << delta << "\n";
                double change_x = 0.0;
                double change_y = 0.0;
                
                switch(direction_moving) {
                    case NORTH:
                        change_y = 0.006 * delta;
                        break;
                    case NORTHEAST:
                        change_x = 0.004243 * delta;
                        change_y = 0.004243 * delta;
                        break;
                    case EAST:
                        change_x = 0.006 * delta;
                        break;
                    case SOUTHEAST:
                        change_x = 0.004243 * delta;
                        change_y = -0.004243 * delta;
                        break;
                    case SOUTH:
                        change_y = -0.006 * delta;
                        break;
                    case SOUTHWEST:
                        change_x = -0.004243 * delta;
                        change_y = -0.004243 * delta;
                        break;
                    case WEST:
                        change_x = -0.006 * delta;
                        break;
                    case NORTHWEST:
                        change_x = -0.004243 * delta;
                        change_y = 0.004243 * delta;
                        break;
                }

                if(state == WALKING) {
                    position[0] += change_x * 0.8;
                    position[1] += change_y * 0.8;
                } else if(state == RUNNING) {
                    position[0] += change_x * 1.28;
                    position[1] += change_y * 1.28;
                }
            }
        }
    };

    std::unordered_map<uint64_t, Server_enemy> enemy_map;
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

        void start(TCP_server& parent_server);

        void recieve_loop(TCP_server& parent_server);
        
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

    void TCP_connection::start(TCP_server& parent_server) {
        recieve_loop(parent_server);
        send_loop();
        while(true) {
            if(break_connection) {
                socket.close();
                break;
            }
        }
    }

    void TCP_connection::recieve_loop(TCP_server& parent_server) {
        std::shared_ptr<std::array<char, sizeof(packet_header)>> buffer(new std::array<char, sizeof(packet_header)>);
        socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
            [this, buffer, &parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                if(error_code == asio::error::eof) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected (no error).\n";
                    std::string message = this->player_name + " \\cfffquit the game.";
                    std::vector<uint8_t> body(message.size());
                    memcpy(body.data(), message.data(), message.size());
                    parent_server.broadcast(0, body, this->client_id);
                    break_connection = true;
                } else if(error_code) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";
                    std::string message = this->player_name + " \\cfffquit the game.";
                    std::vector<uint8_t> body(message.size());
                    memcpy(body.data(), message.data(), message.size());
                    parent_server.broadcast(0, body, this->client_id);
                    break_connection = true;
                } else {
                    packet_header header;
                    memcpy(&header, buffer->data(), bytes_transferred);

                    std::shared_ptr<std::vector<uint8_t>> buffer_body(new std::vector<uint8_t>(header.size_bytes));
                    this->socket.async_read_some(asio::buffer(buffer_body->data(), buffer_body->size()), 
                        [this, header_type = header.type, buffer_body, &parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                            if(error_code) { // if there is an error, disconnect player and tell other clients
                                std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";

                                std::string message = this->player_name + " \\cfffquit the game.";
                                std::vector<uint8_t> body(message.size());
                                memcpy(body.data(), message.data(), message.size());
                                parent_server.broadcast(0, body, this->client_id);

                                break_connection = true;
                            } else { // if there isn't an error, do things with the recieved data depending on what the header's id is
                                recieve_loop(parent_server); // primes next recieve, this does NOT stall the program waiting for data to be recieved
                                switch(header_type) {
                                    case 0: { // player logged in
                                        std::cout << "Recieved " << bytes_transferred << " bytes from client " + std::to_string(client_id) + ". (player logged in)\n";
                                        player_join_packet_toserv recv_packet = from_byte_vector<player_join_packet_toserv>(buffer_body->data());

                                        this->player_name = make_string_array(recv_packet.name.data(), 64);
                                        this->associated_entity_id = entity_id_counter++;
                                        entity_map.emplace(this->associated_entity_id, Server_entity{this->associated_entity_id, recv_packet.entity_packet.position, recv_packet.entity_packet.direction});

                                        player_join_packet_toclient send_packet{recv_packet.name, {this->associated_entity_id, recv_packet.entity_packet.position, recv_packet.entity_packet.direction}};
                                        std::vector<uint8_t> join_packet_body = to_byte_vector(send_packet);
                                        parent_server.broadcast(1, join_packet_body, this->client_id);

                                        std::string message = this->player_name + " \\cfffjoined the game.";
                                        std::vector<uint8_t> body(message.size());
                                        memcpy(body.data(), message.data(), message.size());
                                        parent_server.broadcast(0, body);
                                        break;
                                    }
                                    case 1: { // chat message
                                        std::cout << "Recieved " << bytes_transferred << " bytes from client " + std::to_string(client_id) + ". (chat message)\n";
                                        std::string message = this->player_name + " \\cfff: " + make_string(reinterpret_cast<char*>(buffer_body->data()), bytes_transferred);
                                        std::vector<uint8_t> body(message.size());
                                        memcpy(body.data(), message.data(), message.size());
                                        parent_server.broadcast(0, body);
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
                time_t time_container = get_time();
                while(this->socket.is_open()) {
                    time_t time_tick = get_time();
                    if(time_tick - time_container >= 40000000) {
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
                                    }
                                );
                            }
                        }

                        for(auto& [id, enemy] : enemy_map) {
                            packet<entity_movement_packet_toclient> entity_packet(2, {id, enemy.position, enemy.direction_facing});
                            std::vector<uint8_t> send_vector = to_byte_vector<packet<entity_movement_packet_toclient>>(entity_packet);
                            this->socket.async_send(asio::buffer(send_vector.data(), send_vector.size()), 
                                [this](const asio::error_code& error_code, size_t bytes_transferred) {
                                    if(error_code) {
                                        std::cout << "Failed to send message to client " << this->client_id << ", error: " << error_code.message() << "\n";
                                    }
                                }
                            );
                        }
                    }
                }
            }
        );
    }

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
                            connection->start(*this);
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