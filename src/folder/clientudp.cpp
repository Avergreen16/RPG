#include <winsock2.h>
#include <iostream>
#include <array>

struct player_packet {
    unsigned int player_ID;
    char username[16];
    std::array<double, 2> position;
    double health_points;
};

int main() {
    WSADATA wsadata;
    int wsaerr;
    WORD wVersionRequester = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequester, &wsadata);
    if(wsaerr == 0) {
        std::cout << "WSAStartup successful!\n";
    } else {
        std::cout << "WSAStartup error!\n";
        std::cout << wsadata.szSystemStatus << "\n";
        return 1;
    }

    SOCKET client_socket = INVALID_SOCKET;
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(client_socket == INVALID_SOCKET) {
        std::cout << "socket error!\n";
        std::cout << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    } else {
        std::cout << "socket successful!\n";
    }

    unsigned short int server_port = 16003;
    sockaddr_in service;
    service.sin_family = AF_INET;
    std::string server_address = "127.0.0.1";
    service.sin_addr.s_addr = inet_addr(server_address.c_str());
    service.sin_port = htons(server_port);

    char pass[16] = "Hello world!";
    std::cout << "sending string: " << pass << "\n";
    sendto(client_socket, pass, 16, 0, (SOCKADDR*)&service, sizeof(service));

    /*std::cout << "Password: ";
    int password_entered = 0;
    while(password_entered == 0) {
        char pass[16];
        std::cin.getline(pass, 16);
        send(client_socket, pass, 16, 0);
        recv(client_socket, (char*)&password_entered, sizeof(password_entered), 0);
        if(password_entered == 0) {
            std::cout << "That was the wrong password. Try again: ";
        } else if(password_entered == 1) {
            std::cout << "That was the correct password.\n";
        } else if(password_entered == 2) {
            std::cout << "You've entered the wrong password too many times and have been locked out.\n";
        }
    }*/

    closesocket(client_socket);
    WSACleanup();
    return 0;
}