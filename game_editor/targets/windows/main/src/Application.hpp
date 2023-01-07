#pragma once

#include "Editor.hpp"

#include <Engine.hpp>
#include <WinDesktopApplication.hpp>
#include <Window.hpp>

#include <nodec_scene_editor/impl/scene_editor_module.hpp>

class Application final : public WinDesktopApplication {
public:
    Application() {}
    ~Application() {
        // At first, we release the service instances held by this application.
        // Note that all services is not actually released.
        // Some instances are held by other members (like Engine instance).
        // The services in app layer will be released.
        release_all_services();
    }

public:
    void quit() noexcept override {
    }

protected:
    void setup() {
        using namespace nodec;
        using namespace nodec_scene_editor::impl;
        using namespace nodec_scene_editor;

        engine.reset(new Engine(*this));

        editor.reset(new Editor(engine.get()));

        add_service<SceneEditor>(editor);

        configure();

        // ImGui does not work if the resolution and window size are not the same.
        engine->screen_module().internal_size = engine->screen_module().internal_resolution;

        engine->setup();
        editor->setup();

        // Do first step (initialize).
        engine->world_module().reset();
    }

    void loop() {
        engine->frame_begin();

        editor->update();

        engine->frame_end();
    }

private:
    std::unique_ptr<Engine> engine;
    std::shared_ptr<Editor> editor;
};