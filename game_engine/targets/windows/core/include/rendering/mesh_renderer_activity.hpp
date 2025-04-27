#ifndef NODEC_GAME_ENGINE__RENDERING__MESH_RENDERER_ACTIVITY_HPP_
#define NODEC_GAME_ENGINE__RENDERING__MESH_RENDERER_ACTIVITY_HPP_

#include <DirectXMath.h>

#include <nodec/array_view.hpp>
#include <nodec/gfx/gfx.hpp>
#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene/scene_entity.hpp>

#include "camera_state.hpp"
#include "draw_command.hpp"

class MeshDrawCommand : public DrawCommand {
public:
    MeshDrawCommand() {}

    void set(const DirectX::XMMATRIX &matrix_m,
             std::shared_ptr<MeshBackend> mesh,
             std::shared_ptr<MaterialBackend> material) {
        matrix_m_ = matrix_m;
        mesh_ = mesh;
        material_ = material;
    }

    void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
              SceneRendererContext &renderer_context, Graphics &gfx) override {
        using namespace DirectX;
        renderer_context.bs_default().bind();
        auto matrix_m_inverse = DirectX::XMMatrixInverse(nullptr, matrix_m_);
        auto matrix_mvp = matrix_m_ * matrix_v * matrix_p;

        auto &cb_model_properties = renderer_context.cb_model_properties();

        XMStoreFloat4x4(&cb_model_properties.data().matrix_m, matrix_m_);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_m_inverse, matrix_m_inverse);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_mvp, matrix_mvp);
        cb_model_properties.apply();

        renderer_context.bind_material(material_.get());

        mesh_->bind(&gfx);
        gfx.DrawIndexed(static_cast<UINT>(mesh_->triangles.size()));
    }

    DirectX::XMMATRIX matrix_m_;
    std::shared_ptr<MeshBackend> mesh_;
    std::shared_ptr<MaterialBackend> material_;
};

class MeshRendererActivity {
public:
    MeshRendererActivity() {
    }

    nodec::ArrayView<MeshDrawCommand *> get_commands_if_needed(
        const CameraState &camera_state,
        nodec_scene::SceneEntity entity,
        const nodec_rendering::components::MeshRenderer &renderer,
        const nodec_scene::components::LocalToWorld &local_to_world) {
        using namespace DirectX;
        if (renderer.meshes.size() != renderer.materials.size()) return {};

        commands_.resize(renderer.meshes.size());
        commands_for_return_.resize(commands_.size());

        int index = 0;
        for (int i = 0; i < renderer.meshes.size(); ++i) {
            auto &mesh = renderer.meshes[i];
            auto &material = renderer.materials[i];
            if (!mesh || !material) continue;

            auto mesh_backend = std::static_pointer_cast<MeshBackend>(mesh);
            auto material_backend = std::static_pointer_cast<MaterialBackend>(material);
            auto shader_backend = std::static_pointer_cast<ShaderBackend>(material->shader());
            if (!shader_backend) continue;

            // フラスタムカリングを適用
            if (!nodec::gfx::intersects(camera_state.frustum(), mesh_backend->bounds, local_to_world.value)) {
                return {};
            }

            const auto matrix_m = XMMATRIX(local_to_world.value.m);
            // const auto matrix_mvp = matrix_m * matrix_vp;
            // auto pixel_size = calculate_screen_pixel_size(matrix_mvp, mesh_backend->bounds,
            //                                               context.target_width(), context.target_height());
            // if (pixel_size < 3.0f) {
            //     return;
            // }

            //// オブジェクトのIDを生成（エンティティIDを使用）
            //// uintptr_t object_id = reinterpret_cast<uintptr_t>(entity.raw_handle());
            // uintptr_t object_id = static_cast<uintptr_t>(entity);

            //// オクルージョンカリングシステムにオブジェクトを登録
            // occlusion_system_.register_object(object_id, matrix_m, mesh_backend);

            //// オブジェクトが可視かチェック
            // if (!occlusion_system_.is_visible(object_id)) {
            //     // 不可視の場合はスキップ
            //     continue;
            // }

            // const bool is_transparent = material_backend->is_transparent();
            auto &command = commands_[index];
            if (!command) {
                command = std::make_unique<MeshDrawCommand>();
            }
            command->set(matrix_m, mesh_backend, material_backend);
            commands_for_return_[index] = command.get();
            ++index;
        }
        return nodec::ArrayView<MeshDrawCommand*>{commands_for_return_.data(), static_cast<std::size_t>(index)};
    }

private:
    std::vector<std::unique_ptr<MeshDrawCommand>> commands_;
    std::vector<MeshDrawCommand *> commands_for_return_;
};

#endif