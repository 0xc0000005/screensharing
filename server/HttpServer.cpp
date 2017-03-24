#include <iostream>

#include "HttpServer.h"
#include "Terminator.h"


HttpServer::HttpServer(const char* server_addr, std::uint16_t server_port, ITerminator* terminator) :
    m_socket_binder(server_addr, server_port),
    m_terminator(terminator)
{
}

void HttpServer::start(unsigned int threads)
{
    for (unsigned int i = 0; i < threads; ++i) {
        std::promise<void> promise;
        auto future = promise.get_future();
        std::thread thread(thread_runner, std::move(promise), std::ref<SocketBinder>(m_socket_binder), m_terminator);
        auto item = std::make_pair(std::move(thread), std::move(future));
        m_thread_pool.push_back(std::move(item));
    }
}

void HttpServer::stop()
{
    if (m_terminator)
        m_terminator->terminate();

    while (!m_thread_pool.empty()) {
        auto item = std::move(m_thread_pool.back());
        m_thread_pool.pop_back();

        try {
            auto& future = item.second;
            future.get(); // process exceptions if any
        }
        catch (std::exception& e) {
            std::cout << std::endl << e.what() << std::endl;
        }
        auto& thread = item.first;
        if (thread.joinable())
            thread.join();
    }
}

void HttpServer::thread_runner(std::promise<void> promise, SocketBinder& socket_binder, ITerminator* terminator)
{
    try {
        RequestHandler thread(socket_binder, terminator);
        thread.run();
        promise.set_value();
    }
    catch (...) {
        // stop all other threads, don't try to recover
        if (terminator)
            terminator->terminate();

        promise.set_exception(std::current_exception());
    }
}
