#ifndef COMPONENT_EDITORS__PREFAB_EDITOR_HPP_
#define COMPONENT_EDITORS__PREFAB_EDITOR_HPP_

#include <nodec_resources/resources.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene_editor/component_editor.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>

#include <nodec_scene_serialization/components/prefab.hpp>

namespace component_editors {

class PrefabEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_scene_serialization::components::Prefab> {
public:
    PrefabEditor(nodec_resources::Resources &resources,
                 nodec_scene::Scene &scene,
                 nodec_scene_serialization::SceneSerialization &serialization)
        : resources_(resources), scene_(scene), serialization_(serialization) {}

    void on_inspector_gui(nodec_scene_serialization::components::Prefab &,
                          const nodec_scene_editor::InspectorGuiContext &) override;

private:
    nodec_resources::Resources &resources_;
    nodec_scene::Scene &scene_;
    nodec_scene_serialization::SceneSerialization &serialization_;
};
} // namespace component_editors

#endif