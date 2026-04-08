#include "utils.h"
#include <iostream>
#include <windows.h>

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

    std::cout << "Working directory: " << dir << std::endl;
    return dir;
}