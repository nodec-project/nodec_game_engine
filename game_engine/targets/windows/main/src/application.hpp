#ifndef NODEC_GAME_ENGINE__APPLICATION_HPP_
#define NODEC_GAME_ENGINE__APPLICATION_HPP_

#include <memory>

#include <WinDesktopApplication.hpp>
#include <engine.hpp>

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
        engine_.reset(new Engine(*this));
        configure();
        engine_->screen().set_size(engine_->screen().resolution());
        engine_->setup();

        engine_->world_module().reset();
    }

    void loop() {
        engine_->frame_begin();
        engine_->world_module().step();
        engine_->frame_end();
    }

private:
    std::unique_ptr<Engine> engine_;
};

#endif