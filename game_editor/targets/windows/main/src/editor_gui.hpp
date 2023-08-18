#ifndef EDITOR_GUI_HPP_
#define EDITOR_GUI_HPP_

#include <memory>

#include <imessentials/text_buffer.hpp>
#include <nodec_resources/resources.hpp>

class EditorGui {
public:
    EditorGui(nodec_resources::Resources &resources)
        : resources_(resources) {}

    template<typename T>
    std::shared_ptr<T> resource_field(const char *label, std::shared_ptr<T> resource) {
        std::string orig_name;
        bool found;
        std::tie(orig_name, found) = resources_.registry().lookup_name(resource);

        auto &buffer = imessentials::get_text_buffer(1024, orig_name);

        if (ImGui::InputText(label, buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string new_name = buffer.data();
            if (new_name.empty()) {
                // if empty, set resource to null.
                resource.reset();
            } else {
                auto new_resource = resources_.registry().get_resource<T>(new_name).get();
                resource = new_resource ? new_resource : resource;
            }
        }
        return resource;
    }

private:
    nodec_resources::Resources &resources_;
};

#endif