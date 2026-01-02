#ifndef NODEC_GAME_EDITOR__APPLICATION_HPP_
#define NODEC_GAME_EDITOR__APPLICATION_HPP_

#include "editor.hpp"
#include "editor_window.hpp"

#include <engine.hpp>
#include <graphics/graphics_device.hpp>
#include <win_desktop_application.hpp>
#include <window.hpp>

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

        // Create shared graphics device first
        graphics_device_.reset(new GraphicsDevice());

        engine.reset(new Engine(*this));

        editor.reset(new Editor(engine.get()));

        add_service<SceneEditor>(editor);

        configure();

        // ImGui does not work if the resolution and window size are not the same.
        engine->screen().set_size(engine->screen().resolution());

        // Setup engine with shared device (Engine's window won't manage ImGui)
        engine->setup(*graphics_device_);

        // Create EditorWindow for ImGui (uses shared device)
        editor_window_.reset(new EditorWindow(
            *graphics_device_,
            1280, 720,
            L"nodec Game Editor"
        ));

        editor->setup();

        // Do first step (initialize).
        engine->world_module().reset();

        event_loop().schedule([&]() {
            run_main_loop();
        });
    }

    void run_main_loop() {
        int exit_code = 0;
        MSG msg;

        // while queue has message, remove and dispatch them (but do not block on empty queue)
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                exit_code = (int)msg.wParam;
                // signals quit
                return;
            }

            // TranslateMessage will post auxiliary WM_CHAR messages from key msgs
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // EditorWindow handles ImGui frame
        editor_window_->begin_frame();

        // Editor update (DockSpace, editor windows)
        editor->update();

        // Engine frame processing (transform updates, scene rendering, etc.)
        // Note: Engine's Graphics doesn't manage ImGui, so frame_begin/end just handle rendering
        engine->frame_begin();
        engine->frame_end();

        // EditorWindow presents ImGui
        editor_window_->end_frame();

        event_loop().schedule([&]() {
            run_main_loop();
        });
    }

private:
    std::unique_ptr<GraphicsDevice> graphics_device_;
    std::unique_ptr<Engine> engine;
    std::shared_ptr<Editor> editor;
    std::unique_ptr<EditorWindow> editor_window_;
};

#endif