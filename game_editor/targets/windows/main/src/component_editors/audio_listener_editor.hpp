#ifndef NODEC_GAME_EDITOR__COMPONENT_EDITORS__AUDIO_LISTENER_EDITOR_HPP_
#define NODEC_GAME_EDITOR__COMPONENT_EDITORS__AUDIO_LISTENER_EDITOR_HPP_

#include <nodec_scene_editor/component_editor.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class AudioListenerEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_scene_audio::components::AudioSource> {
public:
    AudioListenerEditor(EditorGui &gui)
        : gui_(gui) {}

    void on_inspector_gui(nodec_scene_audio::components::AudioSource &source, const nodec_scene_editor::InspectorGuiContext &context) override {
    }

private:
    EditorGui &gui_;
};
} // namespace component_editors

#endif