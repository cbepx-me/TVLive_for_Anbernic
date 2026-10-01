#include "player.hpp"
#include "config.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>
#include <cerrno>

namespace tv {

TVPlayer::TVPlayer() {
    // 每个进程独立 socket 名，避免多实例冲突
    char buf[64];
    std::snprintf(buf, sizeof(buf), "/tmp/tv-mpv-%d.sock", (int)getpid());
    ipc_socket_ = buf;

    // 清理上次可能遗留的
    ::unlink(ipc_socket_.c_str());
}

TVPlayer::~TVPlayer() {
    stop();
}

// ---------- 音量 ----------
void TVPlayer::set_volume(int vol) {
    if (vol < 0)   vol = 0;
    if (vol > 130) vol = 130;
    volume_ = vol;

    // 通过 IPC 通知正在跑的 mpv
    if (pid_ > 0 && is_alive()) {
        ipc_set_volume(volume_);
    }
}

void TVPlayer::ipc_set_volume(int vol) {
    if (ipc_socket_.empty()) return;

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, ipc_socket_.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        std::ostringstream oss;
        oss << "{\"command\":[\"set_property\",\"volume\"," << vol << "]}\n";
        std::string payload = oss.str();
        ssize_t w = ::write(fd, payload.data(), payload.size());
        (void)w;
        std::fprintf(stderr, "[Player] mpv ipc set volume -> %d\n", vol);
    } else {
        std::fprintf(stderr, "[Player] ipc connect failed: %s\n", std::strerror(errno));
    }
    ::close(fd);
}

// ---------- 播放 ----------
void TVPlayer::play(const std::string& url, const std::string& sub_file) {
    // 防连发：同一 URL 在 1 秒内重复调用直接忽略
    double now = (double)time(nullptr);
    if (url == last_url_ && now - last_play_time_ < 1.0) {
        std::fprintf(stderr, "[Player] skip duplicate play\n");
        return;
    }

    stop();

    // 清理旧 socket
    ::unlink(ipc_socket_.c_str());

    status_ = "connecting";
    fail_reason_.clear();
    last_url_ = url;
    last_play_time_ = now;

    // 组装参数
    std::vector<std::string> args = {
        "mpv",
        "--no-osc",
        "--fullscreen",
        "--volume=" + std::to_string(volume_),
        "--input-ipc-server=" + ipc_socket_,
        "--network-timeout=20",
        "--cache=yes",
        "--cache-secs=10",
        "--stream-lavf-o=reconnect=1",
        "--user-agent=Mozilla/5.0",
    };
    if (!sub_file.empty()) {
        args.push_back("--sub-file=" + sub_file);
    }
    args.push_back(url);

    std::fprintf(stderr, "[Player] starting mpv: url=%s volume=%d\n",
                 url.c_str(), volume_);

    pid_ = fork();
    if (pid_ < 0) {
        status_ = "failed";
        fail_reason_ = "fork failed";
        std::fprintf(stderr, "[Player] fork failed: %s\n", std::strerror(errno));
        return;
    }

    if (pid_ == 0) {
        // 子进程：exec mpv
        std::vector<char*> argv;
        for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        // 让 mpv 输出走 stderr，方便调试；重定向 stdout 到 /dev/null
        FILE* fp = freopen("/dev/null", "w", stdout);
        (void)fp;
        execvp("mpv", argv.data());
        // execvp 失败
        std::fprintf(stderr, "[Player] execvp mpv failed: %s\n", std::strerror(errno));
        _exit(127);
    }

    // 父进程
    // 略等一下，看是不是立刻退出了
    usleep(300 * 1000);   // 300ms
    int st = 0;
    pid_t r = waitpid(pid_, &st, WNOHANG);
    if (r == pid_) {
        // 立刻退出 = 启动失败
        status_ = "failed";
        fail_reason_ = "mpv exited immediately";
        std::fprintf(stderr, "[Player] mpv exited immediately, status=%d\n",
                     WEXITSTATUS(st));
        pid_ = -1;
        return;
    }
    status_ = "playing";
}

void TVPlayer::stop() {
    if (pid_ > 0) {
        ::kill(pid_, SIGTERM);
        // 等最多 1 秒
        for (int i = 0; i < 20; i++) {
            int st = 0;
            pid_t r = waitpid(pid_, &st, WNOHANG);
            if (r == pid_ || r < 0) break;
            usleep(50 * 1000);
        }
        // 还没退就强杀
        ::kill(pid_, SIGKILL);
        waitpid(pid_, nullptr, 0);
        pid_ = -1;
        status_ = "idle";
        std::fprintf(stderr, "[Player] stopped\n");
    }
    ::unlink(ipc_socket_.c_str());
}

bool TVPlayer::is_alive() {
    if (pid_ <= 0) return false;
    int st = 0;
    pid_t r = waitpid(pid_, &st, WNOHANG);
    if (r == pid_) {
        // 已退出
        pid_ = -1;
        if (status_ == "playing") status_ = "failed";
        return false;
    }
    if (r < 0) {
        // 出错（多半是已经收过尸）
        pid_ = -1;
        return false;
    }
    return true;   // r == 0 还在跑
}

} // namespace tv