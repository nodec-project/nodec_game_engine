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
#include <nodec/serialization/vector3.hpp>
#include <nodec/serialization/quaternion.hpp>

#include <graphics/graphics.hpp>
#include <rendering/scene_renderer.hpp>

#include "../scene_gizmo_impl.hpp"
#include "../scene_gizmo_renderer.hpp"
#include "../editor_config_archive.hpp"
#include "picking_renderer.hpp"

struct SceneViewSettings : BaseEditorConfigBlock {
    int width{640};
    int height{480};
    
    // カメラの姿勢情報を追加
    nodec::Vector3f camera_position{0.0f, 0.0f, -5.0f};
    nodec::Quaternionf camera_rotation{0.0f, 0.0f, 0.0f, 1.0f}; // w, x, y, z形式

    template<class Archive>
    void serialize(Archive &archive) {
        archive(cereal::make_nvp("width", width),
                cereal::make_nvp("height", height),
                cereal::make_nvp("camera_position", camera_position),
                cereal::make_nvp("camera_rotation", camera_rotation));
    }
};

NODEC_GAME_EDITOR_REGISTER_EDITOR_CONFIG_BLOCK(SceneViewSettings)



class SceneViewWindow final : public imessentials::BaseWindow {
    // 固定の定数を削除して変数に変更
private:
    UINT view_width_ = 640;
    UINT view_height_ = 480;
    UINT input_width_ = 640;  // 入力用の変数
    UINT input_height_ = 480;  // 入力用の変数
    bool size_changed_ = false;  // サイズ変更フラグ
    
    // カメラ姿勢保存のクールダウン関連
    float camera_pose_save_cooldown_ = 0.0f;  // 秒
    const float CAMERA_POSE_SAVE_COOLDOWN_TIME = 1.0f;  // クールダウン時間（秒）
    bool camera_pose_needs_save_ = false;  // 保存が必要かどうかのフラグ

public:
    SceneViewWindow(Graphics &gfx, nodec_scene::Scene &scene, SceneRenderer &renderer,
                    nodec_resources::Resources &,
                    SceneGizmoImpl &scene_gizmo, nodec_scene_editor::ComponentRegistry &component_registry,
                    EditorConfigArchive &editor_config_archive
                );

    void on_gui() override;
    
    // ビューサイズを変更するメソッド
    void resize_view(Graphics &gfx, UINT width, UINT height);
    
    // サイズ変更フラグをチェックして必要に応じてリサイズを実行
    void resize_if_needed();
    
    // カメラの姿勢情報を保存するメソッド
    void save_camera_pose();

private:
    nodec_scene::Scene &scene_;
    SceneRenderer &renderer_;
    SceneGizmoImpl &scene_gizmo_;
    nodec_scene_editor::ComponentRegistry &component_registry_;
    nodec_resources::Resources &resources_;
    Graphics &graphics_;
    EditorConfigArchive &editor_config_archive_;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_;

    std::unique_ptr<SceneGizmoRenderer> scene_gizmo_renderer_;
    std::unique_ptr<PickingRenderer> picking_renderer_;

    nodec::Matrix4x4f projection_;
    nodec::Matrix4x4f view_;
    CameraState camera_state_;
    std::unique_ptr<SceneRenderingContext> rendering_context_;

    ImGuizmo::OPERATION gizmo_operation_{ImGuizmo::TRANSLATE};
    ImGuizmo::MODE gizmo_mode_{ImGuizmo::LOCAL};

    bool scene_view_dragging_{false};

    // Camera follow mode
    nodec::entities::Entity follow_camera_entity_{nodec::entities::null_entity};
    bool follow_camera_enabled_{false};
};

#endif