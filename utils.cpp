#include "utils.h"
#include <chrono>
#include <iostream>
#include <windows.h>

static std::chrono::high_resolution_clock::time_point start;

void timer_start() {
    start = std::chrono::high_resolution_clock::now();
}

void timer_end(const std::string& message) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << message << ": " << duration << " ms\n";
}

const std::filesystem::path& GetWorkingDir()
{
    static const std::filesystem::path dir = [] {
        HMODULE hModule = nullptr;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            (LPCTSTR)&GetWorkingDir,
            &hModule
        );

        wchar_t path[MAX_PATH];
        GetModuleFileNameW(hModule, path, MAX_PATH);

        return std::filesystem::path(path).parent_path().parent_path();
    }();

    return dir;
}

void simple_click() {
    INPUT input[2] = {};

    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(2, input, sizeof(INPUT));
}
void click_at(int x, int y) {
    SetCursorPos(x, y);
    simple_click();
}