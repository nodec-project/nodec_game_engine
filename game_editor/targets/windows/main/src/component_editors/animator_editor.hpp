#ifndef NODEC_GAME_EDITOR__COMPONENT_EDITORS__ANIMATOR_EDITOR_HPP_
#define NODEC_GAME_EDITOR__COMPONENT_EDITORS__ANIMATOR_EDITOR_HPP_

#include <imgui.h>
#include <nodec_animation/components/animator.hpp>
#include <nodec_scene_editor/component_editor.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class AnimatorEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_animation::components::Animator> {
public:
    AnimatorEditor(EditorGui &gui)
        : gui_(gui) {}

    void on_inspector_gui(nodec_animation::components::Animator &animator,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        animator.clip = gui_.resource_field("Clip", animator.clip);
    }

private:
    EditorGui &gui_;
};

} // namespace component_editors

#endif