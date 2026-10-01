#include "renderer.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <cstdio>
#include <algorithm>
#include <cmath>

namespace tv {

UIRenderer::UIRenderer(int x_size, int y_size, const std::string& font_path)
    : x_size_(x_size), y_size_(y_size), font_path_(font_path) {

    pixels_.assign((size_t)x_size_ * y_size_ * 4, 0);
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "[UI] SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "[UI] TTF_Init failed: %s\n", TTF_GetError());
        return;
    }
    
    window_ = SDL_CreateWindow(
        "TVLive",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        0, 0,
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::fprintf(stderr, "[UI] CreateWindow failed: %s\n", SDL_GetError());
        return;
    }
    std::fprintf(stderr, "[UI] window created\n");

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        std::fprintf(stderr, "[UI] accel renderer failed, fallback SW\n");
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::fprintf(stderr, "[UI] CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    std::fprintf(stderr, "[UI] %dx%d ready, font=%s\n",
                 x_size_, y_size_, font_path_.c_str());
}

UIRenderer::~UIRenderer() {
    for (auto& kv : fonts_) {
        if (kv.second) TTF_CloseFont(kv.second);
    }
    fonts_.clear();
    TTF_Quit();

    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_Quit();
}

void UIRenderer::clear(uint32_t color) {
    uint32_t* p = reinterpret_cast<uint32_t*>(pixels_.data());
    size_t n = (size_t)x_size_ * y_size_;
    for (size_t i = 0; i < n; i++) p[i] = color;
}

void UIRenderer::set_pixel(int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= x_size_ || y >= y_size_) return;
    uint8_t* p = &pixels_[(size_t)(y * x_size_ + x) * 4];
    p[0] = (color >>  0) & 0xFF;
    p[1] = (color >>  8) & 0xFF;
    p[2] = (color >> 16) & 0xFF;
    p[3] = (color >> 24) & 0xFF;
}

void UIRenderer::fill_rect(int x0, int y0, int x1, int y1, uint32_t color) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > x_size_) x1 = x_size_;
    if (y1 > y_size_) y1 = y_size_;
    if (x0 >= x1 || y0 >= y1) return;

    for (int y = y0; y < y1; y++) {
        uint32_t* row = reinterpret_cast<uint32_t*>(&pixels_[(size_t)y * x_size_ * 4]);
        for (int x = x0; x < x1; x++) row[x] = color;
    }
}

TTF_Font* UIRenderer::get_font(int size) {
    auto it = fonts_.find(size);
    if (it != fonts_.end()) return it->second;

    if (font_path_.empty()) {
        fonts_[size] = nullptr;
        return nullptr;
    }

    TTF_Font* f = TTF_OpenFont(font_path_.c_str(), size);
    if (!f) {
        std::fprintf(stderr, "[UI] TTF_OpenFont(%s, %d) failed: %s\n",
                     font_path_.c_str(), size, TTF_GetError());
    }
    fonts_[size] = f;
    return f;
}

std::tuple<int, int> UIRenderer::measure_text(const std::string& s, int font_size) {
    TTF_Font* f = get_font(font_size);
    if (!f) return {0, 0};
    int w = 0, h = 0;
    if (TTF_SizeUTF8(f, s.c_str(), &w, &h) != 0) return {0, 0};
    return {w, h};
}

// 把 surface（RGBA32）与内部像素缓冲区做 alpha 混合
void UIRenderer::blit_surface(SDL_Surface* surf, int dx, int dy, bool force_black) {
    if (!surf) return;

    SDL_Surface* rgba = surf;
    bool need_free = false;
    if (surf->format->format != SDL_PIXELFORMAT_RGBA32) {
        rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
        need_free = true;
        if (!rgba) return;
    }

    int sw = rgba->w, sh = rgba->h;
    const uint8_t* spix = (const uint8_t*)rgba->pixels;
    int spitch = rgba->pitch;

    for (int y = 0; y < sh; y++) {
        int py = dy + y;
        if (py < 0 || py >= y_size_) continue;
        const uint8_t* srow = spix + y * spitch;
        uint8_t* drow = &pixels_[(size_t)py * x_size_ * 4];

        for (int x = 0; x < sw; x++) {
            int px = dx + x;
            if (px < 0 || px >= x_size_) continue;

            const uint8_t* sp = srow + x * 4;
            uint8_t sa = sp[3];
            if (sa == 0) continue;

            uint8_t sr = force_black ? 0 : sp[0];
            uint8_t sg = force_black ? 0 : sp[1];
            uint8_t sb = force_black ? 0 : sp[2];

            uint8_t* dp = drow + px * 4;
            uint8_t dr = dp[0], dg = dp[1], db = dp[2];

            dp[0] = (sr * sa + dr * (255 - sa)) / 255;
            dp[1] = (sg * sa + dg * (255 - sa)) / 255;
            dp[2] = (sb * sa + db * (255 - sa)) / 255;
            dp[3] = 255;
        }
    }

    if (need_free) SDL_FreeSurface(rgba);
}

uint32_t UIRenderer::lerp_color(uint32_t c1, uint32_t c2, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    uint8_t r1 =  c1        & 0xFF;
    uint8_t g1 = (c1 >>  8) & 0xFF;
    uint8_t b1 = (c1 >> 16) & 0xFF;
    uint8_t r2 =  c2        & 0xFF;
    uint8_t g2 = (c2 >>  8) & 0xFF;
    uint8_t b2 = (c2 >> 16) & 0xFF;
    uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);
    return rgba(r, g, b, 255);
}

void UIRenderer::fill_gradient_v(int x0, int y0, int x1, int y1,
                                 uint32_t top, uint32_t bottom) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > x_size_) x1 = x_size_;
    if (y1 > y_size_) y1 = y_size_;
    if (x0 >= x1 || y0 >= y1) return;

    int h = y1 - y0;
    for (int y = y0; y < y1; y++) {
        float t = (float)(y - y0) / (float)h;
        uint32_t c = lerp_color(top, bottom, t);
        uint32_t* row = reinterpret_cast<uint32_t*>(&pixels_[(size_t)y * x_size_ * 4]);
        for (int x = x0; x < x1; x++) row[x] = c;
    }
}

void UIRenderer::fill_rounded_rect(int x0, int y0, int x1, int y1,
                                   int radius, uint32_t color) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > x_size_) x1 = x_size_;
    if (y1 > y_size_) y1 = y_size_;
    if (x0 >= x1 || y0 >= y1) return;

    // 夹取半径
    int max_r = std::min({radius, (x1 - x0) / 2, (y1 - y0) / 2});
    if (max_r < 1) {
        fill_rect(x0, y0, x1, y1, color);
        return;
    }

    for (int y = y0; y < y1; y++) {
        int left = x0, right = x1;
        int dy_top = (y0 + max_r) - y;
        int dy_bot = y - (y1 - max_r - 1);
        if (dy_top > 0) {
            int dx = (int)std::sqrt((double)max_r * max_r - (double)dy_top * dy_top);
            left  = x0 + max_r - dx;
            right = x1 - max_r + dx;
        } else if (dy_bot > 0) {
            int dx = (int)std::sqrt((double)max_r * max_r - (double)dy_bot * dy_bot);
            left  = x0 + max_r - dx;
            right = x1 - max_r + dx;
        }
        if (left < x0) left = x0;
        if (right > x1) right = x1;
        if (left < right) {
            uint32_t* row = reinterpret_cast<uint32_t*>(&pixels_[(size_t)y * x_size_ * 4]);
            for (int x = left; x < right; x++) row[x] = color;
        }
    }
}

void UIRenderer::fill_circle(int cx, int cy, int radius, uint32_t color) {
    if (radius <= 0) return;
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)std::sqrt((double)(r2 - dy * dy));
        int y = cy + dy;
        if (y < 0 || y >= y_size_) continue;
        int x0 = cx - dx;
        int x1 = cx + dx + 1;
        if (x0 < 0) x0 = 0;
        if (x1 > x_size_) x1 = x_size_;
        if (x0 >= x1) continue;
        uint32_t* row = reinterpret_cast<uint32_t*>(&pixels_[(size_t)y * x_size_ * 4]);
        for (int x = x0; x < x1; x++) row[x] = color;
    }
}

void UIRenderer::text(int x, int y, const std::string& s, int font_size,
                      uint32_t color, const std::string& anchor, bool shadow) {
    if (s.empty()) return;

    TTF_Font* f = get_font(font_size);
    if (!f) return;

    SDL_Color c = {
        (uint8_t)( color        & 0xFF),
        (uint8_t)((color >>  8) & 0xFF),
        (uint8_t)((color >> 16) & 0xFF),
        255
    };
    SDL_Surface* srf = TTF_RenderUTF8_Blended(f, s.c_str(), c);
    if (!srf) return;

    int tw = srf->w, th = srf->h;

    // anchor 计算
    int dx = x, dy = y;
    if (!anchor.empty()) {
        char h = anchor[0];
        char v = (anchor.size() >= 2) ? anchor[1] : 't';
        if      (h == 'l')                dx = x;
        else if (h == 'm' || h == 'c')    dx = x - tw / 2;
        else if (h == 'r')                dx = x - tw;

        if      (v == 't') dy = y;
        else if (v == 'm') dy = y - th / 2;
        else if (v == 'b') dy = y - th;
    }

    // shadow 先画 4 个偏移
    if (shadow) {
        blit_surface(srf, dx + 1, dy + 1, true);
        blit_surface(srf, dx + 1, dy - 1, true);
        blit_surface(srf, dx - 1, dy + 1, true);
        blit_surface(srf, dx - 1, dy - 1, true);
    }
    blit_surface(srf, dx, dy, false);

    SDL_FreeSurface(srf);
}

// 把 src 缩放到 dw × dh 后，alpha 混合到内部缓冲区的 (dx, dy)
void UIRenderer::blit_surface_scaled(SDL_Surface* src,
                                     int dx, int dy, int dw, int dh) {
    if (!src || dw <= 0 || dh <= 0) return;

    // 1) 统一成 RGBA32
    SDL_Surface* rgba = src;
    bool need_free_rgba = false;
    if (src->format->format != SDL_PIXELFORMAT_RGBA32) {
        rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
        need_free_rgba = true;
        if (!rgba) return;
    }

    // 2) 缩放
    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, dw, dh, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        if (need_free_rgba) SDL_FreeSurface(rgba);
        return;
    }
    SDL_BlitScaled(rgba, nullptr, scaled, nullptr);

    // 3) alpha blend 到内部缓冲区
    const uint8_t* spix   = (const uint8_t*)scaled->pixels;
    int            spitch = scaled->pitch;

    for (int y = 0; y < dh; y++) {
        int py = dy + y;
        if (py < 0 || py >= y_size_) continue;
        const uint8_t* srow = spix + y * spitch;
        uint8_t*       drow = &pixels_[(size_t)py * x_size_ * 4];

        for (int x = 0; x < dw; x++) {
            int px = dx + x;
            if (px < 0 || px >= x_size_) continue;

            const uint8_t* sp = srow + x * 4;
            uint8_t sa = sp[3];
            if (sa == 0) continue;

            uint8_t* dp = drow + px * 4;
            if (sa == 255) {
                dp[0] = sp[0]; dp[1] = sp[1]; dp[2] = sp[2]; dp[3] = 255;
            } else {
                uint8_t dr = dp[0], dg = dp[1], db = dp[2];
                dp[0] = (sp[0] * sa + dr * (255 - sa)) / 255;
                dp[1] = (sp[1] * sa + dg * (255 - sa)) / 255;
                dp[2] = (sp[2] * sa + db * (255 - sa)) / 255;
                dp[3] = 255;
            }
        }
    }

    SDL_FreeSurface(scaled);
    if (need_free_rgba) SDL_FreeSurface(rgba);
}

void UIRenderer::draw_image_fit(SDL_Surface* surf,
                                int cx, int cy, int max_w, int max_h) {
    if (!surf) return;
    int sw = surf->w, sh = surf->h;
    if (sw <= 0 || sh <= 0) return;

    float scale = std::min((float)max_w / sw, (float)max_h / sh);
    int tw = std::max(1, (int)(sw * scale));
    int th = std::max(1, (int)(sh * scale));
    int dx = cx - tw / 2;
    int dy = cy - th / 2;

    blit_surface_scaled(surf, dx, dy, tw, th);
}

void UIRenderer::reset_renderer() {
    if (!window_) return;

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer_) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        std::fprintf(stderr, "[UI] renderer re-created\n");
    }
}

void UIRenderer::recreate_all() {
    std::fprintf(stderr, "[UI] recreating window...\n");

    // 先销毁旧 renderer + window
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    // 重建 window（参数与构造函数里一致）
    window_ = SDL_CreateWindow(
        "TVLive",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        0, 0,
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::fprintf(stderr, "[UI] recreate window failed: %s\n", SDL_GetError());
        return;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        std::fprintf(stderr, "[UI] recreate accel renderer failed, fallback SW\n");
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::fprintf(stderr, "[UI] recreate renderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    std::fprintf(stderr, "[UI] window + renderer recreated\n");
}

void UIRenderer::paint() {
    if (!renderer_) return;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels_.data(),
        x_size_, y_size_,
        32, x_size_ * 4,
        SDL_PIXELFORMAT_RGBA32
    );
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;

    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, tex, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
    SDL_DestroyTexture(tex);
}

} // namespace tv