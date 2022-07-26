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

    struct TCP_connection {
        tcp::socket socket;

        std::string message = "This is client ";
        unsigned int client_id;

        TCP_connection(asio::io_context& io_context, unsigned int client_id);

        void start();

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

    struct TCP_client {
        asio::io_context io_context;
        tcp::resolver resolver;
        tcp::socket socket;

        TCP_client(unsigned short int target_port);

        void start();
    };

    TCP_connection::TCP_connection(asio::io_context& io_context, unsigned int client_id) : socket(io_context) {
        message += std::to_string(client_id) + "\n";
        this->client_id = client_id;
    }

    void TCP_connection::start() {
        bool break_bool = false;
        asio::streambuf buffer;
        socket.async_receive(buffer.prepare(256),
            [this, &break_bool](const asio::error_code& error_code, size_t bytes_transferred) {
                if(error_code == asio::error::eof) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected (no error).\n";
                } else if(error_code) {
                    std::cout << "Client " + std::to_string(client_id) + " disconnected, error: " << error_code.message() << "\n";
                }
                break_bool = true;
            }
        );

        unsigned int time_container = clock() - 4000;
        while(true) {
            if(break_bool) break;
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
        }

        socket.close();
    }

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

    void TCP_server::broadcast(std::string message) {
        for(auto& [key, c_thread] : connection_map) {
            async_write(c_thread.connection->socket, asio::buffer(message), 
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

    TCP_client::TCP_client(unsigned short int target_port) : resolver(io_context), socket(io_context) {
        auto endpoint = resolver.resolve("127.0.0.1", std::to_string(target_port));
        asio::connect(socket, endpoint);
    }

    void TCP_client::start() {
        while(true) {
            // listen for messages
            std::array<char, 128> buffer;

            asio::error_code error_code;

            size_t length = socket.read_some(asio::buffer(buffer.data(), buffer.size()), error_code);

            if(error_code == asio::error::eof) {
                break;
            } else if(error_code) {
                std::cout << "Connection closed, error: " << error_code.message() << "\n";
                break;
            }

            std::cout.write(buffer.data(), length);
        }
    }
}