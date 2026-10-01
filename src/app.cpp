#include "app.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <ctime>

namespace tv {

TVApp::TVApp()
    : input_(),
      ui_(tv::detect_screen_size().first,
          tv::detect_screen_size().second,
          tv::FONT_FILE),
      logo_(tv::APP_PATH + "/logo_cache") {

    config_file_ = tv::APP_PATH + "/tv.ini";
    load_config();
    init();
}

TVApp::~TVApp() {
    player_.stop();
}

// 极简 INI 读取：找 [section] 下的 key=value，找不到返回 def
static std::string ini_read(const std::string& path,
                            const std::string& section,
                            const std::string& key,
                            const std::string& def = "") {
    std::ifstream f(path);
    if (!f.good()) return def;

    std::string line, cur;
    std::string sec_hdr = "[" + section + "]";
    std::string kpat    = key + "=";

    while (std::getline(f, line)) {
        // trim
        size_t b = line.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(b, e - b + 1);

        if (!line.empty() && line[0] == '[') {
            cur = line;
            continue;
        }
        if (cur == sec_hdr && line.rfind(kpat, 0) == 0) {
            return line.substr(kpat.size());
        }
    }
    return def;
}

// 极简 INI 写入：若文件不存在则新建；若 key 存在则替换；否则在文件末尾追加
static void ini_write(const std::string& path,
                      const std::string& section,
                      const std::string& key,
                      const std::string& value) {
    std::vector<std::string> lines;
    {
        std::ifstream f(path);
        std::string l;
        while (std::getline(f, l)) lines.push_back(l);
    }

    std::string sec_hdr = "[" + section + "]";
    std::string kpat    = key + "=";

    int sec_idx   = -1;
    int key_idx   = -1;
    int next_sec  = -1;
    std::string cur;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string s = lines[i];
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) continue;
        size_t e = s.find_last_not_of(" \t\r\n");
        s = s.substr(b, e - b + 1);

        if (!s.empty() && s[0] == '[') {
            if (cur == sec_hdr && next_sec < 0 && sec_idx >= 0) next_sec = (int)i;
            cur = s;
            if (cur == sec_hdr && sec_idx < 0) sec_idx = (int)i;
            continue;
        }
        if (cur == sec_hdr && s.rfind(kpat, 0) == 0) {
            key_idx = (int)i;
            break;
        }
    }

    if (key_idx >= 0) {
        lines[key_idx] = key + "=" + value;
    } else if (sec_idx >= 0) {
        // 在 section 内追加
        int insert_at = (next_sec >= 0) ? next_sec : (int)lines.size();
        lines.insert(lines.begin() + insert_at, key + "=" + value);
    } else {
        // 新建 section
        if (!lines.empty()) lines.push_back("");
        lines.push_back(sec_hdr);
        lines.push_back(key + "=" + value);
    }

    std::ofstream f(path);
    for (auto& l : lines) f << l << "\n";
}

void TVApp::init() {
    translator_.load(language_);
    scan_sources();
    update_system_status();
    std::fprintf(stderr, "[App] screen %dx%d\n", ui_.width(), ui_.height());
}

void TVApp::load_config() {
    // 语言
    // 1) /mnt/vendor/oem/language.ini 里的系统索引
    language_ = "";
    {
        std::ifstream f("/mnt/vendor/oem/language.ini");
        if (f.good()) {
            int idx = -1;
            f >> idx;
            if (idx >= 0 && idx < (int)system_langs_.size()) {
                language_ = system_langs_[idx];
                std::fprintf(stderr, "[App] system language index %d -> %s\n",
                             idx, language_.c_str());
            } else {
                std::fprintf(stderr, "[App] language.ini invalid index %d\n", idx);
            }
        }
    }

    // 2) 兜底 en_US
    if (language_.empty()) language_ = "en_US";

    // 音量
    std::string vs = ini_read(config_file_, "Volume", "value", "");
    if (!vs.empty()) {
        int v = std::atoi(vs.c_str());
        if (v >= 0 && v <= 130) player_.set_volume(v);
    }
    // 恢复位置
    std::string ss = ini_read(config_file_, "Resume", "source_index", "");
    if (!ss.empty()) {
        int s = std::atoi(ss.c_str()) - 1;
        if (s >= 0) current_source_idx_ = s;
    }
    std::string cs = ini_read(config_file_, "Resume", "channel_index", "");
    if (!cs.empty()) {
        int c = std::atoi(cs.c_str()) - 1;
        if (c >= 0) current_channel_idx_ = c;
    }
    std::fprintf(stderr, "[App] config: lang=%s src=%d ch=%d vol=%d\n",
                 language_.c_str(), current_source_idx_, current_channel_idx_,
                 player_.volume());
}

void TVApp::save_config() {
    ini_write(config_file_, "Volume",  "value",    std::to_string(player_.volume()));
    ini_write(config_file_, "Resume",  "source_index",  std::to_string(current_source_idx_ + 1));
    ini_write(config_file_, "Resume",  "channel_index", std::to_string(current_channel_idx_ + 1));
}

void TVApp::scan_sources() {
    sources_ = TVScanner::find_sources();
    if (sources_.empty()) {
        std::fprintf(stderr, "[App] no sources\n");
        return;
    }
    if (current_source_idx_ < 0 || current_source_idx_ >= (int)sources_.size())
        current_source_idx_ = 0;
    load_source(current_source_idx_);
}

void TVApp::load_source(int idx) {
    if (idx < 0 || idx >= (int)sources_.size()) return;
    current_channels_ = TVScanner::scan_source(sources_[idx]);
    current_source_idx_ = idx;
    current_channel_idx_ = 0;
    std::fprintf(stderr, "[App] loaded source %s (%zu channels)\n",
                 sources_[idx].name.c_str(), current_channels_.size());
}

void TVApp::update_system_status() {
    battery_level_ = 0;
    const char* paths[] = {
        "/sys/class/power_supply/battery/capacity",
        "/sys/class/power_supply/BAT0/capacity",
        "/sys/class/power_supply/axp2202-battery/capacity",
    };
    for (auto p : paths) {
        std::ifstream f(p);
        if (f.good()) { f >> battery_level_; break; }
    }

    wifi_connected_ = false;
    wifi_essid_.clear();
    {
        std::ifstream f("/sys/class/net/wlan0/operstate");
        std::string line;
        if (f.good() && std::getline(f, line)) {
            wifi_connected_ = (line == "up");
        }
    }
    if (wifi_connected_) {
        FILE* p = popen("iw dev wlan0 link 2>/dev/null", "r");
        if (p) {
            char buf[512];
            while (fgets(buf, sizeof(buf), p)) {
                std::string s = buf;
                auto pos = s.find("SSID:");
                if (pos != std::string::npos) {
                    std::string ssid = s.substr(pos + 5);
                    while (!ssid.empty() && (ssid.back() == '\n' || ssid.back() == '\r' || ssid.back() == ' '))
                        ssid.pop_back();
                    size_t b = ssid.find_first_not_of(" \t");
                    if (b != std::string::npos) ssid = ssid.substr(b);
                    wifi_essid_ = ssid;
                    break;
                }
            }
            pclose(p);
        }
    }
}

// ---------- 输入 ----------
void TVApp::handle_input() {
    input_.poll();
    if (input_.code_name().empty()) return;

    const std::string& k = input_.code_name();

    // 熄屏状态：只认 B / X，且只唤醒不执行
    if (screen_off_) {
        if (k == "B" || k == "X") {
            screen_off_ = false;
            idle_timer_  = 0.0;
        }
        input_.reset();
        return;
    }

    // 亮屏状态：有任意按键 → 重置 idle
    idle_timer_ = 0.0;

    int val = input_.value();

    if (k == "SELECT") {
        running_ = false;
    } else if (k == "V+") {
        player_.set_volume(player_.volume() + 5);
        if (player_.status() != "playing") {
            volume_osd_timer_ = 1.5;
            volume_osd_value_ = player_.volume();
        }
        save_config();
    } else if (k == "V-") {
        player_.set_volume(player_.volume() - 5);
        if (player_.status() != "playing") {
            volume_osd_timer_ = 1.5;
            volume_osd_value_ = player_.volume();
        }
        save_config();
    } else if (k == "L1" || k == "X") {
        change_source(-1);
    } else if (k == "R1") {
        change_source(1);
    } else if (k == "DY") {
        if (val == -1)      change_channel(-1);
        else if (val == 1)  change_channel(1);
    } else if (k == "DX") {
        int page = page_size();
        if (val == -1)      change_channel(-page);
        else if (val == 1)  change_channel(page);
    } else if (k == "A") {
        if (player_.status() != "playing" || playing_channel_idx_ != current_channel_idx_)
            play_selected();
    } else if (k == "B") {
        player_.stop();
        playing_channel_idx_ = -1;
        ui_.recreate_all();
    } else if (k == "Y") {
        std::fprintf(stderr, "[App] refresh sources\n");
        scan_sources();
        if (player_.status() == "failed") {
            player_.stop();
        }
    }

    input_.reset();
}

void TVApp::change_channel(int delta) {
    if (current_channels_.empty()) return;
    int n = (int)current_channels_.size();
    current_channel_idx_ = ((current_channel_idx_ + delta) % n + n) % n;
    if (player_.is_alive()) play_selected();
    save_config();
}

void TVApp::change_source(int delta) {
    if (sources_.empty()) return;
    int n = (int)sources_.size();
    current_source_idx_ = ((current_source_idx_ + delta) % n + n) % n;
    load_source(current_source_idx_);
    if (player_.is_alive()) {
        player_.stop();
        playing_channel_idx_ = -1;
    }
    save_config();
}

void TVApp::play_selected() {
    if (current_channels_.empty()) return;
    auto& c = current_channels_[current_channel_idx_];
    player_.play(c.url);
    if (player_.status() == "playing") playing_channel_idx_ = current_channel_idx_;
    else                                playing_channel_idx_ = -1;
}

int TVApp::page_size() const {
    int top_h = 44;       // 顶部状态栏
    int bot_h = 44;       // 底部提示栏
    int item_h = 28;      // 每个频道项高度

    int ly           = top_h + 8;         // 左侧面板 y 起点（和 draw_left_list 一致）
    int list_top     = ly + 54;           // 频道列表起点（源信息占 54px）
    int bottom_limit = ui_.height() - bot_h - 16;   // 面板底部（-16 是内边距）

    int avail = bottom_limit - list_top;
    int n = avail / item_h;
    if (n < 1) n = 1;
    return n;
}

// ---------- 绘制 ----------
void TVApp::draw() {
    int W = ui_.width(), H = ui_.height();

    // ---------- 熄屏模式 ----------
    if (screen_off_) {
        ui_.clear(tv::rgba(0x00, 0x00, 0x00));
        ui_.text(W / 2, H / 2,
                 translator_.t("Press B or X to wake"),
                 28, tv::rgba(0x3A, 0x3A, 0x3A), "mm");
        ui_.paint();
        return;
    }

    ui_.fill_gradient_v(0, 0, W, H,
                        tv::rgba(0x18, 0x20, 0x38),
                        tv::rgba(0x0C, 0x12, 0x1C));
    draw_top_bar();
    draw_left_list();
    draw_right_panel();

    // ---------- 音量 OSD 叠加 ----------
    if (volume_osd_timer_ > 0.0) {
        int bw = 260, bh = 60;
        int bx = (W - bw) / 2;
        int by = (H - bh) / 2;
        ui_.fill_rounded_rect(bx, by, bx + bw, by + bh, 12,
                              tv::rgba(0x10, 0x18, 0x2A));

        int inner_x = bx + 16;
        int inner_y = by + bh - 16;
        int inner_w = bw - 32;
        int inner_h = 6;
        ui_.fill_rounded_rect(inner_x, inner_y, inner_x + inner_w, inner_y + inner_h, 3,
                              tv::rgba(0x2A, 0x3A, 0x5A));
        int fw = (int)((double)inner_w * volume_osd_value_ / 130.0);
        if (fw > 0) {
            ui_.fill_rounded_rect(inner_x, inner_y, inner_x + fw, inner_y + inner_h, 3,
                                  tv::rgba(0x4F, 0xC3, 0xF7));
        }

        char buf[32];
        std::snprintf(buf, sizeof(buf), "Volume  %d", volume_osd_value_);
        ui_.text(bx + bw / 2, by + 18, buf, 20,
                 tv::rgba(0xE0, 0xE8, 0xF0), "mm");
    }

    ui_.paint();
}

void TVApp::draw_top_bar() {
    int W = ui_.width();
    int top_h = 44;
    ui_.fill_rect(0, 0, W, top_h, tv::rgba(0x0A, 0x10, 0x20));

    // 左侧标题
    std::string title = std::string(translator_.t("TVLive"))
                      + " v" + tv::VERSION;
    ui_.text(12, top_h / 2, title, 18,
             tv::rgba(0xE0, 0xE8, 0xF0), "lm");

    // ---------------- 右侧三块信息：从右往左排列 ----------------
    const int right_edge = W - 12;
    const int gap_px     = 16;
    const int y_mid      = top_h / 2;

    int cursor = right_edge;

    // ---- 1) 电池（最右） ----
    {
        char bat[32];
        std::snprintf(bat, sizeof(bat), "%d%%", battery_level_);
        uint32_t bat_col = battery_level_ >= 60 ? tv::rgba(0x4F, 0xC3, 0xF7)
                         : battery_level_ >= 20 ? tv::rgba(0x64, 0xF6, 0xA6)
                                                : tv::rgba(0xEF, 0x53, 0x50);
        int fs = 18;
        ui_.text(cursor, y_mid, bat, fs, bat_col, "rm");
        auto [tw, th] = ui_.measure_text(bat, fs);
        cursor -= tw + gap_px;
    }

    // ---- 2) 时间（电池左边） ----
    {
        char time_str[16];
        std::time_t now_t = std::time(nullptr);
        std::tm tm_buf;
        localtime_r(&now_t, &tm_buf);
        std::strftime(time_str, sizeof(time_str), "%H:%M", &tm_buf);

        int fs = 18;
        ui_.text(cursor, y_mid, time_str, fs,
                 tv::rgba(0xE0, 0xE8, 0xF0), "rm");
        auto [tw, th] = ui_.measure_text(time_str, fs);
        cursor -= tw + gap_px;
    }

    // ---- 3) WiFi（时间左边） ----
    {
        std::string wifi;
        if (wifi_connected_) {
            wifi = wifi_essid_.empty() ? "WiFi" : ("WiFi: " + wifi_essid_);
        } else {
            wifi = "WiFi ×";
        }
        // 太长就截断，避免挤掉标题
        if (wifi.size() > 22) wifi = wifi.substr(0, 20) + "…";

        uint32_t wifi_col = wifi_connected_ ? tv::rgba(0x4F, 0xC3, 0xF7)
                                            : tv::rgba(0xEF, 0x53, 0x50);
        ui_.text(cursor, y_mid, wifi, 16, wifi_col, "rm");
    }
}

void TVApp::draw_left_list() {
    int W = ui_.width(), H = ui_.height();
    int top_h = 44, bot_h = 44;

    int lx = 12, ly = top_h + 8;
    int lw = (int)(W * 0.40);
    int lh = H - ly - bot_h - 16;

    ui_.fill_rounded_rect(lx, ly, lx + lw, ly + lh, 8, tv::rgba(0x0F, 0x1A, 0x2E));

    std::string src_name = sources_.empty() ? "no source"
                                            : sources_[current_source_idx_].name;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "● %s (%zu)", src_name.c_str(),
                  current_channels_.size());
    ui_.text(lx + 12, ly + 8, buf, 18, tv::rgba(0x64, 0xB5, 0xF6), "lt");
    std::snprintf(buf, sizeof(buf), "%d/%zu %s",
                  current_source_idx_ + 1, sources_.size(),
                  translator_.t("Source").c_str());
    ui_.text(lx + 12, ly + 34, buf, 14, tv::rgba(0x7A, 0x8B, 0xA0), "lt");

    // 频道列表
    int item_h = 28;
    int list_top = ly + 54;
    int visible = page_size();
    int start = current_channel_idx_ - visible / 2;
    if (start < 0) start = 0;
    if (current_channels_.size() > (size_t)visible &&
        start > (int)current_channels_.size() - visible)
        start = (int)current_channels_.size() - visible;

    for (int i = 0; i < visible; i++) {
        int idx = start + i;
        if (idx >= (int)current_channels_.size()) break;

        int y = list_top + i * item_h;
        if (idx == current_channel_idx_) {
            ui_.fill_rounded_rect(lx + 6, y, lx + lw - 6, y + item_h - 2, 4,
                                  tv::rgba(0x1E, 0x88, 0xE5));
        }
        uint32_t col = (idx == current_channel_idx_) ? tv::rgba(0xFF, 0xFF, 0xFF)
                                                     : tv::rgba(0xB0, 0xC4, 0xDE);
        auto& c = current_channels_[idx];
        std::string label = std::to_string(idx + 1) + " " + c.name;
        if (c.line_total > 1)
            label += "  " + std::to_string(c.line_no) + "/" + std::to_string(c.line_total);
        ui_.text(lx + 12, y + 4, label, 16, col, "lt");
    }
}

void TVApp::draw_right_panel() {
    int W = ui_.width(), H = ui_.height();
    int top_h = 44, bot_h = 44;

    int lx = 12;
    int lw = (int)(W * 0.40);
    int ly = top_h + 8;
    int lh = H - ly - bot_h - 16;

    int rx = lx + lw + 12;
    int rw = W - rx - 12;
    int rh = lh;

    ui_.fill_rounded_rect(rx, ly, rx + rw, ly + rh, 8, tv::rgba(0x0F, 0x1A, 0x2E));

    if (current_channels_.empty()) {
        ui_.text(rx + rw / 2, ly + rh / 2, translator_.t("No channels"), 22,
                 tv::rgba(0x7A, 0x8B, 0xA0), "mm");
        return;
    }

    auto& cur = current_channels_[current_channel_idx_];

    // ---------- 台标框 ----------
    int pad = 14;
    int logo_w = rw - pad * 2;
    int logo_h = std::min(160, rh - 130);
    int logo_x = rx + pad;
    int logo_y = ly + pad;

    ui_.fill_rounded_rect(logo_x, logo_y, logo_x + logo_w, logo_y + logo_h, 10,
                          tv::rgba(0x22, 0x2E, 0x44));

    if (!cur.logo.empty()) {
        SDL_Surface* img = logo_.get(cur.logo, logo_w - 16, logo_h - 16);
        if (img) {
            ui_.draw_image_fit(img, logo_x + logo_w / 2, logo_y + logo_h / 2,
                               logo_w - 16, logo_h - 16);
        } else {
            std::string msg =
                logo_.is_failed(cur.logo) ? "failed" :
                logo_.is_loading(cur.logo) ? translator_.t("loading...") : "queued";
            ui_.text(logo_x + logo_w / 2, logo_y + logo_h / 2, msg, 14,
                     tv::rgba(0x7A, 0x8B, 0xA0), "mm");
        }
    } else {
        ui_.text(logo_x + logo_w / 2, logo_y + logo_h / 2, translator_.t("no logo"), 14,
                 tv::rgba(0x7A, 0x8B, 0xA0), "mm");
    }

    // ---------- 频道名 + 分组 ----------
    int info_y = logo_y + logo_h + 20;
    ui_.text(rx + rw / 2, info_y, cur.name, 22,
             tv::rgba(0xE0, 0xE8, 0xF0), "mm");

    std::string meta = translator_.t("Group:") + " " + cur.group;
    if (cur.line_total > 1)
        meta += "   " + translator_.t("Line:") + " "
              + std::to_string(cur.line_no) + "/" + std::to_string(cur.line_total);
    ui_.text(rx + rw / 2, info_y + 26, meta, 14,
             tv::rgba(0x7A, 0x8B, 0xA0), "mm");

    // ---------- A Play 按钮（同时作为状态显示） ----------
    int btn_w = 200, btn_h = 40;
    int btn_x = rx + (rw - btn_w) / 2;
    int btn_y = ly + rh - 60;

    uint32_t btn_col = tv::rgba(0x1E, 0x3A, 0x5F);
    std::string btn_txt = "A " + translator_.t("Play");
    uint32_t btn_txt_col = tv::rgba(0xB0, 0xC4, 0xDE);

    if (player_.status() == "playing") {
        btn_col = tv::rgba(0x2E, 0x7D, 0x32);
        btn_txt = "Playing";
        btn_txt_col = tv::rgba(0xFF, 0xFF, 0xFF);
    } else if (player_.status() == "connecting") {
        btn_col = tv::rgba(0x9E, 0x7D, 0x1E);
        btn_txt = "Connecting...";
        btn_txt_col = tv::rgba(0xFF, 0xFF, 0xFF);
    } else if (player_.status() == "failed") {
        btn_col = tv::rgba(0x8E, 0x2E, 0x2E);
        btn_txt = "Playback failed";
        btn_txt_col = tv::rgba(0xFF, 0xFF, 0xFF);
    }

    ui_.fill_rounded_rect(btn_x, btn_y, btn_x + btn_w, btn_y + btn_h, 8, btn_col);
    ui_.text(btn_x + btn_w / 2, btn_y + btn_h / 2, translator_.t(btn_txt), 18,
             btn_txt_col, "mm");

    // ---------- 常驻音量条（放在按钮上方，紧贴按钮） ----------
    int vb_w = rw - 80;
    int vb_h = 8;
    int vb_x = rx + (rw - vb_w) / 2;
    int vb_y = btn_y - 26;

    ui_.fill_rounded_rect(vb_x, vb_y, vb_x + vb_w, vb_y + vb_h, 4,
                          tv::rgba(0x2A, 0x3A, 0x5A));

    int vol_max = 130;
    int vol_now = player_.volume();
    int fill_w = (int)((double)vb_w * vol_now / vol_max);
    if (fill_w > 0) {
        uint32_t fill_col = vol_now < 50 ? tv::rgba(0x4C, 0xAF, 0x50)
                          : vol_now < 90 ? tv::rgba(0xFF, 0xEB, 0x3B)
                                         : tv::rgba(0xFF, 0x57, 0x22);
        ui_.fill_rounded_rect(vb_x, vb_y, vb_x + fill_w, vb_y + vb_h, 4, fill_col);
    }

    char vbuf[16];
    std::snprintf(vbuf, sizeof(vbuf), "Vol %d", vol_now);
    ui_.text(vb_x, vb_y - 14, vbuf, 12,
             tv::rgba(0x7A, 0x8B, 0xA0), "lb");

    // ---------- 底部提示 ----------
    ui_.fill_rect(0, H - bot_h, W, H, tv::rgba(0x0A, 0x10, 0x20));
    std::string hint = "↑↓ " + translator_.t("Channel")
                     + "  ←→ " + translator_.t("Page")
                     + "  L1/R1 " + translator_.t("Source")
                     + "  A " + translator_.t("Play")
                     + "  B " + translator_.t("Stop")
                     + "  Y " + translator_.t("Refresh")
                     + "  SEL " + translator_.t("Exit");
    ui_.text(12, H - bot_h + 4, hint, 14,
             tv::rgba(0xB0, 0xC4, 0xDE), "lt");
}

// ---------- 主循环 ----------
void TVApp::run() {
    auto last = std::chrono::steady_clock::now();
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;

        handle_input();
        logo_.process_pending();

        // 音量 OSD 计时
        if (volume_osd_timer_ > 0.0) {
            volume_osd_timer_ -= dt;
            if (volume_osd_timer_ < 0.0) volume_osd_timer_ = 0.0;
        }

        // 系统状态刷新（每 8 秒）
        status_timer_ += dt;
        if (status_timer_ > 8.0) {
            status_timer_ = 0;
            update_system_status();
        }

        // 无操作熄屏（10 秒）—— 播放中不熄屏
        if (player_.status() == "playing") {
            idle_timer_ = 0.0;
            screen_off_ = false;
        } else {
            idle_timer_ += dt;
            if (idle_timer_ > IDLE_TIMEOUT) {
                screen_off_ = true;
                idle_timer_ = IDLE_TIMEOUT;
            }
        }

        // mpv 异常退出
        if (player_.status() == "playing" && !player_.is_alive()) {
            playing_channel_idx_ = -1;
        }

        draw();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    save_config();
}

} // namespace tv