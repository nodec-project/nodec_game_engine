#include "InspectorGUI.hpp"

// #include <ImGuizmo.h>
#include <nodec/logging.hpp>

using namespace nodec_scene::components;
using namespace nodec_rendering::components;
using namespace nodec;

void InspectorGUI::on_gui_name(nodec_scene::components::Name &name) {
    auto &buffer = imessentials::get_text_buffer(1024, name.name);

    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##name", buffer.data(), buffer.size(), input_text_flags)) {
        // on type enter
    }
    name.name = buffer.data();
}

void InspectorGUI::on_gui_transform(Transform &trfm) {
    // static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    // static ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;

    {
        ImGui::Text("Position");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat3("##Position", trfm.local_position.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f")) {
            trfm.dirty = true;
        }
    }

    {
        static Vector3f eulerAngles;
        static bool isActive = false;

        if (!isActive) {
            eulerAngles = math::gfx::euler_angles_xyz(trfm.local_rotation);
        }

        ImGui::Text("Rotation (XYZ Euler)");

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        if (ImGui::DragFloat3("##Rotation", eulerAngles.v, 0.1f, -FLT_MAX, +FLT_MAX, "%.3f")) {
            trfm.local_rotation = math::gfx::euler_angles_xyz(eulerAngles);
            trfm.dirty = true;
        }

        isActive = ImGui::IsItemActive();
    }

    {
        ImGui::Text("Scale");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat3("##Scale", trfm.local_scale.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f")) {
            trfm.dirty = true;
        }
    }

    // if (currentGizmoOperation != ImGuizmo::SCALE) {
    //     if (ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL)) {
    //         currentGizmoMode = ImGuizmo::LOCAL;
    //     }
    //     ImGui::SameLine();
    //     if (ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD)) {
    //         currentGizmoMode = ImGuizmo::WORLD;
    //     }
    // }

    // ImGuiIO &io = ImGui::GetIO();
    // ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ////ImGuizmo::Manipulate()
}

void InspectorGUI::on_gui_mesh_renderer(nodec_rendering::components::MeshRenderer &renderer) {
    using namespace nodec;
    using namespace nodec_rendering::resources;
    using namespace nodec_rendering;
    {
        imessentials::list_edit(
            "Meshes", renderer.meshes,
            [&](int index, auto &mesh) {
                mesh = resource_name_edit("", mesh);
            },
            [&]() {
                auto empty = resource_registry_->get_resource_direct<Mesh>("");
                renderer.meshes.emplace_back(empty);
            },
            [&](int index, auto &mesh) {});
    }
    {
        imessentials::list_edit(
            "Materials", renderer.materials,
            [&](int index, auto &material) {
                material = resource_name_edit("", material);
            },
            [&]() {
                auto empty = resource_registry_->get_resource_direct<Material>("");
                renderer.materials.emplace_back(empty);
            },
            [&](int index, auto &material) {});
    }
}

void InspectorGUI::on_gui_camera(Camera &camera) {
    ImGui::DragFloat("Near Clip Plane", &camera.near_clip_plane);
    ImGui::DragFloat("Far Clip Plane", &camera.far_clip_plane);

    {
        int current = static_cast<int>(camera.projection);
        ImGui::Combo("Projection", &current, "Perspective\0Orthographic");
        camera.projection = static_cast<Camera::Projection>(current);
    }

    switch (camera.projection) {
    case Camera::Projection::Perspective:
        ImGui::DragFloat("Fov Angle", &camera.fov_angle);
        break;
    case Camera::Projection::Orthographic:
        ImGui::DragFloat("Ortho Width", &camera.ortho_width);
    default: break;
    }
}

void InspectorGUI::on_gui_physics_shape(nodec_physics::components::PhysicsShape &shape) {
    using namespace nodec_physics::components;

    {
        int current = static_cast<int>(shape.shape_type);
        ImGui::Combo("Shape Type", &current, "Box\0Sphere");
        shape.shape_type = static_cast<PhysicsShape::ShapeType>(current);
    }

    switch (shape.shape_type) {
    case PhysicsShape::ShapeType::Box:
        ImGui::DragFloat3("Size", shape.size.v);

        break;
    case PhysicsShape::ShapeType::Sphere:
        ImGui::DragFloat("Radius", &shape.radius);

        break;
    default:
        ImGui::Text("Unsupported shape.");
        break;
    }
}

void InspectorGUI::on_gui_scene_lighting(nodec_rendering::components::SceneLighting &lighting) {
    lighting.skybox = resource_name_edit("Skybox", lighting.skybox);
    ImGui::ColorEdit4("Ambient Color", lighting.ambient_color.v, ImGuiColorEditFlags_Float);
}

void InspectorGUI::on_gui_prefab(nodec_scene_serialization::components::Prefab &prefab) {
    using namespace nodec;
    
    if (ImGui::Button("Save")) {
        logging::InfoStream(__FILE__, __LINE__) << "save";
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        logging::InfoStream(__FILE__, __LINE__) << "load";
    }


    auto &buffer = imessentials::get_text_buffer(1024, prefab.source);
    if (ImGui::InputText("Source", buffer.data(), buffer.size())) {}
    prefab.source = buffer.data();
    ImGui::Checkbox("Active", &prefab.active);
}