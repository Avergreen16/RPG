#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <ctime>
#include <thread>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

namespace netwk {
    using asio::ip::tcp;

    struct packet_header {
        uint16_t type;
        uint32_t size_bytes;
    };

    struct packet {
        packet_header header;

        std::vector<char> body;
    };

    struct TCP_server;

    struct TCP_connection {
        tcp::socket socket;

        std::string message = "This is client ";
        unsigned int client_id;
        bool break_connection = false;
        std::string player_name;

        TCP_connection(asio::io_context& io_context, unsigned int client_id);

        void start(TCP_server* parent_server);

        void recieve_loop(TCP_server* parent_server);
        
        void send_loop();

        static std::shared_ptr<TCP_connection> create(asio::io_context& io_context, unsigned int client_id);
    };

    struct connection_thread {
        std::shared_ptr<TCP_connection> connection;
        std::thread thread;
    };

    struct TCP_server {
        unsigned int total_connections = 0;
        unsigned int connection_id_counter = 0;

        unsigned short int port;
        asio::io_context io_context;
        tcp::acceptor acceptor;
        std::unordered_map<unsigned int, connection_thread> connection_map;

        TCP_server(unsigned short int port);

        void start_accept();

        int run();

        void broadcast(std::string message);
    };

    TCP_connection::TCP_connection(asio::io_context& io_context, unsigned int client_id) : socket(io_context) {
        message += std::to_string(client_id) + "\n";
        this->client_id = client_id;
    }

    void TCP_connection::start(TCP_server* parent_server) {
        recieve_loop(parent_server);
        while(true) {
            if(break_connection) {
                socket.close();
                break;
            }
        }
    }

    void TCP_connection::recieve_loop(TCP_server* parent_server) {
        std::array<char, sizeof(packet_header)>* buffer = new std::array<char, sizeof(packet_header)>;
        socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
            [this, buffer, parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                if(error_code == asio::error::eof) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected (no error).\n";
                    break_connection = true;
                } else if(error_code) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";
                    break_connection = true;
                } else {
                    packet_header header;
                    memcpy(&header, buffer->data(), bytes_transferred);

                    if(header.type == 0) {
                        std::vector<char>* buffer_1 = new std::vector<char>(header.size_bytes);
                        socket.async_read_some(asio::buffer(buffer_1->data(), buffer_1->size()), 
                            [this, buffer_1, parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                                if(error_code) {
                                    std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";
                                    break_connection = true;
                                } else {
                                    this->player_name = std::string(buffer_1->data());
                                    delete buffer_1;
                                    parent_server->broadcast(this->player_name + "\\c000 joined the game.");
                                    recieve_loop(parent_server);
                                }
                            }
                        );
                    } else {
                        std::vector<char>* buffer_1 = new std::vector<char>(header.size_bytes);
                        socket.async_read_some(asio::buffer(buffer_1->data(), buffer_1->size()), 
                            [this, buffer_1, parent_server](const asio::error_code& error_code, size_t bytes_transferred) {
                                if(error_code) {
                                    std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";
                                    break_connection = true;
                                } else {
                                    parent_server->broadcast(this->player_name + " \\c000: " + buffer_1->data());
                                    delete buffer_1;
                                    recieve_loop(parent_server);
                                }
                            }
                        );
                    }
                    delete buffer;
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

    std::shared_ptr<TCP_connection> TCP_connection::create(asio::io_context& io_context, unsigned int client_id) {
        return std::shared_ptr<TCP_connection>(new TCP_connection(io_context, client_id));
    }

    TCP_server::TCP_server(unsigned short int port) : acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
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

    void TCP_server::broadcast(std::string message) {
        packet input = {{0}, std::vector<char>(message.begin(), message.end())};
        input.header.size_bytes = input.body.size();
        std::vector<std::byte> packet_buffer(sizeof(packet_header) + input.body.size());
        memcpy(packet_buffer.data(), &input, sizeof(packet_header));
        memcpy(packet_buffer.data() + sizeof(packet_header), input.body.data(), input.body.size());
        for(auto& [key, c_thread] : connection_map) {
            async_write(c_thread.connection->socket, asio::buffer(packet_buffer.data(), packet_buffer.size()), 
                [key, message](const asio::error_code& error, size_t bytes) {
                    if(error) {
                        std::cout << "Failed to send message to client " + std::to_string(key) + ".\n";
                    } else {
                        std::cout << "Sent " << bytes << " bytes of data to client " + std::to_string(key) + ".\n";
                        std::cout << "Message: " << message << "\n";
                    }
                }
            );
        }
    }
}