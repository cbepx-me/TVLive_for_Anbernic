#include "config.hpp"
#include <unistd.h>
#include <limits.h>
#include <cstdlib>
#include <fstream>
#include <utility>

namespace tv {

std::string APP_PATH;
std::string FONT_FILE;

// 在 main 开头调用一次
void init_paths() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::string p(buf);
        auto slash = p.find_last_of('/');
        APP_PATH = (slash == std::string::npos) ? "." : p.substr(0, slash);
    } else {
        APP_PATH = ".";
    }

    // 先看程序目录下有没有 font/font.ttf，没有再用系统的
    std::string local = APP_PATH + "/font/font.ttf";
    if (access(local.c_str(), R_OK) == 0) {
        FONT_FILE = local;
    } else {
        FONT_FILE = "/mnt/vendor/bin/default.ttf";
    }
}

std::pair<int, int> detect_screen_size() {
    // 1) 读板卡型号
    std::string board;
    std::ifstream f("/mnt/vendor/oem/board.ini");
    if (f.good()) std::getline(f, board);
    while (!board.empty() &&
           (board.back() == '\n' || board.back() == '\r' || board.back() == ' '))
        board.pop_back();

    if (board.empty()) board = "RG35xxH";     // 回退

    // 2) 型号 -> 编号
    int hw = 5;
    auto bit = BOARD_MAPPING.find(board);
    if (bit != BOARD_MAPPING.end()) hw = bit->second;

    // 3) 编号 -> 分辨率
    auto rit = SCREEN_RES.find(hw);
    if (rit == SCREEN_RES.end()) return {640, 480};

    return { std::get<0>(rit->second), std::get<1>(rit->second) };
}

} // namespace tv