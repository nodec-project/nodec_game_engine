#include "InspectorGUI.hpp"

#include <nodec/formatter.hpp>
#include <nodec/logging.hpp>
#include <nodec_scene_serialization/archive_context.hpp>
#include <nodec_scene_serialization/components/entity_built.hpp>
#include <nodec_scene_serialization/entity_loader.hpp>
#include <nodec_scene_serialization/entity_serializer.hpp>
#include <nodec_scene_serialization/systems/prefab_load_system.hpp>

#include <cereal/archives/json.hpp>

#include <fstream>

using namespace nodec_scene::components;
using namespace nodec_rendering::components;
using namespace nodec;

void InspectorGUI::on_gui_name(nodec_scene::components::Name &name) {
    auto &buffer = imessentials::get_text_buffer(1024, name.value);

    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##name", buffer.data(), buffer.size(), input_text_flags)) {
        // on type enter
    }
    name.value = buffer.data();
}

void InspectorGUI::on_gui_transform(LocalTransform &trfm) {
    // static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    // static ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;

    {
        ImGui::Text("Position");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat3("##Position", trfm.position.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f")) {
            trfm.dirty = true;
        }
    }

    {
        static Vector3f eulerAngles;
        static bool isActive = false;

        if (!isActive) {
            eulerAngles = math::gfx::euler_angles_xyz(trfm.rotation);
        }

        ImGui::Text("Rotation (XYZ Euler)");

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        if (ImGui::DragFloat3("##Rotation", eulerAngles.v, 0.1f, -FLT_MAX, +FLT_MAX, "%.3f")) {
            trfm.rotation = math::gfx::euler_angles_xyz(eulerAngles);
            trfm.dirty = true;
        }

        isActive = ImGui::IsItemActive();
    }

    {
        ImGui::Text("Scale");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat3("##Scale", trfm.scale.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f")) {
            trfm.dirty = true;
        }
    }
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
                auto empty = resources_.registry().get_resource_direct<Mesh>("");
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
                auto empty = resources_.registry().get_resource_direct<Material>("");
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

void InspectorGUI::on_gui_rigid_body(nodec_physics::components::RigidBody &rigid_body) {
    using namespace nodec_physics::components;

    ImGui::DragFloat("Mass", &rigid_body.mass);

    if (ImGui::TreeNode("Constrains")) {

        ImGui::PushID("Freeze Position");
        {
            ImGui::Text("Freeze Position");

            bool flag;
            flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezePositionX);
            ImGui::Checkbox("X", &flag);
            flag ? rigid_body.constraints |= RigidBodyConstraints::FreezePositionX
                 : rigid_body.constraints &= ~RigidBodyConstraints::FreezePositionX;

            ImGui::SameLine();

            flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezePositionY);
            ImGui::Checkbox("Y", &flag);
            flag ? rigid_body.constraints |= RigidBodyConstraints::FreezePositionY
                 : rigid_body.constraints &= ~RigidBodyConstraints::FreezePositionY;

            ImGui::SameLine();

            flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezePositionZ);
            ImGui::Checkbox("Z", &flag);
            flag ? rigid_body.constraints |= RigidBodyConstraints::FreezePositionZ
                 : rigid_body.constraints &= ~RigidBodyConstraints::FreezePositionZ;

            ImGui::PopID();
        }

        ImGui::PushID("Freeze Rotation");
        {
            ImGui::Text("Freeze Rotation");
            bool flag;
            flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezeRotationX);
            ImGui::Checkbox("X", &flag);
            flag ? rigid_body.constraints |= RigidBodyConstraints::FreezeRotationX
                 : rigid_body.constraints &= ~RigidBodyConstraints::FreezeRotationX;

            ImGui::SameLine();

            flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezeRotationY);
            ImGui::Checkbox("Y", &flag);
            flag ? rigid_body.constraints |= RigidBodyConstraints::FreezeRotationY
                 : rigid_body.constraints &= ~RigidBodyConstraints::FreezeRotationY;

            ImGui::SameLine();

            flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezeRotationZ);
            ImGui::Checkbox("Z", &flag);
            flag ? rigid_body.constraints |= RigidBodyConstraints::FreezeRotationZ
                 : rigid_body.constraints &= ~RigidBodyConstraints::FreezeRotationZ;

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
    
    
    if (ImGui::TreeNode("Info")) {
        ImGui::DragFloat3("Linear Velocity", rigid_body.linear_velocity.v);
        ImGui::DragFloat3("Angular Velocity", rigid_body.angular_velocity.v);

        ImGui::TreePop();
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

void InspectorGUI::on_gui_prefab(nodec_scene_serialization::components::Prefab &prefab, const nodec_scene::SceneEntity &entity, nodec_scene::SceneRegistry &registry) {
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene_serialization;
    using namespace nodec_scene_serialization::components;
    using namespace nodec_scene_serialization::systems;

    if (ImGui::Button("Save")) {
        [&]() {
            // TODO: add filter.
            auto serializable = EntitySerializer(serialization_).serialize(entity, scene_, [&](const SceneEntity &entt) {
                if (entt == entity) return true;
                auto *prefab = registry.try_get_component<Prefab>(entt);
                if (prefab != nullptr) return false;
                return true;
            });

            if (!serializable) {
                logging::WarnStream(__FILE__, __LINE__)
                    << "Failed to save.\n"
                       "details:\n"
                       "Target entity is invalid.";
                return;
            }
            std::ofstream out(Formatter() << resources_.resource_path() << "/" << prefab.source, std::ios::binary);
            if (!out) {
                logging::WarnStream(__FILE__, __LINE__)
                    << "Failed to save.\n"
                       "details:\n"
                       "Cannot open the file path. Make sure the directory exists.";
                return;
            }

            ArchiveContext context(resources_.registry());
            using Options = cereal::JSONOutputArchive::Options;
            cereal::UserDataAdapter<ArchiveContext, cereal::JSONOutputArchive> archive(context, out, Options(324, Options::IndentChar::space, 2u));

            try {
                archive(cereal::make_nvp("entity", *serializable));
            } catch (std::exception &e) {
                logging::WarnStream(__FILE__, __LINE__)
                    << "Failed to save.\n"
                       "details:/n"
                    << e.what();
                return;
            }

            logging::InfoStream(__FILE__, __LINE__) << "Saved!";
        }();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        // Be careful. The following codes will change component structure while iterating.
        scene_.hierarchy_system().remove_all_children(entity);
        registry.remove_component<EntityBuilt>(entity);
        registry.emplace_component<PrefabLoadSystem::PrefabLoadActivity>(entity);
        // logging::InfoStream(__FILE__, __LINE__) << "Loaded!";
    }

    auto &buffer = imessentials::get_text_buffer(1024, prefab.source);
    if (ImGui::InputText("Source", buffer.data(), buffer.size())) {}
    prefab.source = buffer.data();
    ImGui::Checkbox("Active", &prefab.active);
}

void InspectorGUI::on_gui_post_processing(nodec_rendering::components::PostProcessing &processing) {
    using namespace nodec_rendering::components;
    using namespace nodec_rendering::resources;

    {
        ImGui::PushID("effects");
        ImGui::Text("Effects");
        for (auto iter = processing.effects.begin(); iter != processing.effects.end();) {
            ImGui::PushID(&*iter);

            ImGui::Checkbox("##enabled", &(iter->enabled));
            ImGui::SameLine();

            auto &material = iter->material;
            material = resource_name_edit("##material", material);

            ImGui::SameLine();
            if (ImGui::Button("-")) {
                iter = processing.effects.erase(iter);
                ImGui::PopID();
                continue;
            }
            ++iter;
            ImGui::PopID();
        }

        if (ImGui::Button("+")) {
            auto material = resources_.registry().get_resource<Material>("").get();
            processing.effects.push_back({false, material});
        }

        ImGui::PopID();
    }
}