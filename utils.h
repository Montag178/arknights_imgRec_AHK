#pragma once
#include <filesystem>
#include <string>

void timer_start();
void timer_end(const std::string& message = "");
const std::filesystem::path& GetWorkingDir();
void simple_click();
void click_at(int x, int y);
