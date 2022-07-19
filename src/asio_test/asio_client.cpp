#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#define _WIN32_WINNT 0x0a00
#endif
#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>
#include <asio-1.22.2\include\asio\ts\buffer.hpp>
#include <asio-1.22.2\include\asio\ts\internet.hpp>

std::vector<char> v_buffer(0x5000);

void grab_some_data(asio::ip::tcp::socket& socket) {
    socket.async_read_some(asio::buffer(v_buffer.data(), v_buffer.size()), 
        [&](asio::error_code error_code, std::size_t length) {
            if(!error_code) {
                std::cout << "Read " << length << " bytes.\n";

                for(int i = 0; i < length; i++) {
                    std::cout << v_buffer[i];
                }

                grab_some_data(socket);
            }
        }
    );
}

int main() {
    asio::error_code error_code;

    // create the platform specific interface (context)
    asio::io_context context;

    // give the context fake work to do to prevent it from stopping
    asio::io_context::work idle_work(context);

    std::thread ctx_thread(
        [&]() {
            context.run();
        }
    );

    // create endpoint for where the computer will connect to, 80 is the port number
    asio::ip::tcp::endpoint endpoint0(asio::ip::make_address("51.38.81.49", error_code), 80);

    // socket is created, the context is passed in to make the socket work for this system
    asio::ip::tcp::socket socket0(context);

    socket0.connect(endpoint0, error_code);

    if(!error_code) {
        std::cout << "Connected\n";
    } else {
        std::cout << "Failed to connect: " << error_code.message() << "\n";
    }

    if(socket0.is_open()) {
        grab_some_data(socket0);

        std::string request =
        "GET /index.html HTTP/1.1\r\n"
        "Host: david-barr.co.uk\r\n"
        "Connection: close\r\n\r\n";

        socket0.write_some(asio::buffer(request.data(), request.size()), error_code);
    }

    context.stop();
    ctx_thread.join();

    return 0;
}