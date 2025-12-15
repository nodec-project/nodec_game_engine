#ifndef NODEC_GAME_ENGINE__APPLICATION_HPP_
#define NODEC_GAME_ENGINE__APPLICATION_HPP_

#include <memory>

#include <win_desktop_application.hpp>
#include <engine.hpp>
#include <graphics/graphics_device.hpp>

class Application : public WinDesktopApplication {
public:
    Application() {}
    ~Application() {
        release_all_services();
    }

    void quit() noexcept override {
    }

protected:
    void setup() {
        graphics_device_ = std::make_unique<GraphicsDevice>();
        engine_.reset(new Engine(*this));
        configure();
        engine_->screen().set_size(engine_->screen().resolution());
        engine_->setup(*graphics_device_);

        engine_->world_module().reset();
    }

    void loop() {
        engine_->frame_begin();
        engine_->world_module().step();
        engine_->frame_end();
    }

private:
    std::unique_ptr<GraphicsDevice> graphics_device_;
    std::unique_ptr<Engine> engine_;
};

#endif