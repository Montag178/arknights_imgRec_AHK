#include <functional>

#include <unknwn.h> 
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.graphics.capture.interop.h>

#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>


#include <d3d11.h>
#include <d3d11_2.h>
#include <dxgi1_2.h>
#include <iostream>
#include <chrono>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "direct3d11.interop.h"

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

bool ReadTexture(ID3D11Texture2D* tex, int width, int height,
    const std::function<void(void*, int)>& callback) {

    com_ptr<ID3D11Device> device;
    com_ptr<ID3D11DeviceContext> ctx;
    tex->GetDevice(device.put());
    device->GetImmediateContext(ctx.put());

    // create query
    com_ptr<ID3D11Query> query_event;
    {
        D3D11_QUERY_DESC qdesc = { D3D11_QUERY_EVENT , 0 };
        device->CreateQuery(&qdesc, query_event.put());
    }

    // create staging texture
    com_ptr<ID3D11Texture2D> staging;
    {
        D3D11_TEXTURE2D_DESC tmp;
        tex->GetDesc(&tmp);
        D3D11_TEXTURE2D_DESC desc{ (UINT)width, (UINT)height, 1, 1,
            tmp.Format, { 1, 0 }, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
        device->CreateTexture2D(&desc, nullptr, staging.put());
    }

    // dispatch copy
    {
        D3D11_BOX box{ };
        box.right = width;
        box.bottom = height;
        box.back = 1;
        ctx->CopySubresourceRegion(staging.get(), 0, 0, 0, 0, tex, 0, &box);
        ctx->End(query_event.get());
        ctx->Flush();
    }

    // wait for copy to complete
    int wait_count = 0;
    while (ctx->GetData(query_event.get(), nullptr, 0, 0) == S_FALSE) {
        ++wait_count; // just for debug
    }

    // map
    D3D11_MAPPED_SUBRESOURCE mapped{ };
    if (SUCCEEDED(ctx->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        D3D11_TEXTURE2D_DESC desc{ };
        staging->GetDesc(&desc);

        callback(mapped.pData, mapped.RowPitch);
        ctx->Unmap(staging.get(), 0);
        return true;
    }
    return false;
}

int CaptureWindow(HWND hwnd, const wchar_t* filename, winrt::com_ptr<ID3D11Device>& d3dDevice, winrt::com_ptr<ID3D11DeviceContext>& d3dContext)
{
    auto start = std::chrono::high_resolution_clock::now();

    D3D_FEATURE_LEVEL featureLevel;
    winrt::check_hresult(D3D11CreateDevice(
    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(), &featureLevel, d3dContext.put()));
    GraphicsCaptureItem item{ nullptr };

    auto activationFactory = get_activation_factory<GraphicsCaptureItem>();
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

    auto pool = Direct3D11CaptureFramePool::Create(
        device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>(), 
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        1,
        size);
    auto session = pool.CreateCaptureSession(item);
    if (!session.IsSupported()){
        std::cout << "err, world" << std::endl;
        return 2;
    }
    session.StartCapture();
    
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame{ nullptr };
    for (int i=0; i<10; ++i) { // 最大約500ms待つ
        frame = pool.TryGetNextFrame();
        if (frame) {
            std::cout << "err, world" << std::endl;
            break; 
        }
        Sleep(10);
    }
    if (!frame) return 2;


    
    auto frameTex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    // create staging ID3D11Texture2d

    winrt::com_ptr<ID3D11Texture2D> stagingTex;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width              = 1920;
    desc.Height             = 1080;
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
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = d3dContext->Map(
        stagingTex.get(),
        0,
        D3D11_MAP_READ,
        0,
        &mapped
    );
    if (FAILED(hr)) return 3;

    // DXGI_MAPPED_RECT rect;

    int width = size.Width;
    int height = size.Height;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "time" << duration << "ms" << std::endl;
    stbi_write_png("C:\\Users\\reward\\Desktop\\capture.png", width, height, 4, mapped.pData, mapped.RowPitch);

    // frameSurface->Unmap();
    d3dContext->Unmap(stagingTex.get(), 0);
    return 0;
}

// DLL function here
#ifdef BUILD_DLL
extern "C" __declspec(dllexport)
int RunProcess(){
    return 0;
}
#endif

int main(){
    //prepare bitmap
    #ifdef TEST_MODE
        std::cout << "this is test mode" << std::endl;
    #else
        std::cout << "Hello, world" << std::endl;
        winrt::init_apartment();
        winrt::com_ptr<ID3D11Device> d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext> d3dContext;
        const wchar_t* filename = L"C:/Users/reward/Desktop/capture.png";
        CaptureWindow(GetForegroundWindow(), filename, d3dDevice, d3dContext);
        // int width = 800, height = 600;
        // std::vector<uint8_t> dummy(width * height * 4, 0xFF);
        // stbi_write_png("C:\\Users\\reward\\Desktop\\capture.png", width, height, 4, dummy.data(), width * 4);
    #endif
    return 0;
}