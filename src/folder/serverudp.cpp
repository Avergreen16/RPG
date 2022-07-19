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

    SOCKET server_socket = INVALID_SOCKET;
    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(server_socket == INVALID_SOCKET) {
        std::cout << "socket error!\n";
        std::cout << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    } else {
        std::cout << "socket successful!\n";
    }
    
    unsigned short int port = 16003;
    sockaddr_in service;
    service.sin_family = AF_INET;
    //std::string address = "127.0.0.1";
    service.sin_addr.s_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(port);
    if(bind(server_socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        std::cout << "bind error!\n";
        std::cout << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return 1;
    } else {
        std::cout << "bind successful! waiting to recieve data...\n";
    }

    sockaddr_in client_addr;
    int clientaddr_len = sizeof(client_addr);
    char recieve_data[16];
    recvfrom(server_socket, recieve_data, 16, 0, (SOCKADDR*)&client_addr, &clientaddr_len);
    std::cout << "string recieved:" << recieve_data << "\n";
    std::cout << "client address: " << client_addr.sin_addr.S_un.S_addr << "\nclient port: " << client_addr.sin_port << "\n";
    
    /*int tries = 0;
    char correct_password[16] = "Avergreen16";
    int password_entered = 0;
    while(password_entered == 0 && tries < 5) {
        char pass[16];
        recv(accept_socket, pass, 16, 0);
        if(strcmp(pass, correct_password) == 0) {
            password_entered = 1;
            std::cout << "The correct password was entered.\n";
        } else {
            tries++;
            if(tries >= 5) {
                password_entered = 2;
                std::cout << "The wrong password was entered too many times and the client has been locked out.\n";
            }
        }
        send(accept_socket, (char*)&password_entered, sizeof(password_entered), 0);
    }*/

    closesocket(server_socket);
    WSACleanup();
    return 0;
}