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
#include "worldgen.cpp"

const uint max_sends_per_sec = 20;
const uint max_ticks_per_sec = 100;

const uint sends_mil = 1000000000 / max_sends_per_sec;
const uint ticks_mil = 1000000000 / max_ticks_per_sec;

time_t __attribute__((always_inline)) get_time() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

double get_8dir_distance(std::array<double, 2> pos_0, std::array<double, 2> pos_1) {
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

        bool target_aquired = false;
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
                    directions dir = OUT_OF_RANGE;
                    if(entity_map.contains(target_id)) {
                        dir = Astar_pathfinding({7, 7}, {int(entity_map[target_id].position[0] - position[0]) + 7, int(entity_map[target_id].position[1] - position[1]) + 7}, empty_9x9_array);
                    }
                    if(!(dir == NONE || dir == OUT_OF_RANGE)) {
                        target_aquired = true;
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
                        if(dir == OUT_OF_RANGE) {
                            target_aquired = false;
                        } else {
                            target_aquired = true;
                        }
                        state = IDLE;
                    }
                }
            } else {
                pathfind_bool = true;
            }
        }

        void tick(uint delta) {
            if(target_aquired == false) {
                uint64_t id_container = UINT64_MAX;
                double distance_container = __DBL_MAX__;
                for(auto& [id, entity] : entity_map) {
                    double distance_to_entity = get_8dir_distance(position, entity.position);
                    if(distance_to_entity < distance_container) {
                        id_container = id;
                        distance_container = distance_to_entity;
                    }
                }
                target_id = id_container;
            }
            
            pathfind();

            if(state != IDLE) {
                double change_x = 0.0;
                double change_y = 0.0;
                
                switch(direction_moving) {
                    case NORTH:
                        change_y = 0.000000006 * delta;
                        break;
                    case NORTHEAST:
                        change_x = 0.000000004243 * delta;
                        change_y = 0.000000004243 * delta;
                        break;
                    case EAST:
                        change_x = 0.000000006 * delta;
                        break;
                    case SOUTHEAST:
                        change_x = 0.000000004243 * delta;
                        change_y = -0.000000004243 * delta;
                        break;
                    case SOUTH:
                        change_y = -0.000000006 * delta;
                        break;
                    case SOUTHWEST:
                        change_x = -0.000000004243 * delta;
                        change_y = -0.000000004243 * delta;
                        break;
                    case WEST:
                        change_x = -0.000000006 * delta;
                        break;
                    case NORTHWEST:
                        change_x = -0.000000004243 * delta;
                        change_y = 0.000000004243 * delta;
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
        TCP_server* parent_server;
        tcp::socket socket;

        std::string message = "This is client ";
        uint client_id;
        uint64_t associated_entity_id;
        bool break_connection = false;
        std::string player_name;
        std::thread send_loop_thread;

        TCP_connection(asio::io_context& io_context, uint client_id, TCP_server* parent_server);

        void start();

        void recieve_loop();
        
        void send_loop();

        void send(std::vector<uint8_t> packet);

        static std::shared_ptr<TCP_connection> create(asio::io_context& io_context, uint client_id, TCP_server* parent_server);
    };

    struct connection_thread {
        std::shared_ptr<TCP_connection> connection;
        std::thread thread;
    };

    struct TCP_server {
        std::unordered_map<uint, Chunk_data> loaded_chunks;

        uint total_connections = 0;
        uint connection_id_counter = 0;

        uint16_t port;
        asio::io_context io_context;
        tcp::acceptor acceptor;
        std::unordered_map<uint, connection_thread> connection_map;

        std::queue<std::pair<uint, uint>> chunk_requests;
        std::thread chunk_gen_thread;

        TCP_server(uint16_t port);

        void start_accept();

        int run();

        void broadcast(uint id, std::vector<uint8_t> body);

        void broadcast(uint id, std::vector<uint8_t> body, uint origin_client);
    };

    TCP_connection::TCP_connection(asio::io_context& io_context, uint client_id, TCP_server* parent_server) : socket(io_context) {
        this->parent_server = parent_server;
        message += std::to_string(client_id) + "\n";
        this->client_id = client_id;
    }

    void TCP_connection::start() {
        recieve_loop();
        send_loop();
        while(true) {
            if(break_connection) {
                socket.close();
                break;
            }
        }
    }

    void TCP_connection::recieve_loop() {
        std::shared_ptr<std::array<char, sizeof(packet_header)>> buffer(new std::array<char, sizeof(packet_header)>);
        socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
            [this, buffer](const asio::error_code& error_code, size_t bytes_transferred) {
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
                        [this, header_id = header.id, buffer_body](const asio::error_code& error_code, size_t bytes_transferred) {
                            if(error_code) { // if there is an error, disconnect player and tell other clients
                                std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";

                                std::string message = this->player_name + " \\cfffquit the game.";
                                std::vector<uint8_t> body(message.size());
                                memcpy(body.data(), message.data(), message.size());
                                parent_server->broadcast(0, body, this->client_id);

                                break_connection = true;
                            } else { // if there isn't an error, do things with the recieved data depending on what the header's id is
                                recieve_loop(); // primes next recieve, this does NOT stall the program waiting for data to be recieved

                                switch(header_id) {
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
                                    case 3: { // chunk request
                                        std::cout << "Recieved " << bytes_transferred << " bytes from client " + std::to_string(client_id) + ". (chunk request)\n";
                                        std::vector<uint> request_vector(bytes_transferred / sizeof(uint));
                                        memcpy(request_vector.data(), buffer_body->data(), bytes_transferred);

                                        for(uint chunk_key : request_vector) {
                                            parent_server->chunk_requests.push({client_id, chunk_key});
                                        }

                                        break;
                                    }
                                    default: { // the header on the packet didn't match any header id
                                        std::cout << "Recieved a packet with an invalid header type from client " + std::to_string(this->client_id) + ": " << header_id << "\n";
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
                    if(time_tick - time_container >= sends_mil) {
                        time_container = time_tick;

                        for(auto& [id, entity] : entity_map) {
                            if(id != this->associated_entity_id) {
                                entity_movement_packet_toclient entity_packet{id, entity.position, entity.direction_facing};
                                send(make_packet(2, (void*)&entity_packet, sizeof(entity_packet)));
                            }
                        }

                        for(auto& [id, enemy] : enemy_map) {
                            entity_movement_packet_toclient entity_packet{id, enemy.position, enemy.direction_facing};
                            send(make_packet(2, (void*)&entity_packet, sizeof(entity_packet)));
                        }
                    }
                }
            }
        );
    }

    void TCP_connection::send(std::vector<uint8_t> packet) {
        this->socket.async_send(asio::buffer(packet.data(), packet.size()), 
            [this](const asio::error_code& error_code, size_t bytes_transferred) {
                if(error_code) {
                    std::cout << "Failed to send message to client " << this->client_id << ", error: " << error_code.message() << "\n";
                }
            }
        );
    }

    std::shared_ptr<TCP_connection> TCP_connection::create(asio::io_context& io_context, uint client_id, TCP_server* parent_server) {
        return std::shared_ptr<TCP_connection>(new TCP_connection(io_context, client_id, parent_server));
    }

    TCP_server::TCP_server(uint16_t port) : acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
        this->port = port;

        chunk_gen_thread = std::thread(
            [this]() {
                Worldgen worldgen(0.7707326, 6, 3, 7, 1.5, 4, 4, 3, 1.5, 4, 4, 4, 1.3);

                while(true) {
                    if(chunk_requests.size() != 0) {
                        std::pair<uint, uint> client_id_chunk_key = chunk_requests.front();
                        chunk_requests.pop();
                        if(!loaded_chunks.contains(client_id_chunk_key.second)) {
                            insert_chunk(loaded_chunks, world_size_chunks, client_id_chunk_key.second, worldgen);
                        }

                        std::pair<uint, Chunk_data> key_data_pair = {client_id_chunk_key.second, loaded_chunks[client_id_chunk_key.second]};
                        connection_map[client_id_chunk_key.first].connection->send(make_packet(3, &key_data_pair, sizeof(key_data_pair)));
                    }
                }
            }
        );
    }

    void TCP_server::start_accept() {
        // create connection
        auto connection = TCP_connection::create(io_context, connection_id_counter, this);
        

        // asynchronously accept connection
        acceptor.async_accept(connection->socket,
            [this, connection](const asio::error_code& error) {
                if(!error) {
                    std::cout << "Connection " + std::to_string(connection_id_counter) + " accepted.\n";
                    this->connection_map.insert({connection_id_counter, connection_thread{connection, std::thread(
                        [this, connection]() {
                            connection->start();
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