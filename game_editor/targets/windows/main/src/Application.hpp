#ifndef NODEC_GAME_EDITOR__APPLICATION_HPP_
#define NODEC_GAME_EDITOR__APPLICATION_HPP_

#include "editor.hpp"

#include <engine.hpp>
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

        engine.reset(new Engine(*this));

        editor.reset(new Editor(engine.get()));

        add_service<SceneEditor>(editor);

        configure();

        // ImGui does not work if the resolution and window size are not the same.
        engine->screen().set_size(engine->screen().resolution());

        engine->setup();
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
        // while (GetMessage(&msg, nullptr, 0, 0)) {
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

        engine->frame_begin();

        editor->update();

        engine->frame_end();

        event_loop().schedule([&]() {
            run_main_loop();
        });
    }

private:
    std::unique_ptr<Engine> engine;
    std::shared_ptr<Editor> editor;
};

#endif