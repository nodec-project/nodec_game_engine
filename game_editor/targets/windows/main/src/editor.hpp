#ifndef NODEC_GAME_EDITOR__EDITOR_HPP_
#define NODEC_GAME_EDITOR__EDITOR_HPP_

#include <Engine.hpp>

#include <imessentials/impl/menu_impl.hpp>
#include <imessentials/impl/window_impl.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec_scene_editor/impl/scene_editor_impl.hpp>

#include "editor_gui.hpp"
#include "scene_gizmo_impl.hpp"

class Editor final : public nodec_scene_editor::impl::SceneEditorImpl {
public:
    enum class Mode {
        Edit,
        Play
    };

    enum class State {
        Playing,
        Paused
    };

public:
    Editor(Engine *engine);

    ~Editor() {
    }

    void setup();

    void step() {
        if (state_ != State::Paused) return;

        do_one_step_ = true;
    }

    void reset() {
        engine_->world_module().reset();
        state_ = State::Paused;
    }

    void play() {
        if (state_ == State::Playing) return;

        state_ = State::Playing;
    }

    void pause() {
        if (state_ == State::Paused) return;

        state_ = State::Paused;
    }

    void update();

    State state() const noexcept {
        return state_;
    }

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    Engine *engine_;
    State state_{State::Paused};
    bool do_one_step_{false};
    std::unique_ptr<EditorGui> editor_gui_;
    std::unique_ptr<SceneGizmoImpl> scene_gizmo_;
};

#endif