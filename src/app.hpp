#pragma once
#include <string>
#include <vector>

#include "config.hpp"
#include "translator.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "scanner.hpp"
#include "player.hpp"
#include "logo_loader.hpp"

namespace tv {

class TVApp {
public:
    TVApp();
    ~TVApp();

    void run();

private:
    void init();
    void scan_sources();
    void load_source(int idx);
    void update_system_status();

    void handle_input();

    // 音量 OSD
    double volume_osd_timer_ = 0.0;   // 剩余显示时间
    int    volume_osd_value_ = 0;     // 显示的数值

    void change_channel(int delta);
    void change_source(int delta);
    void play_selected();
    int  page_size() const;

    void draw();
    void draw_top_bar();
    void draw_left_list();
    void draw_right_panel();

    // 模块
    InputHandler input_;
    UIRenderer   ui_;
    TVPlayer     player_;
    Translator   translator_;
    LogoLoader   logo_;

    // 数据
    std::vector<Source>  sources_;
    int                  current_source_idx_  = 0;
    std::vector<Channel> current_channels_;
    int                  current_channel_idx_ = 0;
    int                  playing_channel_idx_ = -1;

    // 语言
    std::string language_ = "zh_CN";
    const std::vector<std::string> system_langs_ = {
        "zh_CN", "zh_TW", "en_US", "ja_JP", "ko_KR",
        "es_LA", "ru_RU", "de_DE", "fr_FR", "pt_BR"
    };

    // 系统状态
    bool         wifi_connected_   = false;
    std::string  wifi_essid_;
    int          battery_level_    = 0;
    bool         battery_charging_ = false;
    double       status_timer_     = 0.0;

    // 循环
    bool running_ = true;

    // 无操作熄屏
    static constexpr double IDLE_TIMEOUT = 10.0;   // 秒
    double idle_timer_ = 0.0;
    bool   screen_off_ = false;
    
    // 配置文件
    std::string config_file_;
    void load_config();
    void save_config();
};

} // namespace tv