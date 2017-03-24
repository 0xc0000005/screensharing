#include <conio.h>
#include <iostream>
#include <fstream>

#include "HttpServer.h"
#include "Terminator.h"

class KbTerminator : public ITerminator
{
    std::atomic_int m_terminated = 0;

public:
    bool is_terminated() override {
        m_terminated.fetch_or(_kbhit());
        return m_terminated != 0;
    }
    void terminate() { m_terminated.fetch_or(1); }
};

static void print_usage_and_exit(const char* prog_name)
{
    std::cout << std::endl;
    std::cout << "Usage: " << prog_name << " ip-addr port" << std::endl;
    std::cout << "Current directory will be used as root" << std::endl;
    exit(-1);
}

int main(int argc, const char** argv)
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    const auto prog_name = argv[0];

    if (argc < 3)
        print_usage_and_exit(prog_name);

    const auto server_addr = argv[1];
    const auto server_port_str = argv[2];

    std::ifstream file(INDEX_HTML, std::ifstream::binary);
    if (!file.good()) {
        std::cout << "Cannot find " << INDEX_HTML << " in current directory" << std::endl;
        print_usage_and_exit(prog_name);
    }
    file.close();

    std::uint16_t server_port = 0;
    try { server_port = static_cast<std::uint16_t>(std::stoi(server_port_str)); } // force to 0 if any parsing errors
    catch (...) {}

    if (server_port == 0) {
        std::cout << "Cannot parse port: " << server_port_str << std::endl;
        print_usage_and_exit(prog_name);
    }

    auto max_threads = std::thread::hardware_concurrency();
    if (max_threads < 1)
        max_threads = 2;

    std::cout << "Starting server on " << server_addr << ":" << server_port << " using " << max_threads << " threads..." << std::endl;

    KbTerminator terminator;

    HttpServer server(server_addr, server_port, &terminator);
    server.start(max_threads);

    if (!terminator.is_terminated())
        std::cout << "Press any key to stop server..." << std::endl;

    while(!terminator.is_terminated())
        std::this_thread::yield();

    server.stop();

    WSACleanup();

    return 0;
}
