#ifndef MATERIAL_EDITOR_WINDOW_HPP_
#define MATERIAL_EDITOR_WINDOW_HPP_

#include <imgui.h>

#include <imessentials/window.hpp>
#include <nodec_rendering/resources/material.hpp>
#include <nodec_resources/resources.hpp>

class MaterialEditorWindow : public imessentials::BaseWindow {
    using Material = nodec_rendering::resources::Material;

    enum class ControlType {
        DragFloat,
        ColorEdit
    };

public:
    MaterialEditorWindow(nodec_resources::Resources &resources)
        : BaseWindow{"Material Editor", nodec::Vector2f{200, 500}},
          resources_(resources) {
    }

    void on_gui() override;

private:
    nodec_resources::Resources &resources_;
    std::shared_ptr<Material> target_material_;
    ControlType control_type_{ControlType::DragFloat};
};
#endif