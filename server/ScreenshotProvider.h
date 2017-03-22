#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <mutex>

typedef std::shared_ptr<std::vector<unsigned char>> ImagePtr_t;
typedef std::chrono::duration<unsigned int> TimeInt_t;

class ScreenshotProvider
{
    typedef std::chrono::steady_clock::time_point TimePoint_t;

    ULONG_PTR m_gdiplus_token;
    TimePoint_t m_last_updated;
    std::mutex m_mutex;

    ImagePtr_t m_screenshort;

public:

    static ImagePtr_t get(TimeInt_t interval = std::chrono::seconds(10)) {
        return instance().update(interval);
    }

    static bool expired(TimeInt_t interval = std::chrono::seconds(10)) {
        return instance().is_expired(interval);
    }

private:
    ScreenshotProvider();
    ~ScreenshotProvider();

    static ScreenshotProvider& instance() {
        static ScreenshotProvider instance;
        return instance;
    }

    ImagePtr_t update(TimeInt_t interval);
    bool is_expired(TimeInt_t interval);
};

