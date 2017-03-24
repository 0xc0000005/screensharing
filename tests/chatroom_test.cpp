#include <thread>
#include <chrono>
#include <string>
#include <gtest/gtest.h>

#include "../server/ChatRoom.h"

class ChatRoomTest : public ChatRoom
{
public:
    int active_users(int for_seconds) { return ChatRoom::active_users(for_seconds); }
};

TEST(chat_room, add_message)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    ASSERT_EQ(room.messages_after(0, "127.0.0.1").size(), 1);
}

TEST(chat_room, empty_room)
{
    ChatRoomTest room;
    ASSERT_EQ(room.messages_after(0, "127.0.0.1").size(), 0);
}

TEST(chat_room, purge_messages)
{
    ChatRoomTest room;
    for (int i = 0; i < 500; ++i)
        room.put_message("test", "127.0.0.1");
    ASSERT_EQ(room.messages_after(0, "127.0.0.1").size(), 500);
    room.put_message("test", "127.0.0.1");
    ASSERT_EQ(room.messages_after(0, "127.0.0.1").size(), 500);
}

TEST(chat_room, json)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    auto json = room.json_messages_after("0", "");
    ASSERT_TRUE(json.length() != 0);
}

TEST(chat_room, json_users_1)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.1");
    auto json = room.json_messages_after("0", "");
    auto users_pos = json.find("\"users\":");
    ASSERT_TRUE(users_pos != std::string::npos);
    ASSERT_EQ(json.substr(users_pos).compare("\"users\":1}"), 0);
}

TEST(chat_room, json_users_3)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.2");
    room.put_message("test", "127.0.0.3");
    auto json = room.json_messages_after("0", "");
    auto users_pos = json.find("\"users\":");
    ASSERT_TRUE(users_pos != std::string::npos);
    ASSERT_EQ(json.substr(users_pos).compare("\"users\":3}"), 0);
}

TEST(chat_room, json_new_user)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.2");
    room.put_message("test", "127.0.0.3");
    auto json = room.json_messages_after("0", "");
    auto msg_pos = json.find("\"messages\":");
    ASSERT_EQ(msg_pos, std::string::npos);
}

TEST(chat_room, json_messages)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    auto json = room.json_messages_after("1", "");
    auto msg_pos = json.find("\"messages\":");
    ASSERT_TRUE(msg_pos != std::string::npos);
}

TEST(chat_room, json_purge_activity)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.2");
    room.put_message("test", "127.0.0.3");

    auto sleep_time = std::chrono::seconds(ChatRoomTest::activity_timeout + 1);
    std::this_thread::sleep_for(sleep_time);

    auto json = room.json_messages_after("0", "");
    auto users_pos = json.find("\"users\":");
    ASSERT_EQ(json.substr(users_pos).compare("\"users\":0}"), 0);
}

TEST(chat_room, json_msg_timepoint_after)
{
    ChatRoomTest room;
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.2");
    room.put_message("test", "127.0.0.3");
    auto current_time = std::chrono::steady_clock::now().time_since_epoch().count();
    auto json = room.json_messages_after(std::to_string(current_time), "");
    auto msg_pos = json.find("\"messages\":");
    ASSERT_EQ(msg_pos, std::string::npos);
}

TEST(chat_room, json_msg_timepoint_before)
{
    ChatRoomTest room;
    auto current_time = std::chrono::steady_clock::now().time_since_epoch().count();
    room.put_message("test", "127.0.0.1");
    room.put_message("test", "127.0.0.2");
    room.put_message("test", "127.0.0.3");
    auto json = room.json_messages_after(std::to_string(current_time), "");
    auto msg_pos = json.find("\"messages\":");
    ASSERT_TRUE(msg_pos != std::string::npos);
}

TEST(chat_room, json_msg_timepoint_middle)
{
    ChatRoomTest room;
    room.put_message("test1", "127.0.0.1");
    room.put_message("test2", "127.0.0.2");
    auto current_time = std::chrono::steady_clock::now().time_since_epoch().count();
    room.put_message("test3", "127.0.0.3");
    room.put_message("test4", "127.0.0.4");
    auto json = room.json_messages_after(std::to_string(current_time), "");
    std::string expected{ "{\"messages\":[\"127.0.0.3: test3\", \"127.0.0.4: test4\"], \"timestamp\":" };
    ASSERT_EQ(json.compare(0, expected.length(), expected), 0);
}

TEST(chat_room, json_active_users)
{
    ChatRoomTest room;
    room.put_message("test1", "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    room.put_message("test2", "127.0.0.2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    room.put_message("test3", "127.0.0.3");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(room.active_users(5), 3);
    ASSERT_EQ(room.active_users(3), 2);
    ASSERT_EQ(room.active_users(2), 1);
    ASSERT_EQ(room.active_users(0), 0);
}
