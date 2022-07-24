#include <iostream>
#include <array>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>
//#include <asio-1.22.2\include\asio\ts\buffer.hpp>
//#include <asio-1.22.2\include\asio\ts\internet.hpp>

using asio::ip::tcp;

int main() {
    try {
        asio::io_context io_context;

        tcp::resolver resolver(io_context);

        auto endpoint = resolver.resolve("127.0.0.1", "42254");

        tcp::socket socket(io_context);

        asio::connect(socket, endpoint);

        while(true) {
            // listen for messages
            std::array<char, 128> buffer;

            asio::error_code error_code;

            size_t length = socket.read_some(asio::buffer(buffer.data(), buffer.size()), error_code);

            if(error_code == asio::error::eof) {
                break;
            } else if(error_code) {
                throw error_code;
            }

            std::cout.write(buffer.data(), length);
        }
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}