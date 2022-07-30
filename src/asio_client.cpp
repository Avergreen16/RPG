#include <iostream>
#include <array>
#include <list>
#include <memory>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

using asio::ip::tcp;

namespace netwk {
    struct packet_header {
        uint16_t type;
        uint32_t size_bytes;
    };

    struct entity_movement_packet {
        std::array<double, 2> position;
        directions direction;
    };
    
    std::string make_string(char* input_ptr, size_t size_bytes, bool terminator_char = true) {
        std::string return_string(input_ptr, size_bytes);
        if(terminator_char) return_string += '\0';
        return return_string;
    }
    
    struct TCP_client {
        asio::io_context io_context;
        tcp::resolver resolver;
        tcp::socket socket;
        std::thread recieve_thread;

        TCP_client(unsigned short int target_port);

        void receive_loop(std::vector<std::string>& input_collector, bool& game_running);

        void start(std::vector<std::string>& input_collector, bool& game_running);

        void send(int id, std::vector<uint8_t>& body);
    };

    TCP_client::TCP_client(unsigned short int target_port) : resolver(io_context), socket(io_context) { // connects the socket to the server
        auto endpoint = resolver.resolve("127.0.0.1", std::to_string(target_port));
        asio::connect(socket, endpoint);
    }

    void TCP_client::receive_loop(std::vector<std::string>& input_collector, bool& game_running) { // recieves incoming packets and does things with them based on header contents
        if(!game_running) socket.close();
        else {
            std::shared_ptr<std::array<char, sizeof(packet_header)>> buffer(new std::array<char, sizeof(packet_header)>);
            socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
                [this, buffer, &input_collector, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                    if(error_code) {
                        std::cout << "Error: " << error_code.message() << "\n";
                    } else {
                        packet_header header;
                        memcpy(&header, buffer->data(), bytes_transferred);

                        switch(header.type) {
                            case 0: { // chat message
                                std::shared_ptr<std::vector<char>> buffer_1(new std::vector<char>(header.size_bytes));
                                socket.async_read_some(asio::buffer(buffer_1->data(), buffer_1->size()), 
                                    [this, buffer_1, &input_collector, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                                        if(error_code) {
                                            std::cout << "Error: " << error_code.message() << "\n";
                                        } else { // insert text into input collector to be processed by client
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server.\n";
                                            std::string message = make_string(buffer_1->data(), bytes_transferred);
                                            std::cout << "Message recieved: " << message << "\n";
                                            input_collector.push_back(message);
                                            receive_loop(input_collector, game_running);
                                        }
                                    }
                                );
                                break;
                            }

                            case 1: { // player position packets
                                std::shared_ptr<std::vector<char>> buffer_1(new std::vector<char>(header.size_bytes));
                                socket.async_read_some(asio::buffer(buffer_1->data(), buffer_1->size()), 
                                    [this, buffer_1, &input_collector, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                                        if(error_code) {
                                            std::cout << "Error: " << error_code.message() << "\n";
                                        } else {

                                        }
                                    }
                                );
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

    void TCP_client::start(std::vector<std::string>& input_collector, bool& game_running) { // starts the recieve loop
        recieve_thread = std::thread(
            [this, &input_collector, &game_running]() {
                this->receive_loop(input_collector, game_running);
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
                } else {
                    std::cout << "Sent " << bytes << " bytes of data to the server.\n";
                }
            }
        );
    };
}