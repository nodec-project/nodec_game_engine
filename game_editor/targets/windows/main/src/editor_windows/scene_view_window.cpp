#include "scene_view_window.hpp"

#include <imgui_internal.h>

#include <nodec/gfx/gfx.hpp>
#include <nodec_scene_editor/components/selected.hpp>

#include <DirectXMath.h>

SceneViewWindow::SceneViewWindow(
    Graphics &gfx, nodec_scene::Scene &scene, SceneRenderer &renderer, nodec_resources::Resources &resources,
    SceneGizmoImpl &scene_gizmo, nodec_scene_editor::ComponentRegistry &component_registry,
    EditorConfigArchive &editor_config_archive)
    : BaseWindow("Scene View##EditorWindows", nodec::Vector2f(view_width_, view_height_)),
      scene_gizmo_(scene_gizmo), component_registry_(component_registry), resources_(resources),
      scene_(scene), renderer_(renderer), graphics_(gfx), editor_config_archive_(editor_config_archive) {
    auto& base_config_block = editor_config_archive_.config().blocks["editor-backends.SceneView"];
    if (base_config_block) {
        auto config_block = static_cast<SceneViewSettings *>(base_config_block.get());
        view_width_ = config_block->width;
        view_height_ = config_block->height;
    } else {
        auto config_block = std::make_unique<SceneViewSettings>();
        config_block->width = view_width_;
        config_block->height = view_height_;
        base_config_block = std::move(config_block);
    }

    // Generate the render target textures.
    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = view_width_;
    texture_desc.Height = view_height_;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    
    // 初期入力値を現在のサイズに設定
    input_width_ = view_width_;
    input_height_ = view_height_;

    ThrowIfFailedGfx(
        gfx.device().CreateTexture2D(&texture_desc, nullptr, &texture_),
        &gfx, __FILE__, __LINE__);

    // Generate the render target views.
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc{};
        desc.Format = texture_desc.Format;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        ThrowIfFailedGfx(
            gfx.device().CreateRenderTargetView(texture_.Get(), &desc, &render_target_view_),
            &gfx, __FILE__, __LINE__);
    }

    // Generate the shader resource views.
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Format = texture_desc.Format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;

        ThrowIfFailedGfx(
            gfx.device().CreateShaderResourceView(texture_.Get(), &desc, &shader_resource_view_),
            &gfx, __FILE__, __LINE__);
    }

    rendering_context_.reset(new SceneRenderingContext(view_width_, view_height_, gfx));
    scene_gizmo_renderer_.reset(new SceneGizmoRenderer(gfx, resources));

    {
        using namespace nodec_rendering::components;
        using namespace DirectX;

        const auto view_aspect = static_cast<float>(view_width_) / view_height_;
        Camera camera;
        camera.projection = Camera::Projection::Perspective;
        camera.far_clip_plane = 10000.0f;
        camera.near_clip_plane = 0.01f;
        camera.fov_angle = 45.0f;

        camera_state_.update_projection(camera, view_aspect);

        XMFLOAT4X4 matrix;
        XMStoreFloat4x4(&matrix, camera_state_.matrix_p());

        projection_.set(matrix.m[0], matrix.m[1], matrix.m[2], matrix.m[3]);
    }
}

void SceneViewWindow::on_gui() {
    using namespace nodec;
    using namespace DirectX;
    using namespace nodec_scene::components;
    using namespace nodec_scene;

    // サイズ変更をチェック
    resize_if_needed();

    // サイズ変更UIを追加
    {
        ImGui::Text("View Size Settings:");
        ImGui::PushItemWidth(70);
        ImGui::InputInt("Width##ViewSizeW", reinterpret_cast<int*>(&input_width_), 0, 0);
        ImGui::SameLine();
        ImGui::InputInt("Height##ViewSizeH", reinterpret_cast<int*>(&input_height_), 0, 0);
        ImGui::PopItemWidth();
        
        // 入力値の範囲制限
        if (input_width_ < 1) input_width_ = 1;
        if (input_width_ > 4096) input_width_ = 4096;
        if (input_height_ < 1) input_height_ = 1;
        if (input_height_ > 4096) input_height_ = 4096;
        
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            // サイズが変更された場合だけ更新
            if (input_width_ != view_width_ || input_height_ != view_height_) {
                // ウィンドウ外部からgfxを取得する必要があるため、size_changed_フラグを設定
                size_changed_ = true;
            }
        }
        
        ImGui::SameLine();
        ImGui::Text("Current: %dx%d", view_width_, view_height_);
    }

    if (ImGui::RadioButton("Translate", gizmo_operation_ == ImGuizmo::TRANSLATE)) {
        gizmo_operation_ = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", gizmo_operation_ == ImGuizmo::ROTATE)) {
        gizmo_operation_ = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", gizmo_operation_ == ImGuizmo::SCALE)) {
        gizmo_operation_ = ImGuizmo::SCALE;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", gizmo_mode_ == ImGuizmo::LOCAL)) {
        gizmo_mode_ = ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("World", gizmo_mode_ == ImGuizmo::WORLD)) {
        gizmo_mode_ = ImGuizmo::WORLD;
    }
    
    ImGui::BeginChild("SceneRender", ImVec2(view_width_, view_height_), false, ImGuiWindowFlags_NoMove);
    {
        const auto view_aspect = static_cast<float>(view_width_) / view_height_;

        {
            using namespace nodec_rendering::components;
            Camera camera;
            camera.projection = Camera::Projection::Perspective;
            camera.far_clip_plane = 10000.0f;
            camera.near_clip_plane = 0.01f;
            camera.fov_angle = 45.0f;

            XMFLOAT4X4 matrix;
            XMStoreFloat4x4(&matrix, XMMatrixPerspectiveFovLH(
                                         XMConvertToRadians(45),
                                         view_aspect,
                                         0.01f, 10000.0f));

            projection_.set(matrix.m[0], matrix.m[1], matrix.m[2], matrix.m[3]);
        }

        auto &io = ImGui::GetIO();
        auto view_inverse_ = math::inv(view_);
        gfx::TRSComponents camera_trs;
        gfx::decompose_trs(view_inverse_, camera_trs);

        const auto forward = gfx::rotate(Vector3f(0.f, 0.f, 1.f), camera_trs.rotation);
        const auto right = gfx::rotate(Vector3f(1.f, 0.f, 0.f), camera_trs.rotation);
        const auto up = gfx::rotate(Vector3f(0.f, 1.f, 0.f), camera_trs.rotation);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
            scene_view_dragging_ = true;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && ImGui::IsWindowHovered()) {
            scene_view_dragging_ = true;
        }

        if (scene_view_dragging_) {
            {
                // XXX: This code prevent other items captures the mouse event while dragging.
                // But, I dont understand why this code works:)
                auto *window = ImGui::GetCurrentWindow();
                ImGui::SetActiveID(ImGui::GetID("SceneViewDragging"), window);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                constexpr float SCALE_FACTOR = 0.2f;
                const auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

                // Apply rotation around the local right vector after current rotation.
                camera_trs.rotation = gfx::quaternion_from_angle_axis(delta.y * SCALE_FACTOR, right) * camera_trs.rotation;

                // And apply rotation around the world up vector.
                camera_trs.rotation = gfx::quaternion_from_angle_axis(delta.x * SCALE_FACTOR, Vector3f(0.f, 1.f, 0.f)) * camera_trs.rotation;

                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                constexpr float SCALE_FACTOR = 0.02f;
                const auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);

                // The mouse moving right, the camera moving left.
                camera_trs.translation -= right * delta.x * SCALE_FACTOR;
                camera_trs.translation += up * delta.y * SCALE_FACTOR;

                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                scene_view_dragging_ = false;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
                scene_view_dragging_ = false;
            }
        }

        if (ImGui::IsWindowHovered()) {
            camera_trs.translation += forward * io.MouseWheel;
        }

        {
            view_inverse_ = gfx::trs(camera_trs.translation, camera_trs.rotation, camera_trs.scale);
            view_ = math::inv(view_inverse_);

            camera_state_.update_transform(view_inverse_);

            renderer_.render(scene_, camera_state_, render_target_view_.Get(), *rendering_context_);
        }

        {
            using namespace nodec_scene_editor;
            SceneGuiContext context{scene_.registry()};
            for (auto &pair : component_registry_) {
                pair.second->editor().on_scene_gui(scene_gizmo_, context);
            }
            scene_gizmo_renderer_->render(scene_, view_, projection_, *render_target_view_.Get(), *rendering_context_);
            scene_gizmo_renderer_->clear_gizmos(scene_);
        }

        ImGui::Image((void *)shader_resource_view_.Get(), ImVec2(view_width_, view_height_));

        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, view_width_, view_height_);

        // ImGuizmo::DrawGrid(view_.m, projection_.m, Matrix4x4f::identity.m, 100.f);
        // ImGuizmo::ViewManipulate(view_.m, 0.8f, ImVec2(view_manipulate_right - 128, view_manipulate_top), ImVec2(128, 128), 0x10101010);

        // Edit transformation.
        [&]() {
            SceneEntity selected_entity{nodec::entities::null_entity};

            {
                auto view = scene_.registry().view<nodec_scene_editor::components::Selected>();
                if (view.begin() == view.end()) return;
                selected_entity = *view.begin();
            }

            auto *local_to_world = scene_.registry().try_get_component<LocalToWorld>(selected_entity);
            if (!local_to_world) return;

            if (ImGuizmo::Manipulate(view_.m, projection_.m, gizmo_operation_, gizmo_mode_, local_to_world->value.m)) {
                local_to_world->dirty = true;
            }
        }();
    }
    ImGui::EndChild();
}

void SceneViewWindow::resize_view(Graphics &gfx, UINT width, UINT height) {
    if (width == 0 || height == 0) return;
    
    view_width_ = width;
    view_height_ = height;
    
    // リソースを再生成
    // 1. テクスチャの再生成
    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = view_width_;
    texture_desc.Height = view_height_;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // 古いリソースを解放
    texture_.Reset();
    render_target_view_.Reset();
    shader_resource_view_.Reset();
    
    ThrowIfFailedGfx(
        gfx.device().CreateTexture2D(&texture_desc, nullptr, &texture_),
        &gfx, __FILE__, __LINE__);

    // 2. レンダーターゲットビューの再生成
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc{};
        desc.Format = texture_desc.Format;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        ThrowIfFailedGfx(
            gfx.device().CreateRenderTargetView(texture_.Get(), &desc, &render_target_view_),
            &gfx, __FILE__, __LINE__);
    }

    // 3. シェーダーリソースビューの再生成
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Format = texture_desc.Format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;

        ThrowIfFailedGfx(
            gfx.device().CreateShaderResourceView(texture_.Get(), &desc, &shader_resource_view_),
            &gfx, __FILE__, __LINE__);
    }

    // 4. レンダリングコンテキストの再生成
    rendering_context_.reset(new SceneRenderingContext(view_width_, view_height_, gfx));

    // 5. プロジェクションマトリックスの更新
    {
        using namespace nodec_rendering::components;
        using namespace DirectX;

        const auto view_aspect = static_cast<float>(view_width_) / view_height_;
        Camera camera;
        camera.projection = Camera::Projection::Perspective;
        camera.far_clip_plane = 10000.0f;
        camera.near_clip_plane = 0.01f;
        camera.fov_angle = 45.0f;

        camera_state_.update_projection(camera, view_aspect);

        XMFLOAT4X4 matrix;
        XMStoreFloat4x4(&matrix, camera_state_.matrix_p());

        projection_.set(matrix.m[0], matrix.m[1], matrix.m[2], matrix.m[3]);
    }

    {
        auto config_block = static_cast<SceneViewSettings *>(editor_config_archive_.config().blocks["editor-backends.SceneView"].get());
        assert(config_block != nullptr);
        config_block->width = view_width_;
        config_block->height = view_height_;
        editor_config_archive_.save();
    }
}

void SceneViewWindow::resize_if_needed() {
    if (!size_changed_) {
        return;
    }
    resize_view(graphics_, input_width_, input_height_);
    size_changed_ = false;
}
