#include "material_editor_window.hpp"

#include <imessentials/text_buffer.hpp>

void MaterialEditorWindow::on_gui() {
    using namespace nodec_rendering;
    using namespace nodec_rendering::resources;

    if (!registry_) return;

    auto material_name = registry_->lookup_name<Material>(target_material_).first;

    auto& buffer = imessentials::get_text_buffer(1024, material_name);
    if (ImGui::InputText("Target", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
        target_material_ = registry_->get_resource<Material>(buffer.data()).get();
    }

    ImGui::Separator();

    if (!target_material_) {
        ImGui::Text("No Target");
        return;
    }

    {
        auto shader = target_material_->shader();
        auto shader_name = registry_->lookup_name<Shader>(shader).first;

        auto &buffer = imessentials::get_text_buffer(1024, shader_name);
        if (ImGui::InputText("Shader", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string new_name = buffer.data();
            if (new_name.empty()) {
                target_material_->set_shader(nullptr);
            } else {
                auto shader_to_set = registry_->get_resource<Shader>(new_name).get();
                if (shader_to_set) target_material_->set_shader(shader_to_set);
            }
        }
    }

    auto shader = target_material_->shader();
    if (!shader) {
        ImGui::Text("No Shader");
        return;
    }

    ImGui::Spacing();

    {
        int current = static_cast<int>(target_material_->cull_mode());
        ImGui::Combo("Cull Mode", &current, "Off\0Front\0Back");
        target_material_->set_cull_mode(static_cast<CullMode>(current));
    }

    ImGui::Spacing();

    // Float properties
    {
        ImGui::PushID("float-properties");
        for (int i = 0; i < shader->float_property_count(); ++i) {
            ImGui::PushID(i);
            auto &name = shader->get_float_property_name(i);
            auto current = target_material_->get_float_property(name);
            ImGui::DragFloat(name.c_str(), &current, 0.01f);
            target_material_->set_float_property(name, current);
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    ImGui::Spacing();

    // Vector4 properties
    {
        ImGui::PushID("vector4-properties");
        if (ImGui::RadioButton("Float", control_type_ == ControlType::DragFloat)) control_type_ = ControlType::DragFloat;
        ImGui::SameLine();
        if (ImGui::RadioButton("Color", control_type_ == ControlType::ColorEdit)) control_type_ = ControlType::ColorEdit;

        for (int i = 0; i < shader->vector4_property_count(); ++i) {
            ImGui::PushID(i);
            auto &name = shader->get_vector4_property_name(i);
            auto current = target_material_->get_vector4_property(name).value();
            switch (control_type_) {
            case ControlType::DragFloat:
                ImGui::DragFloat4(name.c_str(), current.v, 0.01f);
                break;
            default:
            case ControlType::ColorEdit:
                ImGui::ColorEdit4(name.c_str(), current.v, ImGuiColorEditFlags_Float);
                break;
            }
            target_material_->set_vector4_property(name, current);
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    {
        ImGui::PushID("texture_entries");

        for (int i = 0; i < shader->texture_entry_count(); ++i) {
            ImGui::Spacing();

            ImGui::PushID(i);

            auto &name = shader->get_texture_entry_name(i);
            auto current = target_material_->get_texture_entry(name).value();

            ImGui::Text(name.c_str());

            {
                // if (current.texture) {
                //     ImVec2 size{static_cast<float>(current.texture->width()), static_cast<float>(current.texture->height())};
                //     ImGui::Image(current.texture.get(), size);
                // }
                auto texture_name = registry_->lookup_name<Texture>(current.texture).first;

                auto &buffer = imessentials::get_text_buffer(1024, texture_name);
                if (ImGui::InputText("Texture", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string new_name = buffer.data();
                    if (new_name.empty()) {
                        current.texture.reset();
                    } else {
                        auto texture_to_set = registry_->get_resource<Texture>(new_name).get();

                        if (texture_to_set) current.texture = texture_to_set;
                    }
                }
            }

            {
                int filter_mode = static_cast<int>(current.sampler.filter_mode);
                ImGui::Combo("Filter Mode", &filter_mode, "Point\0Bilinear\0Anisotropic");
                current.sampler.filter_mode = static_cast<Sampler::FilterMode>(filter_mode);

                int wrap_mode = static_cast<int>(current.sampler.wrap_mode);
                ImGui::Combo("Wrap Mode", &wrap_mode, "Wrap\0Clamp");
                current.sampler.wrap_mode = static_cast<Sampler::WrapMode>(wrap_mode);
            }


            target_material_->set_texture_entry(name, current);
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    ImGui::Separator();

    if (ImGui::Button("Save")) {

    }
    ImGui::SameLine();
    if (ImGui::Button("Revert")) {

    }
}
