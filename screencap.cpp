#define NOMINMAX
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        OutputDebugStringA("Screencap.dll: PROCESS_ATTACH\n");
    }
    return TRUE;
}
#include <functional>

#include <unknwn.h> 
#include <objbase.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.graphics.capture.interop.h>

#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>


#include <d3d11.h>
#include <d3d11_2.h>
#include <dxgi1_2.h>

#include <chrono>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
# include <opencv2/highgui/highgui.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "direct3d11.interop.h"

#include "utils.h"

using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

# define THRESHOLD 0.7
# define FIND_TARGET_NUM 30
// these variables keep alive until this dll is unloaded, which helps shorten execution time
static bool initialized = false;
static winrt::com_ptr<ID3D11Device> d3dDevice;
static winrt::com_ptr<ID3D11DeviceContext> d3dContext;

void InitializeD3D()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        if (hr == RPC_E_CHANGED_MODE) {
            // COM Library is already initialized by ahk, you can ignore this error
        } else {
            throw winrt::hresult_error(hr);
        }
    }

    D3D_FEATURE_LEVEL featureLevel;
    winrt::check_hresult(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(), &featureLevel, d3dContext.put()));

    initialized = true;
}

int HandleException()
{
    try {
        throw;
    }
    catch (const winrt::hresult_error& ex) {
        std::cerr << "WinRT error: " << std::hex << ex.code() << " - " << ex.message().c_str() << std::endl;
        return (int)ex.code();
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return -2;
    }
}


int CaptureWindow(HWND hwnd, winrt::com_ptr<ID3D11Device>& d3dDevice, winrt::com_ptr<ID3D11DeviceContext>& d3dContext, int* x1, int* y1, int* x2, int* y2, int mode)
{

    GraphicsCaptureItem item{ nullptr };

    auto activationFactory = winrt::get_activation_factory<GraphicsCaptureItem>();
    auto interop = activationFactory.as<IGraphicsCaptureItemInterop>();

    winrt::check_hresult(
        interop->CreateForWindow(
            hwnd,
            winrt::guid_of<GraphicsCaptureItem>(),
            reinterpret_cast<void**>(winrt::put_abi(item))));

    // auto item = CreateCaptureItemForWindow(hwnd);

    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    winrt::com_ptr<::IInspectable> device;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), device.put()));
    auto size = item.Size();
    int width = size.Width;
    int height = size.Height;
    std::cout << "Window size: " << size.Width << "x" << size.Height << std::endl;

    auto pool = Direct3D11CaptureFramePool::Create(
        device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>(), 
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        1,
        size);
    auto session = pool.CreateCaptureSession(item);
    if (!session.IsSupported()){
        std::cout << "session is not supported" << std::endl;
        return -3;
    }
    session.StartCapture();
    
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame{ nullptr };
    for (int i=0; i<10; ++i) { 
        frame = pool.TryGetNextFrame();
        if (frame) {
            std::cout << "frame captured" << std::endl;
            break; 
        }
        Sleep(10);
    }
    if (!frame) return 2;


    
    auto frameTex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

    winrt::com_ptr<ID3D11Texture2D> stagingTex;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width              = width;
    desc.Height             = height;
    desc.MipLevels          = 1;
    desc.ArraySize          = 1;
    desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage              = D3D11_USAGE_STAGING;
    desc.BindFlags          = 0;                    // BindFlags.None
    desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ; // CpuAccessFlags.Read
    desc.MiscFlags          = 0;
    winrt::check_hresult(d3dDevice->CreateTexture2D(&desc, nullptr, stagingTex.put()));
    d3dContext->CopyResource(stagingTex.get(), frameTex.get());
    // std::cout << "CopyResource completed" << std::endl;
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = d3dContext->Map(
        stagingTex.get(),
        0,
        D3D11_MAP_READ,
        0,
        &mapped
    );
    if (FAILED(hr)) {
        std::cout << "Map failed with HRESULT: " << std::hex << hr << std::endl;
        return -4;
    }
    // std::cout << "Map succeeded, RowPitch: " << mapped.RowPitch << std::endl;


    *x1 = 0, *y1 = 0, *x2 = 0, *y2 = 0;
    // std::cout << "captured image size: " << width << "x" << height << std::endl;
    cv::Mat image(height, width, CV_8UC4, mapped.pData, mapped.RowPitch);
    // cv::Mat gray_img, binary_img;
    // cv::cvtColor(image, gray_img, cv::COLOR_BGRA2GRAY);
    // cv::threshold(gray_img, binary_img, 254, 255, cv::THRESH_BINARY);
    // 1~2ms faster 
    cv::Mat binary_img;
    cv::Scalar lower(240, 240, 240, 0);   
    cv::Scalar upper(255, 255, 255, 255); 
    cv::inRange(image, lower, upper, binary_img);

    cv::Rect roi_rect(0.3 * width, 0.13 * height, 0.55 * width, 0.685 * height); 
    cv::Mat roi = binary_img(roi_rect);
    // cv::Mat black_img = cv::Mat::zeros(roi.size(), CV_8UC1); 

    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(roi, lines, 1, CV_PI / 180, 100, 0.104167 * width, 0.01 * width);
    int min_x = roi.cols, min_y = roi.rows, max_x = 0, max_y = 0;
    cv::Point ptLeft = {0, 0}, ptTop = {0, 0}, ptRight = {0, 0}, ptBottom = {0, 0};

    // std::cout << "ROI size: " << roi.cols << "x" << roi.rows << std::endl;
    // std::cout << "Detected lines: " << lines.size() << std::endl;
    for (size_t i = 0; i < lines.size(); i++) {
        cv::Vec4i l = lines[i];
        
        double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
        if ((angle > 25 && angle < 45)) {
            cv::Point p1(l[0], l[1]), p2(l[2], l[3]);
            std::vector<cv::Point> points = {p1, p2};
            for (const auto& pt : points) {
                if (pt.x < min_x) { min_x = pt.x; ptLeft = pt; }
                if (pt.x > max_x) { max_x = pt.x; ptRight = pt; }
                if (pt.y < min_y) { min_y = pt.y; ptTop = pt; }
                if (pt.y > max_y) { max_y = pt.y; ptBottom = pt; }
            }

            std::cout << "Line " << i << ": (" << l[0] << ", " << l[1] << ") to (" << l[2] << ", " << l[3] << "), angle: " << angle << std::endl;
            // cv::line(black_img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2);
        }
    }
    // cv::imshow("Detected Lines", black_img);
    // cv::imshow("ROI", roi);
    // cv::waitKey(0);
    // auto working_dir = GetWorkingDir();
    // cv::imwrite((working_dir / "img/lines.png").string(), black_img);
    if (ptTop.x - ptLeft.x < 0.1 * width || ptRight.x - ptBottom.x < 0.1 * width) {
        std::cout << "could not find two distinct lines" << std::endl;
        d3dContext->Unmap(stagingTex.get(), 0);
        return 1;
    } else {
        if (ptLeft.x == 0 || ptTop.y == 0 || ptRight.x == roi.cols || ptBottom.y == roi.rows) {
            std::cout << "maybe ROI is too small, but continue processing" << std::endl;
        } 
        cv::Point2f button1 = (cv::Point2f(ptLeft) + cv::Point2f(ptTop)) * 0.5f;
        cv::Point2f button2 = (cv::Point2f(ptBottom) + cv::Point2f(ptRight)) * 0.5f;
        cv::Point2f button3 = cv::Point2f(ptTop);
        cv::Point finalButton1(button1.x + roi_rect.x, button1.y + roi_rect.y);
        cv::Point finalButton2(button2.x + roi_rect.x, button2.y + roi_rect.y);
        cv::Point finalButton3(button3.x, button3.y + 0.02 * height);
        // std::cout << "ptLeft: (" << ptLeft.x << ", " << ptLeft.y << "), ptTop: (" << ptTop.x << ", " << ptTop.y << "), ptRight: (" << ptRight.x << ", " << ptRight.y << "), ptBottom: (" << ptBottom.x << ", " << ptBottom.y << ")" << std::endl;
        // std::cout << "button1: (" << button1.x << ", " << button1.y << "), button2: (" << button2.x << ", " << button2.y << "), button3: (" << button3.x << ", " << button3.y << ")" << std::endl;
        // std::cout << "roi_rect: (" << roi_rect.x << ", " << roi_rect.y << ", " << roi_rect.width << ", " << roi_rect.height << ")" << std::endl;
        // cv::circle(black_img, button1, 10, cv::Scalar(255, 255, 255, 255), 3);
        // cv::circle(black_img, button2, 10, cv::Scalar(140, 140, 140, 255), 3);
        // cv::imshow("Detected Buttons", black_img);
        // cv::waitKey(0);

        if (mode == 1) {
            *x1 = finalButton1.x;
            *y1 = finalButton1.y;
            std::cout << "Mode 1: Returning button 1 coordinates: (" << finalButton1.x << ", " << finalButton1.y << ")" << std::endl;
        } else {
            *x1 = finalButton2.x;
            *y1 = finalButton2.y;
            std::cout << "Mode 2: Returning button 2 coordinates: (" << finalButton2.x << ", " << finalButton2.y << ")" << std::endl;
        }
        *x2 = finalButton3.x;
        *y2 = finalButton3.y;
    }

    d3dContext->Unmap(stagingTex.get(), 0);
    return 0;
}


// DLL function here
#ifdef BUILD_DLL
extern "C" __declspec(dllexport)
int RunProcess(int* x1, int* y1, int* x2, int* y2, int mode) {
    try {
        if (!initialized) {
            InitializeD3D();
        }
        int result = CaptureWindow(GetForegroundWindow(), d3dDevice, d3dContext, x1, y1, x2, y2, mode);
        return result;
    } catch (...) {
        return HandleException();
    }
}

#else

int main(){
#ifdef BUILD_EXE_TEST
    std::cout << "this is test mode" << std::endl;
    return 0;

#else //(BUILD_EXE)
    try {
        if (!initialized) {
            InitializeD3D();
        }
        int result = CaptureWindow(GetForegroundWindow(), d3dDevice, d3dContext);
        return result;
    } catch (...) {
        return HandleException();
    }
#endif
}

#endif