#ifndef NODEC_GAME_ENGINE__SCREEN__SCREEN_BACKEND_HPP_
#define NODEC_GAME_ENGINE__SCREEN__SCREEN_BACKEND_HPP_

#include "../window.hpp"

#include <nodec_screen/screen.hpp>

class ScreenBackend : public nodec_screen::Screen {
public:
    ScreenBackend() {}

    void setup(Window *window) {
        window_ = window;
    }

    nodec::Vector2i resolution() const noexcept override {
        return resolution_;
    }

    void set_resolution(const nodec::Vector2i &resolution) override {
        if (window_) return;
        resolution_ = resolution;
    }

    nodec::Vector2i size() const noexcept override {
        return size_;
    }

    void set_size(const nodec::Vector2i &size) override {
        if (window_) return;
        size_ = size;
    }

    std::string title() const noexcept override {
        return title_;
    }

    void set_title(const std::string &title) override;

private:
    nodec::Vector2i resolution_{1280, 720};
    nodec::Vector2i size_{1280, 720};
    std::string title_;

    Window *window_{nullptr};
};

#endif