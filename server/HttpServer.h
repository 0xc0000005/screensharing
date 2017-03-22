#pragma once

#include <vector>
#include <thread>
#include <future>
#include <cstdint>

#include "RequestHandler.h"

class ITerminator;

class HttpServer
{
    SocketBinder m_socket_binder;
    ITerminator* m_terminator = nullptr;

    typedef std::pair<std::thread, std::future<void>> ThreadPoolItem_t;
    typedef std::vector<ThreadPoolItem_t> ThreadPool_t;
    ThreadPool_t m_thread_pool;

public:
    HttpServer(const char* server_addr, std::uint16_t server_port, ITerminator* terminator = nullptr, unsigned int threads = 0);
    ~HttpServer() { stop(); }

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void stop();

private:

    static void thread_runner(std::promise<void> promise, SocketBinder& socket_binder, ITerminator* terminator);
};
