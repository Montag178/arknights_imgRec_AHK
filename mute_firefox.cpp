#include <Windows.h>
#include <atlbase.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <combaseapi.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <shlwapi.h>
#include <wchar.h>


int main() {
    HRESULT hrCi = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hrCi)) { std::cerr << "CoInitializeEx failed\n"; return 1; }

    {
        CComPtr<IMMDeviceEnumerator> pEnum;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pEnum));
        if (FAILED(hr)) { std::cerr << "Create MMDeviceEnumerator failed: 0x" << std::hex << hr << "\n"; return 1; }

        CComPtr<IMMDevice> pDevice;
        hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (FAILED(hr)) { std::cerr << "GetDefaultAudioEndpoint failed: 0x" << std::hex << hr << "\n"; return 1; }

        CComPtr<IAudioSessionManager2> pSessionManager2;
        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager2);
        if (FAILED(hr)) { std::cerr << "Activate(IASM2) failed: 0x" << std::hex << hr << "\n"; return 1; }

        CComPtr<IAudioSessionEnumerator> pSessionEnum;
        hr = pSessionManager2->GetSessionEnumerator(&pSessionEnum);
        if (FAILED(hr)) { std::cerr << "GetSessionEnumerator failed: 0x" << std::hex << hr << "\n"; return 1; }

        int count = 0;
        hr = pSessionEnum->GetCount(&count);
        if (FAILED(hr)) { std::cerr << "GetCount failed: 0x" << std::hex << hr << "\n"; return 1; }

        for (int i = 0; i < count; ++i) {
            CComPtr<IAudioSessionControl> pControl;
            hr = pSessionEnum->GetSession(i, &pControl);
            if (FAILED(hr)) { continue; }

            CComQIPtr<IAudioSessionControl2> pControl2(pControl.p);
            if (!pControl2) { continue; }

            DWORD pid = 0;
            if (FAILED(pControl2->GetProcessId(&pid)) || pid == 0) { continue; }

            CHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
            if (hProcess.m_h == NULL) { continue; }

            WCHAR path[MAX_PATH] = {};
            if (!GetProcessImageFileNameW(hProcess, path, MAX_PATH)) { continue; }

            LPCWSTR exeName = PathFindFileNameW(path);
            if (exeName && _wcsicmp(exeName, L"firefox.exe") == 0) {
                CComQIPtr<ISimpleAudioVolume> pVolume(pControl2.p);
                if (pVolume) {
                    hr = pVolume->SetMute(TRUE, nullptr);
                    if (FAILED(hr)) {
                        std::cerr << "SetMute failed: 0x" << std::hex << hr << "\n";
                    }
                }
            }
        }
    }

    CoUninitialize();
    return 0;
}
