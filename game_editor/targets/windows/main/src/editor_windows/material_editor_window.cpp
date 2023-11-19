#include "material_editor_window.hpp"

#include <cassert>
#include <fstream>

#include <cereal/archives/json.hpp>
#include <imessentials/text_buffer.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec_rendering/serialization/resources/material.hpp>

namespace {

void export_material(std::shared_ptr<nodec_rendering::resources::Material> &material, nodec_resources::Resources &resources) {
    using namespace nodec_rendering::resources;

    assert(material);

    const std::string path = nodec::Formatter() << resources.resource_path() << "/"
                                                << resources.registry().lookup_name<Material>(material).first;
    std::ofstream out(path, std::ios::binary);

    if (!out) {
        nodec::logging::warn(__FILE__, __LINE__)
            << "Failed to open the file.\n"
               "Make sure that the directory exists.\n"
               "  path: "
            << path;
        return;
    }

    using Options = cereal::JSONOutputArchive::Options;
    cereal::JSONOutputArchive archive(out, Options(324, Options::IndentChar::space, 2u));

    try {
        SerializableMaterial ser_material(*material, resources.registry());
        archive(cereal::make_nvp("material", ser_material));
        nodec::logging::info(__FILE__, __LINE__) << "Saved!";
    } catch (std::exception &e) {
        nodec::logging::warn(__FILE__, __LINE__)
            << "Failed to export material.\n"
               "details:\n"
            << e.what();
    }
}

void import_material(std::shared_ptr<nodec_rendering::resources::Material> &material, nodec_resources::Resources &resources) {
    using namespace nodec_rendering::resources;

    assert(material);

    
    const std::string path = nodec::Formatter() << resources.resource_path() << "/"
                                                << resources.registry().lookup_name<Material>(material).first;
    std::ifstream file(path, std::ios::binary);

    
    if (!file) {
        nodec::logging::warn(__FILE__, __LINE__)
            << "Failed to open the file.\n"
               "Make sure that the file exists.\n"
               "  path: "
            << path;
        return;
    }

    cereal::JSONInputArchive archive(file);

    try {
        SerializableMaterial ser_material;
        archive(ser_material);

        ser_material.apply_to(*material, resources.registry());
        
        nodec::logging::info(__FILE__, __LINE__) << "Imported!";
    }
    catch (std::exception& e) {
        nodec::logging::warn(__FILE__, __LINE__)
            << "Failed to import material.\n"
               "details:\n"
            << e.what();
    }
}

} // namespace

void MaterialEditorWindow::on_gui() {
    using namespace nodec_rendering;
    using namespace nodec_rendering::resources;

    const auto material_name = resources_.registry().lookup_name<Material>(target_material_).first;

    auto &buffer = imessentials::get_text_buffer(1024, material_name);
    if (ImGui::InputText("Target", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
        target_material_ = resources_.registry().get_resource<Material>(buffer.data()).get();
    }

    ImGui::Separator();

    if (!target_material_) {
        ImGui::Text("No Target");
        return;
    }

    {
        auto shader = target_material_->shader();
        auto shader_name = resources_.registry().lookup_name<Shader>(shader).first;

        auto &buffer = imessentials::get_text_buffer(1024, shader_name);
        if (ImGui::InputText("Shader", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string new_name = buffer.data();
            if (new_name.empty()) {
                target_material_->set_shader(nullptr);
            } else {
                auto shader_to_set = resources_.registry().get_resource<Shader>(new_name).get();
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
                auto texture_name = resources_.registry().lookup_name<Texture>(current.texture).first;

                auto &buffer = imessentials::get_text_buffer(1024, texture_name);
                if (ImGui::InputText("Texture", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string new_name = buffer.data();
                    if (new_name.empty()) {
                        current.texture.reset();
                    } else {
                        auto texture_to_set = resources_.registry().get_resource<Texture>(new_name).get();

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
        // Export to the file.
        export_material(target_material_, resources_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Revert")) {
        // Import from the file.
        import_material(target_material_, resources_);
    }
}
