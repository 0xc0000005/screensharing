#include <sstream>

#include "ChatRoom.h"

using namespace std::chrono;

TimePoint_t ChatRoom::put_message(const std::string & msg, const std::string & from)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto current_time = steady_clock::now().time_since_epoch().count();
    m_messages.push_front(Message_t{ current_time, from, msg });
    
    // purge messages history
    if (max_messages != 0 && m_messages.size() > max_messages)
        m_messages.pop_back();

    return current_time;
}

std::list<Message_t> ChatRoom::messages_after(TimePoint_t time)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    MsgContainer_t messages;
    for (auto i = m_messages.begin(); i != m_messages.end() && i->time > time; ++i)
        messages.push_front(*i);

    return messages;
}

std::string ChatRoom::json_messages_after(const std::string& time_str)
{
    auto current_time = steady_clock::now().time_since_epoch().count();

    long long time = 0;
    try { time = std::stoll(time_str); } // force to 0 if any parsing errors
    catch (...) {}

    std::ostringstream json;
    json << "{";

    auto last_update = current_time;
    if (time > 0) { // no history for new users
        auto messages = messages_after(time);
        if (messages.size() > 0) {
            json << "\"messages\":[";
            
            for (const auto& message : messages)
                json << "\"" << message.from << ": " << message.message << "\", ";

            json.seekp(-2, json.cur); // delete reduntunt comma and space
            json << "], ";
            last_update = messages.back().time;
        }
    }

    json << "\"timestamp\":" << last_update << "}";

    return json.str();
}

