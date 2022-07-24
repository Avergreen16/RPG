#include <iostream>
#include <memory>
#include <vector>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

namespace netwk {
    using asio::ip::tcp;

    struct TCP_connection {
        tcp::socket socket;

        std::string message = "Hello, client! ;)";

        TCP_connection(asio::io_context& io_context) : socket(io_context) {
            
        }

        void start() {
            asio::async_write(socket, asio::buffer(message), 
                [this](const asio::error_code& error, size_t bytes) {
                    if(error) {
                        std::cout << "Failed to send message.\n";
                    } else {
                        std::cout << "Sent " << bytes << " bytes of data.\n";
                    }
                }
            );

            asio::streambuf buffer;
            socket.async_receive(buffer.prepare(256),
                [this](const asio::error_code& error_code, size_t bytes_transferred) {
                    if(error_code == asio::error::eof) {
                        std::cout << "Client disconnected (no error).\n";
                    } else if(error_code) {
                        std::cout << "Client disconnected, error: " << error_code.message() << "\n";
                    }
                }
            );
        }

        static std::shared_ptr<TCP_connection> create(asio::io_context& io_context) {
            return std::shared_ptr<TCP_connection>(new TCP_connection(io_context));
        }
    };

    struct TCP_server {
        unsigned short int port;
        asio::io_context io_context;
        tcp::acceptor acceptor;
        std::vector<std::shared_ptr<TCP_connection>> connection_vector;

        TCP_server(unsigned short int port) : acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
            this->port = port;
        }

        void start_accept() {
            // create connection
            auto connection = TCP_connection::create(io_context);

            connection_vector.push_back(connection);

            // asynchronously accept connection
            acceptor.async_accept(connection->socket,
                [this, connection](const asio::error_code& error) {
                    if(!error) {
                        connection->start();
                    }

                    start_accept();
                }
            );
        }

        int run() {
            try {
                start_accept();
                io_context.run();
            } catch(std::exception& e) {
                std::cout << e.what() << std::endl;
                return -1;
            }
            
            return 0;
        }
    };
}