#pragma once
#include <string>
#include <sys/types.h>

namespace tv {

class TVPlayer {
public:
    TVPlayer();
    ~TVPlayer();

    // 播放指定 URL。sub_file 为空则不加载字幕
    void play(const std::string& url, const std::string& sub_file = "");

    // 停止播放
    void stop();

    // mpv 是否还在跑
    bool is_alive();

    // 音量（0-130）
    int  volume() const { return volume_; }
    void set_volume(int vol);

    // 状态
    const std::string& status() const { return status_; }
    const std::string& fail_reason() const { return fail_reason_; }

    // 记录最后一次请求的 URL（用于防连发）
    const std::string& last_url() const { return last_url_; }

private:
    void ipc_set_volume(int vol);

    pid_t       pid_ = -1;
    std::string status_ = "idle";       // idle / connecting / playing / failed
    std::string fail_reason_;
    std::string ipc_socket_;
    std::string last_url_;
    int         volume_ = 80;
    double      last_play_time_ = 0.0;
};

} // namespace tv