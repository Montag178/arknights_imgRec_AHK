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
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "direct3d11.interop.h"

#include "utils.h"
#include "MatchingMethods.h"

using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

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

// Data structure to pass GPU texture and mapped buffer between capture and processing functions
struct CaptureData
{
    winrt::com_ptr<ID3D11Texture2D> stagingTex;  // GPU staging texture (keeps texture alive)
    D3D11_MAPPED_SUBRESOURCE mapped;              // CPU-accessible mapped buffer
    int width;                                    // Frame width in pixels
    int height;                                   // Frame height in pixels
    int status;                                   // 0 = success, <0 = error code
};

// Capture window frame and prepare staging texture with mapped buffer
// Returns CaptureData with status code (0 = success, <0 = error)
CaptureData CaptureWindowFrame(HWND hwnd, winrt::com_ptr<ID3D11Device>& d3dDevice, 
                                winrt::com_ptr<ID3D11DeviceContext>& d3dContext)
{
    CaptureData result = {};
    result.status = 0;
    
    try {
        GraphicsCaptureItem item{ nullptr };

        auto activationFactory = winrt::get_activation_factory<GraphicsCaptureItem>();
        auto interop = activationFactory.as<IGraphicsCaptureItemInterop>();

        winrt::check_hresult(
            interop->CreateForWindow(
                hwnd,
                winrt::guid_of<GraphicsCaptureItem>(),
                reinterpret_cast<void**>(winrt::put_abi(item))));

        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> device;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), device.put()));
        auto size = item.Size();
        result.width = size.Width;
        result.height = size.Height;
        std::cout << "Window size: " << size.Width << "x" << size.Height << std::endl;

        auto pool = Direct3D11CaptureFramePool::Create(
            device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>(), 
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            1,
            size);
        auto session = pool.CreateCaptureSession(item);
        if (!session.IsSupported()){
            std::cout << "session is not supported" << std::endl;
            result.status = -3;
            return result;
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
        if (!frame) {
            result.status = 2;
            return result;
        }

        auto frameTex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width              = result.width;
        desc.Height             = result.height;
        desc.MipLevels          = 1;
        desc.ArraySize          = 1;
        desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage              = D3D11_USAGE_STAGING;
        desc.BindFlags          = 0;
        desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags          = 0;
        winrt::check_hresult(d3dDevice->CreateTexture2D(&desc, nullptr, result.stagingTex.put()));
        d3dContext->CopyResource(result.stagingTex.get(), frameTex.get());

        HRESULT hr = d3dContext->Map(
            result.stagingTex.get(),
            0,
            D3D11_MAP_READ,
            0,
            &result.mapped
        );
        if (FAILED(hr)) {
            std::cout << "Map failed with HRESULT: " << std::hex << hr << std::endl;
            result.status = -4;
            return result;
        }
    }
    catch (...) {
        result.status = -1;
    }
    
    return result;
}

// Process captured frame: delegates to line detection algorithm
// Returns button coordinates based on mode selection
// Status: 0 = success, 1 = warning (insufficient lines), <0 = error
int ProcessCapturedFrame(CaptureData& captureData, int mode, 
                         int* x1, int* y1, int* x2, int* y2)
{
    try {
        int width = captureData.width;
        int height = captureData.height;

        *x1 = 0, *y1 = 0, *x2 = 0, *y2 = 0;
        
        // Use mapped buffer directly without copying
        cv::Mat image(height, width, CV_8UC4, captureData.mapped.pData, captureData.mapped.RowPitch);
        
        // Process using line detection method
        // ProcessResult result = ProcessViaLineDetection(image, width, height, false);
        
        ProcessResult result = ProcessViaLineDetection(image, width, height, false);
        // Log debug information
        if (!result.debug_info.empty()) {
            std::cout << result.debug_info;
        }
        
        // Handle different status codes
        if (result.status < 0) {
            // Error occurred
            return result.status;
        } else if (result.status > 0) {
            // Warning: unable to find lines
            return result.status;
        }
        
        // Success: assign button coordinates based on mode
        if (mode == 1) {
            *x1 = (int)result.button1.x;
            *y1 = (int)result.button1.y;
            std::cout << "Mode 1: Returning button 1 coordinates: (" << *x1 << ", " << *y1 << ")" << std::endl;
        } else {
            *x1 = (int)result.button2.x;
            *y1 = (int)result.button2.y;
            std::cout << "Mode 2: Returning button 2 coordinates: (" << *x1 << ", " << *y1 << ")" << std::endl;
        }
        *x2 = (int)result.button3.x;
        *y2 = (int)result.button3.y;
        
        return 0;
    }
    catch (...) {
        return -1;
    }
}

int GetButtonCoordinates(HWND hwnd, winrt::com_ptr<ID3D11Device>& d3dDevice, winrt::com_ptr<ID3D11DeviceContext>& d3dContext, int* x1, int* y1, int* x2, int* y2, int mode)
{
    // Capture window frame and prepare GPU staging texture with mapped buffer
    CaptureData captureData = CaptureWindowFrame(hwnd, d3dDevice, d3dContext);
    
    // Check if capture succeeded
    if (captureData.status != 0) {
        return captureData.status;
    }
    
    // Process the captured frame using the staging texture directly (no copying)
    int processResult = ProcessCapturedFrame(captureData, mode, x1, y1, x2, y2);
    
    // Unmap the staging texture to release GPU resource
    d3dContext->Unmap(captureData.stagingTex.get(), 0);
    
    return processResult;
}


// DLL function here
#ifdef BUILD_DLL
extern "C" __declspec(dllexport)
int RunProcess(int* x1, int* y1, int* x2, int* y2, int mode) {
    try {
        if (!initialized) {
            InitializeD3D();
        }
        int result = GetButtonCoordinates(GetForegroundWindow(), d3dDevice, d3dContext, x1, y1, x2, y2, mode);
        return result;
    } catch (...) {
        return HandleException();
    }
}
extern "C" __declspec(dllexport)
int CaptureTest() {
    try {
        if (!initialized) {
            InitializeD3D();
        }
        // Capture window frame and prepare GPU staging texture with mapped buffer
        CaptureData captureData = CaptureWindowFrame(GetForegroundWindow(), d3dDevice, d3dContext);
        
        // Check if capture succeeded
        if (captureData.status != 0) {
            std::cout << "Error code: " << captureData.status << std::endl;
            d3dContext->Unmap(captureData.stagingTex.get(), 0);
            return captureData.status;
        }

        cv::Mat image(captureData.height, captureData.width, CV_8UC4, captureData.mapped.pData, captureData.mapped.RowPitch);
        cv::imshow("Captured Image (Raw)", image);
        cv::waitKey(0);
        cv::destroyAllWindows();
        
        // Unmap the staging texture to release GPU resource
        d3dContext->Unmap(captureData.stagingTex.get(), 0);
        return 0;
    } catch (...) {
        return HandleException();
    }
}

extern "C" __declspec(dllexport)
int ShowProcessedImage() {
    try {
        if (!initialized) {
            InitializeD3D();
        }
        // Capture window frame and prepare GPU staging texture with mapped buffer
        CaptureData captureData = CaptureWindowFrame(GetForegroundWindow(), d3dDevice, d3dContext);
        
        // Check if capture succeeded
        if (captureData.status != 0) {
            std::cout << "Error code: " << captureData.status << std::endl;
            d3dContext->Unmap(captureData.stagingTex.get(), 0);
            return captureData.status;
        }

        int width = captureData.width;
        int height = captureData.height;

        // Create cv::Mat from captured frame data
        cv::Mat image(height, width, CV_8UC4, captureData.mapped.pData, captureData.mapped.RowPitch);
        
        // Process the captured image using line detection method
        bool is_test_mode = true; // prepare black image for visualization in line detection method
        ProcessResult result = ProcessViaLineDetection(image, width, height, is_test_mode);
        
        // Log debug information
        std::cout << result.debug_info;
        
        // Write debug info to file for AHK to read
        std::ofstream debug_file("debug_info.txt");
        if (debug_file.is_open()) {
            debug_file << result.debug_info;
            debug_file.close();
        }
        
        if (result.status >= 0) {
            // Display processed image with detected buttons
            cv::Mat display_image = image.clone();  // Clone for display overlay
            cv::Rect roi_rect(0.3 * width, 0.13 * height, 0.55 * width, 0.685 * height);
            
            // Draw ROI rectangle on display
            cv::rectangle(display_image, roi_rect, cv::Scalar(0, 255, 0, 255), 2);
            
            // Draw detected button positions on display image
            if (result.status == 0) {
                cv::circle(display_image, cv::Point(result.button1.x, result.button1.y), 15, cv::Scalar(0, 255, 0, 255), 3);
                cv::putText(display_image, "B1", cv::Point(result.button1.x + 20, result.button1.y), 
                           cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0, 255), 2);
                
                cv::circle(display_image, cv::Point(result.button2.x, result.button2.y), 15, cv::Scalar(0, 0, 255, 255), 3);
                cv::putText(display_image, "B2", cv::Point(result.button2.x + 20, result.button2.y), 
                           cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255, 255), 2);
                
                cv::circle(display_image, cv::Point(result.button3.x, result.button3.y), 15, cv::Scalar(255, 0, 0, 255), 3);
                cv::putText(display_image, "B3", cv::Point(result.button3.x + 20, result.button3.y), 
                           cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 0, 0, 255), 2);
                
                std::cout << "Button coordinates:\n";
                std::cout << "  B1: (" << (int)result.button1.x << ", " << (int)result.button1.y << ")\n";
                std::cout << "  B2: (" << (int)result.button2.x << ", " << (int)result.button2.y << ")\n";
                std::cout << "  B3: (" << (int)result.button3.x << ", " << (int)result.button3.y << ")\n";
            }
            
            cv::imshow("Processed Image (with Button Locations)", display_image);
            cv::imshow("Line Detection Result", result.processed_image);
            cv::waitKey(0);
            cv::destroyAllWindows();
        }
        
        // Unmap the staging texture to release GPU resource
        d3dContext->Unmap(captureData.stagingTex.get(), 0);
        return result.status == 0 ? 0 : result.status;
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
        int result = GetButtonCoordinates(GetForegroundWindow(), d3dDevice, d3dContext, &x1, &y1, &x2, &y2, 1);
        return result;
    } catch (...) {
        return HandleException();
    }
#endif
}

#endif