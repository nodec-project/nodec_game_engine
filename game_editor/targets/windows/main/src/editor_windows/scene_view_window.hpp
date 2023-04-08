#ifndef SCENE_VIEW_WINDOW_HPP_
#define SCENE_VIEW_WINDOW_HPP_

#include <Graphics/Graphics.hpp>
#include <Rendering/SceneRenderer.hpp>

#include <imessentials/window.hpp>
#include <nodec_rendering/components/camera.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene/scene_entity.hpp>

#include <nodec/math/gfx.hpp>
#include <nodec/matrix4x4.hpp>

#include <ImGuizmo.h>
#include <imgui.h>

class SceneViewWindow final : public imessentials::BaseWindow {
    // TODO: Support resizing window.
    const UINT VIEW_WIDTH = 640;
    const UINT VIEW_HEIGHT = 480;

public:
    SceneViewWindow(Graphics &gfx, nodec_scene::Scene &scene, SceneRenderer &renderer,
                    nodec_scene::SceneEntity init_selected_entity,
                    nodec::signals::SignalInterface<void(nodec_scene::SceneEntity)> selected_entity_changed_signal);

    void on_gui() override;

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_;

    nodec::Matrix4x4f projection_;
    nodec::Matrix4x4f view_;
    nodec_scene::Scene *scene_{nullptr};
    SceneRenderer *renderer_{nullptr};
    std::unique_ptr<SceneRenderingContext> rendering_context_;

    nodec::signals::Connection selected_entity_changed_conn_;
    nodec_scene::SceneEntity selected_entity_;

    ImGuizmo::OPERATION gizmo_operation_{ImGuizmo::TRANSLATE};
    ImGuizmo::MODE gizmo_mode_{ImGuizmo::WORLD};
};

#endif