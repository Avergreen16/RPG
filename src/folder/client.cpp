#include <winsock2.h>
#include <iostream>
#include <array>

int main() {
    WSADATA wsadata;
    int wsaerr = WSAStartup(0x0202, &wsadata);
    if(wsaerr != 0) {
        std::cout << "WSAStartup error!\n";
        std::cout << wsadata.szSystemStatus << "\n";
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_socket == INVALID_SOCKET) {
        std::cout << "socket error!\n";
        std::cout << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    unsigned short int server_port = 16003;
    std::string server_ip = "127.0.0.1";

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip.c_str());
    address.sin_port = htons(server_port);

    if(connect(client_socket, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cout << "connect error!\n";
        std::cout << WSAGetLastError() << "\n";
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    std::cout << "connection established\n";

    std::cout << "playing \"rock, paper, scissors\"\nr == rock\np = paper\ns = scissors\nq = quit game\n";

    bool quit_game = false;
    char selected_character;
    std::array<int, 2> response_array;
    while(!quit_game) {
        std::cout << "\nenter in a character: ";
        std::cin >> selected_character;
        switch(selected_character) {
            case 'q':
                quit_game = true;
                break;

            case 'r':
                std::cout << "you picked: rock\n";
                break;

            case 'p':
                std::cout << "you picked: paper\n";
                break;

            case 's':
                std::cout << "you picked: scissors\n";
                break;

            default:
                std::cout << "that is not a valid character. please enter in r, p, s, or q.\n";
                continue;
        }
        send(client_socket, &selected_character, 1, 0);

        if(quit_game == true) break;

        recv(client_socket, (char*)&response_array, sizeof(response_array), 0);

        switch(response_array[0]) {
            case 0:
                std::cout << "the computer chose: rock\n";
                break;

            case 1:
                std::cout << "the computer chose: paper\n";
                break;

            case 2:
                std::cout << "the computer chose: scissors\n";
                break;
        }

        switch(response_array[1]) {
            case -1:
                std::cout << "you lose!\n";
                break;

            case 0:
                std::cout << "it's a tie!\n";
                break;

            case 1:
                std::cout << "you win!\n";
                break;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}