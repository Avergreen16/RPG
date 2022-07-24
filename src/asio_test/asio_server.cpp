#include "asio_lib.cpp"

int main() {
    netwk::TCP_server server(0xa50e);
    server.run();
}