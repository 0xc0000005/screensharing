#pragma once

#include <list>
#include <mutex>
#include <chrono>

typedef long long TimePoint_t;

typedef struct {
    TimePoint_t time;
    std::string from;
    std::string message;
} Message_t;

class ChatRoom
{
    typedef std::list<Message_t> MsgContainer_t;

    std::mutex m_mutex;
    MsgContainer_t m_messages;

public:

    static ChatRoom& instance() {
        static ChatRoom instance;
        return instance;
    }

    TimePoint_t put_message(const std::string& msg, const std::string& from);
    std::list<Message_t> messages_after(TimePoint_t time);
    std::string json_messages_after(const std::string& time_str);

    static const int max_messages = 500;

private:
    ChatRoom() {};
    ~ChatRoom() {};
};

