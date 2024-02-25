#ifndef COMPONENT_EDITORS__AUDIO_SOURCE_EDITOR_HPP_
#define COMPONENT_EDITORS__AUDIO_SOURCE_EDITOR_HPP_

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_scene_audio/components/audio_source.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class AudioSourceEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_scene_audio::components::AudioSource> {
public:
    AudioSourceEditor(EditorGui &gui)
        : gui_(gui) {}

    void on_inspector_gui(nodec_scene_audio::components::AudioSource &source, const nodec_scene_editor::InspectorGuiContext &context) override {
        source.clip = gui_.resource_field("Clip", source.clip);

        {
            int position = source.position.count();
            ImGui::DragInt("Position", &position);
            source.position = std::chrono::milliseconds(position);
        }

        ImGui::Checkbox("Loop", &source.loop);

        ImGui::DragFloat("Volume", &source.volume, 0.01f);

        ImGui::Checkbox("Is Spatial", &source.is_spatial);
    }

private:
    EditorGui &gui_;
};
} // namespace component_editors

#endif