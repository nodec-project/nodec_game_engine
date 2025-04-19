#ifndef NODEC_GAME_EDITOR__EDITOR_WINDOWS__SCENE_VIEW_WINDOW_HPP_
#define NODEC_GAME_EDITOR__EDITOR_WINDOWS__SCENE_VIEW_WINDOW_HPP_

#include <imgui.h>

#include <ImGuizmo.h>
#include <imessentials/window.hpp>
#include <nodec/gfx/gfx.hpp>
#include <nodec/matrix4x4.hpp>
#include <nodec_rendering/components/camera.hpp>
#include <nodec_resources/resources.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene/scene_entity.hpp>
#include <nodec_scene_editor/component_registry.hpp>

#include <graphics/graphics.hpp>
#include <rendering/scene_renderer.hpp>

#include "../scene_gizmo_impl.hpp"
#include "../scene_gizmo_renderer.hpp"

class SceneViewWindow final : public imessentials::BaseWindow {
    // 固定の定数を削除して変数に変更
private:
    UINT view_width_ = 640;
    UINT view_height_ = 480;
    UINT input_width_ = 640;  // 入力用の変数
    UINT input_height_ = 480;  // 入力用の変数
    bool size_changed_ = false;  // サイズ変更フラグ

public:
    SceneViewWindow(Graphics &gfx, nodec_scene::Scene &scene, SceneRenderer &renderer,
                    nodec_resources::Resources &,
                    SceneGizmoImpl &scene_gizmo, nodec_scene_editor::ComponentRegistry &component_registry);

    void on_gui() override;
    
    // ビューサイズを変更するメソッド
    void resize_view(Graphics &gfx, UINT width, UINT height);
    
    // サイズ変更フラグをチェックして必要に応じてリサイズを実行
    void check_and_resize_if_needed();

private:
    nodec_scene::Scene &scene_;
    SceneRenderer &renderer_;
    SceneGizmoImpl &scene_gizmo_;
    nodec_scene_editor::ComponentRegistry &component_registry_;
    nodec_resources::Resources &resources_;
    Graphics &graphics_;  // グラフィックスオブジェクトへの参照を保持

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_;

    std::unique_ptr<SceneGizmoRenderer> scene_gizmo_renderer_;

    nodec::Matrix4x4f projection_;
    nodec::Matrix4x4f view_;
    CameraState camera_state_;
    std::unique_ptr<SceneRenderingContext> rendering_context_;

    ImGuizmo::OPERATION gizmo_operation_{ImGuizmo::TRANSLATE};
    ImGuizmo::MODE gizmo_mode_{ImGuizmo::LOCAL};

    bool scene_view_dragging_{false};
};

#endif