#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <mutex>

typedef std::shared_ptr<std::vector<unsigned char>> ImagePtr_t;
typedef long long TimePoint_t;

class ScreenshotProvider
{
    ULONG_PTR m_gdiplus_token;
    TimePoint_t m_last_updated = 0;
    std::mutex m_mutex;

    ImagePtr_t m_screenshort = nullptr;

public:

    const TimePoint_t update_interval = 10;

    static ScreenshotProvider& instance() {
        static ScreenshotProvider instance;
        return instance;
    }

    ImagePtr_t get() {
        return update();
    }

    bool is_expired(TimePoint_t since);
    bool is_expired(std::string since_str);

private:
    ScreenshotProvider();
    ~ScreenshotProvider();

    ImagePtr_t update();
};

