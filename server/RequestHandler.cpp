#include <evhttp.h>
#include <fstream>

#include "RequestHandler.h"
#include "Terminator.h"

#include "ScreenshotProvider.h"
#include "ChatRoom.h"

typedef std::unique_ptr<evhttp_uri, decltype(&evhttp_uri_free)> RAII_evhttp_uri_t;
typedef std::unique_ptr<evkeyvalq, decltype(&evhttp_clear_headers)> RAII_evkeyvalq_t;


void SocketBinder::bind(evhttp* http_event)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_socket == -1) {
        auto *bound_socket = evhttp_bind_socket_with_handle(http_event, m_addr.c_str(), m_port);
        if (!bound_socket)
            throw std::runtime_error("Cannot bind socket");
        m_socket = evhttp_bound_socket_get_fd(bound_socket);
        if (m_socket == -1)
            throw std::runtime_error("Cannot get socket fd");
    }
    else {
        if (evhttp_accept_socket(http_event, m_socket) == -1)
            throw std::runtime_error("Cannot accept socket");
    }
}

RequestHandler::RequestHandler(SocketBinder& socket_binder, ITerminator* terminator) :
    m_base_event(event_base_new(), &event_base_free),
    m_http_event(evhttp_new(m_base_event.get()), &evhttp_free),
    m_terminator(terminator)
{
    if (m_base_event == nullptr || m_http_event == nullptr)
        throw std::runtime_error("Cannot init events");

    evhttp_set_allowed_methods(m_http_event.get(), EVHTTP_REQ_GET|EVHTTP_REQ_PUT);
    evhttp_set_gencb(m_http_event.get(), RequestHandler::generic_request_handler, this);

    socket_binder.bind(m_http_event.get());
}

void RequestHandler::run()
{
    while (m_terminator != nullptr && !m_terminator->is_terminated())
    {
        event_base_loop(m_base_event.get(), EVLOOP_NONBLOCK);
        std::this_thread::yield();
    }
}

void RequestHandler::generic_request_handler(evhttp_request* request, void* param)
{
    RequestHandler* instance = reinterpret_cast<RequestHandler*>(param);

    std::string uri = evhttp_request_get_uri(request);
    auto command = evhttp_request_get_command(request);

    const std::string messages_uri = "/messages";
    const std::string screenshort_uri = "/screenshort";

    if (command == EVHTTP_REQ_GET) {
        if (uri == "/")
            return instance->response_with_file(request, INDEX_HTML, "text/html; charset=UTF-8");
        if (uri.compare(0, screenshort_uri.length(), screenshort_uri) == 0)
            return instance->response_with_screenshort(request);
        if (uri.compare(0, messages_uri.length(), messages_uri) == 0)
            return instance->response_with_messages(request);

        evhttp_send_error(request, HTTP_NOTFOUND, nullptr);
    }

    if (command == EVHTTP_REQ_PUT) {
        if (uri == "/new-message")
            return instance->response_to_message(request);
        evhttp_send_error(request, HTTP_BADREQUEST, nullptr);
    }
}

void RequestHandler::response_with_file(evhttp_request* request, std::string filename, std::string content_type)
{
    std::ifstream file(filename, std::ifstream::ate | std::ifstream::binary);
    if (!file.good()) {
        evhttp_send_error(request, HTTP_NOTFOUND, nullptr);
        return;
    }

    const auto file_size = static_cast<std::size_t>(file.tellg());
    file.seekg(0);
    auto file_content = std::make_unique<char[]>(file_size);
    file.read(file_content.get(), file_size);

    auto headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(headers, "Content-Type", content_type.c_str());

    auto out_buf = evhttp_request_get_output_buffer(request);
    evbuffer_add(out_buf, file_content.get(), file_size);
    evhttp_send_reply(request, HTTP_OK, nullptr, out_buf);
}

void RequestHandler::response_with_screenshort(evhttp_request * request)
{
    const auto uri = evhttp_request_get_uri(request);
    RAII_evhttp_uri_t parsed_uri(evhttp_uri_parse(uri), &evhttp_uri_free);
    const auto query = evhttp_uri_get_query(parsed_uri.get());
    evkeyvalq parsed_query;
    evhttp_parse_query_str(query, &parsed_query);
    RAII_evkeyvalq_t parsed_uri_guard(&parsed_query, &evhttp_clear_headers);

    auto headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(headers, "Content-Type", "image/png");
    evhttp_add_header(headers, "Cache-Control", "no-cache");

    const auto timestamp_str = evhttp_find_header(&parsed_query, "after");
    const auto reload_str = evhttp_find_header(&parsed_query, "reload");
    if (!reload_str && timestamp_str && !ScreenshotProvider::instance().is_expired(timestamp_str)) {
        evhttp_send_reply(request, HTTP_NOTMODIFIED, nullptr, nullptr);
        return;
    }

    auto png_data = ScreenshotProvider::instance().get();
    if (png_data->size()) {
        auto out_buf = evhttp_request_get_output_buffer(request);
        evbuffer_add(out_buf, png_data->data(), png_data->size());
        evhttp_send_reply(request, HTTP_OK, nullptr, out_buf);
    }
    else {
        evhttp_send_error(request, HTTP_NOTFOUND, nullptr);
    }
}

void RequestHandler::response_with_messages(evhttp_request * request)
{
    const auto uri = evhttp_request_get_uri(request);
    RAII_evhttp_uri_t parsed_uri(evhttp_uri_parse(uri), &evhttp_uri_free);
    const auto query = evhttp_uri_get_query(parsed_uri.get());
    evkeyvalq parsed_query;
    evhttp_parse_query_str(query, &parsed_query);
    RAII_evkeyvalq_t parsed_uri_guard(&parsed_query, &evhttp_clear_headers);
    
    const auto timestamp_str = evhttp_find_header(&parsed_query, "after");
    if (timestamp_str) {
        auto headers = evhttp_request_get_output_headers(request);
        evhttp_add_header(headers, "Content-Type", "application/json");

        auto json = ChatRoom::instance().json_messages_after(timestamp_str);

        auto out_buf = evhttp_request_get_output_buffer(request);
        evbuffer_add(out_buf, json.c_str(), json.length());
        evhttp_send_reply(request, HTTP_OK, nullptr, out_buf);
    }
    else {
        evhttp_send_error(request, HTTP_BADREQUEST, nullptr);
    }
}

void RequestHandler::response_to_message(evhttp_request * request)
{
    auto buf = evhttp_request_get_input_buffer(request);
    auto connection = evhttp_request_get_connection(request);
    char* peer_addr;
    ev_uint16_t peer_port;
    evhttp_connection_get_peer(connection, &peer_addr, &peer_port);
    std::string message;
    while (evbuffer_get_length(buf)) {
        char cbuf[128];
        int n = evbuffer_remove(buf, cbuf, sizeof(buf) - 1);
        cbuf[n] = 0;
        message += cbuf;
    }
    ChatRoom::instance().put_message(message, peer_addr);
    evhttp_send_reply(request, HTTP_OK, nullptr, nullptr);
}

