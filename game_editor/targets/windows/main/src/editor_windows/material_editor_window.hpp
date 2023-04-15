#ifndef MATERIAL_EDITOR_WINDOW_HPP_
#define MATERIAL_EDITOR_WINDOW_HPP_

#include <imessentials/window.hpp>
#include <nodec_rendering/resources/material.hpp>

#include <nodec/resource_management/resource_registry.hpp>

#include <imgui.h>

class MaterialEditorWindow : public imessentials::BaseWindow {
    using Material = nodec_rendering::resources::Material;

    enum class ControlType {
        DragFloat,
        ColorEdit
    };

public:
    MaterialEditorWindow(nodec::resource_management::ResourceRegistry *registry)
        : BaseWindow{"Material Editor", nodec::Vector2f{200, 500}},
          registry_(registry) {
    }

    void on_gui() override;

private:
    nodec::resource_management::ResourceRegistry *registry_{nullptr};
    std::shared_ptr<Material> target_material_;
    ControlType control_type_{ControlType::DragFloat};
};
#endif