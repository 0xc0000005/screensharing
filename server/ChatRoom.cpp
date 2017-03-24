#include <sstream>
#include <algorithm>

#include "ChatRoom.h"

using namespace std::chrono;

TimePoint_t ChatRoom::put_message(const std::string & msg, const std::string & from)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto current_time = steady_clock::now().time_since_epoch().count();
    m_messages.push_front(Message_t{ current_time, from, msg });
    m_users_activity[from] = current_time;

    // purge messages history
    if (m_messages.size() > max_messages)
        m_messages.pop_back();

    // purge users activity
    auto active_since = std::max(0ll, current_time - activity_timeout * std::chrono::steady_clock::period::den);
    for (auto i = m_users_activity.begin(); i != m_users_activity.end(); ++i) {
        if (i->second < active_since)
            i = m_users_activity.erase(i);
    }

    return current_time;
}

std::list<Message_t> ChatRoom::messages_after(TimePoint_t time, const std::string& for_user)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto current_time = steady_clock::now().time_since_epoch().count();
    m_users_activity[for_user] = current_time;

    MsgContainer_t messages;
    for (auto i = m_messages.begin(); i != m_messages.end() && i->time > time; ++i)
        messages.push_front(*i);

    return messages;
}

std::string ChatRoom::json_messages_after(const std::string& time_str, const std::string& for_user)
{
    auto current_time = steady_clock::now().time_since_epoch().count();

    long long time = 0;
    try { time = std::stoll(time_str); } // force to 0 if any parsing errors
    catch (...) {}

    std::ostringstream json;
    json << "{";

    auto last_update = current_time;
    if (time > 0) { // no history for new users
        auto messages = messages_after(time, for_user);
        if (messages.size() > 0) {
            json << "\"messages\":[";
            
            for (const auto& message : messages)
                json << "\"" << message.from << ": " << message.message << "\", ";

            json.seekp(-2, json.cur); // delete reduntunt comma and space
            json << "], ";
            last_update = messages.back().time;
        }
    }

    json << "\"timestamp\":" << last_update << ", ";

    int users = active_users(activity_timeout);
    json << "\"users\":" << users << "}";

    return json.str();
}

int ChatRoom::active_users(int for_seconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto current_time = steady_clock::now().time_since_epoch().count();
    auto active_since = std::max(0ll, current_time - for_seconds * std::chrono::steady_clock::period::den);
    int user_counter = 0;
    for (const auto& user : m_users_activity) {
        if (user.second > active_since)
            ++user_counter;
    }

    return user_counter;
}

