#include <winsock2.h>
#include <iostream>
#include <array>
#include <vector>
#include <thread>

struct prng {
    double value;
    double factor;

    double operator() () {
        double temp = value * factor;
        value = temp - int(temp);
        return value;
    }
};

struct player_packet {
    std::array<double, 2> position;
    std::array<int, 2> active_texture;
    bool quit_game = false;
};

std::array<player_packet, 2> player_packets;

void socket_thread(SOCKET s, int player_number) {
    bool quit_game = false;
    int send_number;
    while(!quit_game) {
        recv(s, (char*)&player_packets[player_number], sizeof(player_packet), 0);
        if(player_packets[player_number].quit_game) {
            closesocket(s);
            quit_game = true;
            break;
        }
        if(player_number == 0) send_number = 1;
        else send_number = 0;
        send(s, (char*)&player_packets[send_number], sizeof(player_packet), 0);
    }
}

int main() {
    WSADATA wsadata;
    int wsaerr = WSAStartup(0x0202, &wsadata);
    if(wsaerr != 0) {
        std::cout << "WSAStartup error: " << wsadata.szSystemStatus << "\n";
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_socket == INVALID_SOCKET) {
        std::cout << "socket error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    unsigned short int server_port = 16003;
    std::string server_ip = "127.0.0.1";

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip.c_str());
    address.sin_port = htons(server_port);

    if(bind(server_socket, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cout << "bind error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }
    std::vector<std::thread> thread_vector;

    for(int i = 0; i < 2; i++) {
        if(listen(server_socket, 2) == SOCKET_ERROR) {
            std::cout << "listen error: " << WSAGetLastError() << "\n";
            WSACleanup();
            return 1;
        }
        std::cout << "listening for connections...\n";

        SOCKET accept_socket = accept(server_socket, NULL, NULL);
        if(accept_socket == INVALID_SOCKET) {
            std::cout << "accept error: " << WSAGetLastError() << "\n";
        } else {
            std::cout << "connection accepted\n";
            thread_vector.push_back(std::thread(socket_thread, accept_socket, i));
        }
    }

    for(std::thread &t : thread_vector) {
        if(t.joinable()) t.join();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}