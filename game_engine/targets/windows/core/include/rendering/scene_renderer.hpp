#ifndef NODEC_GAME_ENGINE__RENDERING__SCENE_RENDERER_HPP_
#define NODEC_GAME_ENGINE__RENDERING__SCENE_RENDERER_HPP_

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include <DirectXMath.h>
#include <wrl/client.h>

#include <nodec/logging/logging.hpp>
#include <nodec/resource_management/resource_registry.hpp>
#include <nodec/vector4.hpp>
#include <nodec_rendering/components/camera.hpp>
#include <nodec_rendering/components/directional_light.hpp>
#include <nodec_rendering/components/image_renderer.hpp>
#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_rendering/components/non_visible.hpp>
#include <nodec_rendering/components/point_light.hpp>
#include <nodec_rendering/components/post_processing.hpp>
#include <nodec_rendering/components/scene_lighting.hpp>
#include <nodec_rendering/components/text_renderer.hpp>
#include <nodec_rendering/sampler.hpp>
#include <nodec_scene/components/local_transform.hpp>
#include <nodec_scene/scene.hpp>

#include "../graphics/ConstantBuffer.hpp"
#include "../graphics/RasterizerState.hpp"
#include "../graphics/SamplerState.hpp"
#include "../graphics/geometry_buffer.hpp"
#include "../graphics/graphics.hpp"
#include "camera_state.hpp"
#include "draw_command.hpp"
#include "material_backend.hpp"
#include "mesh_backend.hpp"
#include "scene_renderer_context.hpp"
#include "scene_rendering_context.hpp"
#include "shader_backend.hpp"
#include "texture_backend.hpp"

class DrawGroup {
public:
    std::weak_ptr<ShaderBackend> shader;

    virtual ~DrawGroup() {}

    virtual void draw_all(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
                          SceneRendererContext &context, Graphics &gfx) = 0;

    virtual void clear_draw_commands() = 0;
};

class OpaqueDrawGroup : public DrawGroup {
public:
    std::unordered_map<std::intptr_t, std::vector<DrawCommand *>> draw_commands;

    void draw_all(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
                  SceneRendererContext &context, Graphics &gfx) override {
        for (auto &material_group : draw_commands) {
            for (auto &command : material_group.second) {
                command->draw(matrix_v, matrix_p, context, gfx);
            }
        }
    }

    void append_draw_command(const std::shared_ptr<MaterialBackend> &material_backend,
                             DrawCommand *command) {
        auto material_id = reinterpret_cast<std::intptr_t>(material_backend.get());
        auto &material_group = draw_commands[material_id];
        material_group.push_back(command);
    }

    void clear_draw_commands() override {
        for (auto &material_group : draw_commands) {
            material_group.second.clear();
        }
    }
};

class TransparentDrawGroup : public DrawGroup {
public:
    std::multimap<float, DrawCommand *> draw_commands;
    void draw_all(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
                  SceneRendererContext &context, Graphics &gfx) override {
        for (auto iter = draw_commands.rbegin(); iter != draw_commands.rend(); ++iter) {
            iter->second->draw(matrix_v, matrix_p, context, gfx);
        }
    }

    void clear_draw_commands() override {
        draw_commands.clear();
    }
};

struct DrawGroupPriorityKey {
    DrawGroupPriorityKey(std::shared_ptr<ShaderBackend> &shader, bool is_transparent = false)
        : is_transparent_(is_transparent) {
        assert(shader);
        shader_id_ = reinterpret_cast<std::intptr_t>(shader.get());
        priority_ = shader->rendering_priority();
    }

    bool operator==(const DrawGroupPriorityKey &other) const {
        return shader_id_ == other.shader_id_ && is_transparent_ == other.is_transparent_;
    }

    bool operator<(const DrawGroupPriorityKey &other) const {
        if (is_transparent_ == other.is_transparent_) {
            return priority_ < other.priority_;
        }
        return !is_transparent_;
    }

private:
    std::intptr_t shader_id_;
    int priority_;
    bool is_transparent_;
};

// オクルージョンクエリのための構造体
struct OcclusionQueryInstance {
    Microsoft::WRL::ComPtr<ID3D11Query> query;
    bool visible;
    bool query_issued;
    uint32_t last_visible_frame;
    DirectX::XMMATRIX local_to_world;
    std::shared_ptr<MeshBackend> mesh;
};

// オクルージョンカリング管理クラス
class OcclusionCullingSystem {
public:
    OcclusionCullingSystem(Graphics &gfx);
    ~OcclusionCullingSystem();

    // オブジェクトのオクルージョンクエリを作成または更新
    void register_object(uintptr_t object_id, const DirectX::XMMATRIX &local_to_world, std::shared_ptr<MeshBackend> mesh);

    // オクルージョンテストを開始
    void begin_occlusion_test(const DirectX::XMMATRIX &view_proj);

    // オクルージョンテストの結果を取得
    bool is_visible(uintptr_t object_id);

    // フレーム終了時の処理
    void end_frame();

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> occlusion_depth_state_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> occlusion_raster_state_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> bbox_vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> bbox_ps_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_buffer_;

    std::unordered_map<uintptr_t, OcclusionQueryInstance> query_instances_;
    uint32_t current_frame_index_;
    static constexpr uint32_t VISIBILITY_THRESHOLD = 3; // フレーム数のしきい値
};

class SceneRenderer {
public:
    SceneRenderer(nodec_scene::Scene &, Graphics &, nodec::resource_management::ResourceRegistry &);

    void render(nodec_scene::Scene &scene, ID3D11RenderTargetView &render_target, SceneRenderingContext &context);

    void render(nodec_scene::Scene &scene, const CameraState &camera_state, ID3D11RenderTargetView *render_target, SceneRenderingContext &context);

private:
    void setup_scene_lighting(nodec_scene::Scene &scene);

    void render_internal(nodec_scene::Scene &scene,
                         const CameraState &camera_state,
                         ID3D11RenderTargetView *target, SceneRenderingContext &context);

    void push_draw_command(std::shared_ptr<ShaderBackend> shader, bool is_transparent,
                           const std::shared_ptr<MaterialBackend> &material_backend,
                           DrawCommand *command,
                           const DirectX::XMMATRIX &matrix_m, const DirectX::XMMATRIX &matrix_v_inverse);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    nodec_scene::Scene &scene_;
    Graphics &gfx_;

    SceneRendererContext renderer_context_;

    std::map<DrawGroupPriorityKey, std::unique_ptr<DrawGroup>> draw_groups_;
    // OcclusionCullingSystem occlusion_system_; // オクルージョンカリングシステムの追加
};

#endif