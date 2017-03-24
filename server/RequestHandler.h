#pragma once

#include <evhttp.h>

#include <string>
#include <mutex>

struct evhttp;
class ITerminator;

class SocketBinder
{
    evutil_socket_t m_socket = -1;
    std::mutex m_mutex;
    const std::string m_addr;
    const std::uint16_t m_port;

public:
    SocketBinder(const char* server_addr, std::uint16_t server_port) :
        m_addr(server_addr), m_port(server_port)
    {}

    void bind(evhttp* http_event);
};

class RequestHandler
{
    std::unique_ptr<event_base, decltype(&event_base_free)> m_base_event;
    std::unique_ptr<evhttp, decltype(&evhttp_free)> m_http_event;
    ITerminator* m_terminator = nullptr;

public:
    RequestHandler(SocketBinder& socket_binder, ITerminator* terminator = nullptr);
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    void run();

private:
    static void generic_request_handler(evhttp_request* request, void* param);
    
    void response_with_file(evhttp_request* request, std::string filename, std::string content_type);
    void response_with_screenshort(evhttp_request* request);
    void response_with_messages(evhttp_request* request);
    void response_to_message(evhttp_request* request);
};

const std::string INDEX_HTML = "index.html";