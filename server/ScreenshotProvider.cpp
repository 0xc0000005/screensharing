#include <Windows.h>

#pragma warning(push)
#pragma warning(disable:4458) 
#include <gdiplus.h>
#pragma warning(pop)

#include "ScreenshotProvider.h"

using namespace Gdiplus;
using namespace std::chrono;

static bool GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return false;

    auto buf =std::make_unique<BYTE[]>(size);
    auto codecs = reinterpret_cast<ImageCodecInfo*>(buf.get());

    GetImageEncoders(num, size, codecs);
    for (UINT i = 0; i < num; ++i) {
        auto codec = codecs[i];
        if (wcscmp(codec.MimeType, format) == 0) {
            *pClsid = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

static ImagePtr_t BitmapToPng(HBITMAP hBitmap)
{
    CLSID pngClsid;
    if (!GetEncoderClsid(L"image/png", &pngClsid))
        return nullptr;

    std::unique_ptr<Bitmap> bmp(Gdiplus::Bitmap::FromHBITMAP(hBitmap, nullptr));

    IStream *stream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
    if (FAILED(hr))
        return nullptr;

    bmp->Save(stream, &pngClsid, nullptr);
        
    LARGE_INTEGER offset{0};
    ULARGE_INTEGER size;
    stream->Seek(offset, STREAM_SEEK_END, &size);
    stream->Seek(offset, STREAM_SEEK_SET, nullptr);

    size_t buff_size = static_cast<size_t>(size.QuadPart);
    auto png_data = std::make_shared<std::vector<unsigned char>>();
    png_data->resize(buff_size);

    stream->Read(png_data->data(), buff_size, nullptr);

    stream->Release();

    return png_data;
}

static ImagePtr_t CaptureWindow(HWND hWnd)
{
    auto hdcSrc = GetDC(hWnd);
    const int width = GetDeviceCaps(hdcSrc, HORZRES);
    const int height = GetDeviceCaps(hdcSrc, VERTRES);

    auto hdcMem = CreateCompatibleDC(hdcSrc);
    auto hBitmap = CreateCompatibleBitmap(hdcSrc, width, height);
    auto hBitmapOld = (HBITMAP)SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, 0, 0, width, height, hdcSrc, 0, 0, SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hdcMem, hBitmapOld);

    auto png_data = BitmapToPng(hBitmap);

    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    DeleteDC(hdcSrc);

    return png_data;
}

ScreenshotProvider::ScreenshotProvider()
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplus_token, &gdiplusStartupInput, nullptr);
}


ScreenshotProvider::~ScreenshotProvider()
{
    GdiplusShutdown(m_gdiplus_token);
}

ImagePtr_t ScreenshotProvider::update(TimeInt_t interval)
{
    if (is_expired(interval)) {
        auto old_data = m_screenshort;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (old_data == m_screenshort) {
            m_last_updated = steady_clock::now();
            m_screenshort = CaptureWindow(nullptr);
        }
    }

    return m_screenshort;
}

bool ScreenshotProvider::is_expired(TimeInt_t interval)
{
    auto current_time = steady_clock::now();
    return (m_last_updated + interval < current_time);
}
