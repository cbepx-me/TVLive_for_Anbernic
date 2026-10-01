#pragma once
#include <string>
#include <map>
#include <tuple>
#include <utility>

namespace tv {

inline constexpr const char* VERSION = "1.0.2";

// 板卡型号 -> 内部编号
inline const std::map<std::string, int> BOARD_MAPPING = {
    {"RGcubexx", 1}, {"RG34xx", 2}, {"RG34xxSP", 2}, {"RGSP", 2},
    {"RG28xx", 3}, {"RG35xx+_P", 4}, {"RG35xxH", 5}, {"RG35xxSP", 6},
    {"RG40xxH", 7}, {"RG40xxV", 8}, {"RG35xxPRO", 9},
};

// 内部编号 -> 屏幕分辨率（宽, 高）
inline const std::map<int, std::tuple<int, int>> SCREEN_RES = {
    {1, {576, 576}}, {2, {720, 480}}, {3, {640, 480}}, {4, {640, 480}},
    {5, {640, 480}}, {6, {640, 480}}, {7, {640, 480}}, {8, {640, 480}},
    {9, {640, 480}},
};

// Linux input event code -> 可读键名
inline const std::map<int, std::string> KEYMAP = {
    {304, "A"}, {305, "B"}, {306, "Y"}, {307, "X"},
    {308, "L1"}, {309, "R1"}, {314, "L2"}, {315, "R2"},
    {17, "DY"}, {16, "DX"},
    {310, "SELECT"}, {311, "START"}, {312, "MENUF"},
    {114, "V-"}, {115, "V+"},
};

// 颜色（与 Python 版一致）
inline constexpr const char* COLOR_BG          = "#0C121C";
inline constexpr const char* COLOR_BG_GRADIENT = "#182038";
inline constexpr const char* COLOR_TEXT        = "#E0E8F0";

// 启动时确定，main 里会填充
extern std::string APP_PATH;
extern std::string FONT_FILE;

std::pair<int, int> detect_screen_size();

// 在 main 开头调用一次，用于初始化 APP_PATH / FONT_FILE
void init_paths();

} // namespace tv