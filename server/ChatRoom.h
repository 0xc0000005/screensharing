#pragma once

#include <list>
#include <map>
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
    typedef std::map<std::string, TimePoint_t> UsersActivity_t;

    std::mutex m_mutex;
    MsgContainer_t m_messages;
    UsersActivity_t m_users_activity;

public:

    static ChatRoom& instance() {
        static ChatRoom instance;
        return instance;
    }

    TimePoint_t put_message(const std::string& msg, const std::string& from);
    std::list<Message_t> messages_after(TimePoint_t time, const std::string& for_user);
    std::string json_messages_after(const std::string& time_str, const std::string& for_user);

    static const int max_messages = 500;
    static const int activity_timeout = 5;

private:
    ChatRoom() {};
    ~ChatRoom() {};

    int active_users(int for_seconds);
};

