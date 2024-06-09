#include "geometry_buffer_inspector_window.hpp"

#include <nodec/iterator.hpp>
#include <nodec/logging/logger.hpp>

void GeometryBufferInspectorWindow::on_gui() {
    ImGui::Text("Geometry Buffer Count: %d", rendering_context_.geometry_buffer_count());

    for (auto iter = rendering_context_.geometry_buffer_begin(); iter != rendering_context_.geometry_buffer_end(); ++iter) {
        auto &name = iter->first;
        auto &buffer = iter->second;
        auto &opened = geometry_buffer_opened_[name];
        if (ImGui::Selectable(name.c_str(), &opened)) {
        }
        if (buffer && opened) {
            ImGui::SetNextWindowSize(ImVec2(320, 320), ImGuiCond_FirstUseEver);
            ImGui::Begin(name.c_str(), &opened);
            ImGui::Text("Size: %d x %d", buffer->width(), buffer->height());

            ImGui::Image((void *)&buffer->shader_resource_view(), ImVec2(320, 320));
            ImGui::End();
        }
    }

    // for (auto &pair : rendering_context_.geometry_buffers()) {
    //     auto &name = pair.first;
    //     auto &buffer = pair.second;
    //     ImGui::Text("Buffer: %s", name.c_str());
    //     ImGui::Text("Size: %d x %d", buffer.width(), buffer.height());
    //     ImGui::Separator();
    // }
}