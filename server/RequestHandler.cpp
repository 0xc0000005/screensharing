#include <evhttp.h>
#include <fstream>

#include "RequestHandler.h"
#include "Terminator.h"

#include "ScreenshotProvider.h"


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

    if (command == EVHTTP_REQ_GET) {
        if (uri == "/")
            return instance->response_with_file(request, "index.html", "text/html; charset=UTF-8");
        if (uri == "/screenshort")
            return instance->response_with_screenshort(request);

        evhttp_send_error(request, HTTP_NOTFOUND, nullptr);
    }

    if (command == EVHTTP_REQ_POST) {
        auto buf = evhttp_request_get_input_buffer(request);
        std::string data;
        while (evbuffer_get_length(buf)) {
            char cbuf[128];
            int n = evbuffer_remove(buf, cbuf, sizeof(buf) - 1);
            cbuf[n] = 0;
            data += cbuf;
        }
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
    evhttp_send_reply(request, HTTP_OK, "", out_buf);
}

void RequestHandler::response_with_screenshort(evhttp_request * request)
{
    auto headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(headers, "Content-Type", "image/png");

    auto png_data = ScreenshotProvider::get();
    if (png_data->size()) {
        auto out_buf = evhttp_request_get_output_buffer(request);
        evbuffer_add(out_buf, png_data->data(), png_data->size());
        evhttp_send_reply(request, HTTP_OK, "", out_buf);
    }
    else {
        evhttp_send_error(request, HTTP_NOTFOUND, nullptr);
    }
}

