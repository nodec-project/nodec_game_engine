#ifndef NODEC_GAME_ENGINE__RENDERING__SCENE_RENDERER_HPP_
#define NODEC_GAME_ENGINE__RENDERING__SCENE_RENDERER_HPP_

#include <algorithm>
#include <cassert>
#include <unordered_set>

#include <DirectXMath.h>

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

#include "cb_model_properties.hpp"
#include "cb_scene_properties.hpp"
#include "cb_texture_config.hpp"
#include "material_backend.hpp"
#include "mesh_backend.hpp"
#include "scene_rendering_context.hpp"
#include "shader_backend.hpp"
#include "texture_backend.hpp"
#include <Font/FontCharacterDatabase.hpp>
#include <Graphics/BlendState.hpp>
#include <Graphics/ConstantBuffer.hpp>
#include <Graphics/GeometryBuffer.hpp>
#include <Graphics/RasterizerState.hpp>
#include <Graphics/SamplerState.hpp>
#include <Graphics/graphics.hpp>

// struct ShaderGroup1 {};

// class BaseRendererBackend {};

// class MeshRendererBackend : public BaseRendererBackend {
// public:

// };

class SceneRenderer {
    using TextureEntry = nodec_rendering::resources::Material::TextureEntry;

    static constexpr UINT SCENE_PROPERTIES_CB_SLOT = 0;
    static constexpr UINT TEXTURE_CONFIG_CB_SLOT = 1;
    static constexpr UINT MODEL_PROPERTIES_CB_SLOT = 2;
    static constexpr UINT MATERIAL_PROPERTIES_CB_SLOT = 3;

public:
    SceneRenderer(Graphics *gfx, nodec::resource_management::ResourceRegistry &resourceRegistry);

    // void Render(nodec_scene::Scene &scene, ID3D11RenderTargetView *pTarget, UINT width, UINT height);
    void Render(nodec_scene::Scene &scene, ID3D11RenderTargetView &render_target, SceneRenderingContext &context);

    void Render(nodec_scene::Scene &scene, nodec::Matrix4x4f &view, nodec::Matrix4x4f &projection,
                ID3D11RenderTargetView *pTarget, SceneRenderingContext &context);

private:
    void SetupSceneLighting(nodec_scene::Scene &scene);

    void Render(nodec_scene::Scene &scene,
                const DirectX::XMMATRIX &matrixV, const DirectX::XMMATRIX &matrixVInverse,
                const DirectX::XMMATRIX &matrixP, const DirectX::XMMATRIX &matrixPInverse,
                ID3D11RenderTargetView *pTarget, SceneRenderingContext &context);

    void render_model(nodec_scene::Scene &scene, ShaderBackend *activeShader,
                      const DirectX::XMMATRIX &matrixV, const DirectX::XMMATRIX &matrixP);

    void set_cull_mode(const nodec_rendering::CullMode &mode) {
        using namespace nodec_rendering;
        switch (mode) {
        default:
        case CullMode::Back:
            rs_cull_back_.bind();
            break;
        case CullMode::Front:
            rs_cull_front_.bind();
            break;
        case CullMode::Off:
            rs_cull_none_.bind();
            break;
        }
    }

    /**
     * @brief
     *
     * @param textureEntries
     * @param texHasFlag
     * @return UINT next slot number.
     */
    UINT bind_texture_entries(const std::vector<TextureEntry> &textureEntries, uint32_t &texHasFlag);

    void unbind_all_shader_resources(UINT start, UINT count) {
        assert(count <= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

        static ID3D11ShaderResourceView *const nulls[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{nullptr};

        gfx_->context().VSSetShaderResources(start, count, nulls);
        gfx_->context().PSSetShaderResources(start, count, nulls);
    }

    void unbind_all_shader_resources(UINT count) {
        unbind_all_shader_resources(0, count);
    }

    SamplerState &GetSamplerState(const nodec_rendering::Sampler &sampler) {
        auto &state = mSamplerStates[sampler];
        if (state) return *state;

        state = SamplerState::Create(gfx_, sampler);
        return *state;
    }

private:
    std::shared_ptr<nodec::logging::Logger> logger_;

    // slot 0
    CBSceneProperties cb_scene_properties_;

    // slot 1
    CBTextureConfig cb_texture_config_;

    // slot 2
    CBModelProperties cb_model_properties_;

    std::unordered_map<nodec_rendering::Sampler, nodec::optional<SamplerState>> mSamplerStates;

    RasterizerState rs_cull_none_;
    RasterizerState rs_cull_front_;
    RasterizerState rs_cull_back_;

    BlendState bs_default_;
    BlendState bs_alpha_blend_;

    Graphics *gfx_;

    std::shared_ptr<MeshBackend> quad_mesh_;
    std::unique_ptr<MeshBackend> screen_quad_mesh_;
    std::shared_ptr<MeshBackend> norm_cube_mesh_;

    FontCharacterDatabase mFontCharacterDatabase;
};

#endif