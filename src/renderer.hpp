#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <tuple>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;
struct _TTF_Font;
using TTF_Font = _TTF_Font;

namespace tv {

// 颜色辅助：内存布局 = RGBA（匹配 SDL_PIXELFORMAT_RGBA32）
inline constexpr uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

class UIRenderer {
public:
    UIRenderer(int x_size, int y_size, const std::string& font_path);
    ~UIRenderer();

    bool ok() const { return window_ != nullptr && renderer_ != nullptr; }

    int width()  const { return x_size_; }
    int height() const { return y_size_; }

    // 单色填充整屏
    void clear(uint32_t color);

    // 半开区间 [x0, x1) × [y0, y1) 填充单色
    void fill_rect(int x0, int y0, int x1, int y1, uint32_t color);

    // 单点写入
    void set_pixel(int x, int y, uint32_t color);

    // 垂直渐变：y 从 y0 到 y1，颜色从 top 到 bottom 线性插值
    void fill_gradient_v(int x0, int y0, int x1, int y1,
                         uint32_t top, uint32_t bottom);

    // 圆角矩形：[x0, x1) × [y0, y1)，radius 是四角半径
    void fill_rounded_rect(int x0, int y0, int x1, int y1,
                           int radius, uint32_t color);

    // 实心圆
    void fill_circle(int cx, int cy, int radius, uint32_t color);

    // 颜色线性插值
    static uint32_t lerp_color(uint32_t c1, uint32_t c2, float t);

    // 把图片等比缩放到不超过 max_w × max_h 的框内，居中放在 (cx, cy)
    void draw_image_fit(SDL_Surface* surf, int cx, int cy, int max_w, int max_h);

    // 绘制文字
    // anchor 是两字符：
    //   水平: l / c / r
    //   垂直: t / m / b
    //   例："lt" 左上, "mm" 居中, "rm" 右中, "mb" 中下
    void text(int x, int y, const std::string& s, int font_size = 22,
              uint32_t color = 0xFFFFFFFF,
              const std::string& anchor = "lt",
              bool shadow = false);

    // 测量文字尺寸（返回宽度, 高度）
    std::tuple<int, int> measure_text(const std::string& s, int font_size);

    // 上传到屏幕
    void paint();

    // 销毁并重建 SDL 窗口 + 渲染器（用于 mpv 退出后抢回 framebuffer）
    void recreate_all();

    // 当外部程序（如 mpv）退出后，强制重建 SDL 渲染器，抢回 framebuffer
    void reset_renderer();

private:
    TTF_Font* get_font(int size);
    void      blit_surface(SDL_Surface* surf, int dx, int dy, bool force_black = false);
    void blit_surface_scaled(SDL_Surface* src, int dx, int dy, int dw, int dh);

    int                    x_size_ = 0;
    int                    y_size_ = 0;
    std::vector<uint8_t>   pixels_;
    std::string            font_path_;
    std::map<int, TTF_Font*> fonts_;

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace tv