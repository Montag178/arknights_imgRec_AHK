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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "direct3d11.interop.h"

#include "utils.h"

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


int CaptureWindow(HWND hwnd, const char* filename, winrt::com_ptr<ID3D11Device>& d3dDevice, winrt::com_ptr<ID3D11DeviceContext>& d3dContext)
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
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R8G8B8A8UIntNormalized,
        1,
        size);
    auto session = pool.CreateCaptureSession(item);
    auto start = std::chrono::high_resolution_clock::now();
    if (!session.IsSupported()){
        std::cout << "session is not supported" << std::endl;
        return 2;
    }
    session.StartCapture();
    
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame{ nullptr };
    for (int i=0; i<10; ++i) { // 最大約500ms待つ
        frame = pool.TryGetNextFrame();
        if (frame) {
            std::cout << "frame captured" << std::endl;
            break; 
        }
        Sleep(10);
    }
    if (!frame) return 2;


    
    auto frameTex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    // create staging ID3D11Texture2d

    winrt::com_ptr<ID3D11Texture2D> stagingTex;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width              = width;
    desc.Height             = height;
    desc.MipLevels          = 1;
    desc.ArraySize          = 1;
    desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage              = D3D11_USAGE_STAGING;
    desc.BindFlags          = 0;                    // BindFlags.None
    desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ; // CpuAccessFlags.Read
    desc.MiscFlags          = 0;
    winrt::check_hresult(d3dDevice->CreateTexture2D(&desc, nullptr, stagingTex.put()));
    d3dContext->CopyResource(stagingTex.get(), frameTex.get());
    std::cout << "CopyResource completed" << std::endl;
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
        return 3;
    }
    std::cout << "Map succeeded, RowPitch: " << mapped.RowPitch << std::endl;
    // // Check first pixel
    // if (mapped.pData) {
    //     uint8_t* data = (uint8_t*)mapped.pData;
    //     std::cout << "First pixel: R=" << (int)data[0] << " G=" << (int)data[1] << " B=" << (int)data[2] << " A=" << (int)data[3] << std::endl;
    // }


    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "time" << duration << "ms" << std::endl;
    stbi_write_png(filename, width, height, 4, mapped.pData, mapped.RowPitch);

    // frameSurface->Unmap();
    d3dContext->Unmap(stagingTex.get(), 0);
    return 0;
}


// DLL function here
#ifdef BUILD_DLL
extern "C" __declspec(dllexport)
int RunProcess(){
    try {
        if (!initialized) {
            InitializeD3D();
        }
        const wchar_t* filename = L"C:\\Users\\TumorNecrosisFactor\\Desktop\\capture.png";
        int result = CaptureWindow(GetForegroundWindow(), filename, d3dDevice, d3dContext);
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
        auto working_dir = GetWorkingDir();
        std::string fullpath = (working_dir / "img/capture.png").string();
        const char* filename = fullpath.c_str();
        std::cout << "Output file: " << filename << std::endl;
        int result = CaptureWindow(GetForegroundWindow(), filename, d3dDevice, d3dContext);
        return result;
    } catch (...) {
        return HandleException();
    }
#endif
}

#endif