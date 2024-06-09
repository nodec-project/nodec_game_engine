#ifndef NODEC_GAME_EDITOR__EDITOR_WINDOWS__GEOMETRY_BUFFER_INSPECTOR_WINDOW_HPP_
#define NODEC_GAME_EDITOR__EDITOR_WINDOWS__GEOMETRY_BUFFER_INSPECTOR_WINDOW_HPP_

#include <imgui.h>

#include <unordered_map>

#include <imessentials/window.hpp>

#include <rendering/scene_rendering_context.hpp>

class GeometryBufferInspectorWindow : public imessentials::BaseWindow {
public:
    GeometryBufferInspectorWindow(SceneRenderingContext &rendering_context)
        : BaseWindow("Geometry Buffer Inspector", nodec::Vector2f(200, 500)),
          rendering_context_(rendering_context) {
    }

    void on_gui() override;

private:
    SceneRenderingContext &rendering_context_;
    std::unordered_map<std::string, bool> geometry_buffer_opened_;
};

#endif