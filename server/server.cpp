#include <conio.h>
#include <iostream>

#include "HttpServer.h"
#include "Terminator.h"


int main()
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    char const server_addr[] = "192.168.1.100";
    std::uint16_t server_port = 5555;
    auto max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0)
        max_threads = 2;
    Terminator terminator;

    HttpServer server(server_addr, server_port, &terminator, max_threads);

    std::cout << "Press [Enter] to stop server..." << std::endl;

    while (_kbhit() == 0) {
        if (terminator.is_terminated())
            break;
    }
    server.stop();

    WSACleanup();

    return 0;
}
