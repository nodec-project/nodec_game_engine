#include <nodec/resource_management/resource_registry.hpp>
#include <nodec_rendering/cull_mode.hpp>
#include <nodec_rendering/resources/material.hpp>

#include "../Font/FontCharacterDatabase.hpp"
#include "../graphics/blend_state.hpp"
#include "../graphics/ConstantBuffer.hpp"
#include "../graphics/RasterizerState.hpp"
#include "../graphics/SamplerState.hpp"
#include "../graphics/graphics.hpp"
#include "cb_model_properties.hpp"
#include "cb_scene_properties.hpp"
#include "cb_texture_config.hpp"
#include "mesh_backend.hpp"
#include "texture_backend.hpp"

struct SceneRenderingConstants {
    static constexpr UINT SCENE_PROPERTIES_CB_SLOT = 0;
    static constexpr UINT TEXTURE_CONFIG_CB_SLOT = 1;
    static constexpr UINT MODEL_PROPERTIES_CB_SLOT = 2;
    static constexpr UINT MATERIAL_PROPERTIES_CB_SLOT = 3;
};

class SceneRendererContext {
public:
    SceneRendererContext(std::shared_ptr<nodec::logging::Logger>, Graphics &,
                         nodec::resource_management::ResourceRegistry &);

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

    CBSceneProperties &cb_scene_properties() {
        return cb_scene_properties_;
    }

    CBTextureConfig &cb_texture_config() {
        return cb_texture_config_;
    }

    CBModelProperties &cb_model_properties() {
        return cb_model_properties_;
    }

    SamplerState &sampler_state(const nodec_rendering::Sampler &sampler) {
        auto &state = sampler_states_[sampler];
        if (state) return *state;

        state = SamplerState::Create(&gfx_, sampler);
        return *state;
    }

    UINT bind_texture_entries(const std::vector<nodec_rendering::resources::Material::TextureEntry> &texture_entries,
                              uint32_t &tex_has_flag) {
        cb_texture_config_.data().tex_has_flag = 0;
        UINT slot = 0;

        for (auto &entry : texture_entries) {
            if (!entry.texture) {
                // texture not setted.
                // skip bind texture,
                // but bind sampler because the The Pixel Shader unit expects a Sampler to be set at Slot 0.
                auto &defaultSamplerState = sampler_state({});
                defaultSamplerState.BindPS(&gfx_, slot);
                defaultSamplerState.BindVS(&gfx_, slot);

                ++slot;
                continue;
            }

            auto *textureBackend = static_cast<TextureBackend *>(entry.texture.get());
            {
                auto *view = &textureBackend->shader_resource_view();
                gfx_.context().VSSetShaderResources(slot, 1u, &view);
                gfx_.context().PSSetShaderResources(slot, 1u, &view);
            }

            auto &samplerState = sampler_state(entry.sampler);
            samplerState.BindVS(&gfx_, slot);
            samplerState.BindPS(&gfx_, slot);

            tex_has_flag |= 0x01 << slot;

            ++slot;
        }
        return slot;
    }

    void unbind_all_shader_resources(UINT start, UINT count) {
        assert(count <= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

        static ID3D11ShaderResourceView *const nulls[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{nullptr};

        gfx_.context().VSSetShaderResources(start, count, nulls);
        gfx_.context().PSSetShaderResources(start, count, nulls);
    }

    void unbind_all_shader_resources(UINT count) {
        unbind_all_shader_resources(0, count);
    }

    /**
     * @brief 0.5 x 0.5 quad.
     */
    MeshBackend &quad_mesh() {
        return *quad_mesh_;
    }

    /**
     * @brief 1 x 1 x 1 cube with normal vectors.
     */
    MeshBackend &norm_cube_mesh() {
        return *norm_cube_mesh_;
    }

    /**
     * @brief 1 x 1 quad.
     */
    MeshBackend &screen_quad_mesh() {
        return *screen_quad_mesh_;
    }

    BlendState &bs_default() {
        return bs_default_;
    }

    BlendState &bs_alpha_blend() {
        return bs_alpha_blend_;
    }

    FontCharacterDatabase &font_character_database() {
        return font_character_database_;
    }

private:
    std::shared_ptr<nodec::logging::Logger> logger_;

    Graphics &gfx_;

    FontCharacterDatabase font_character_database_;

    // slot 0
    CBSceneProperties cb_scene_properties_;

    // slot 1
    CBTextureConfig cb_texture_config_;

    // slot 2
    CBModelProperties cb_model_properties_;

    RasterizerState rs_cull_none_;
    RasterizerState rs_cull_front_;
    RasterizerState rs_cull_back_;

    std::unordered_map<nodec_rendering::Sampler, nodec::optional<SamplerState>> sampler_states_;

    std::shared_ptr<MeshBackend> quad_mesh_;
    std::unique_ptr<MeshBackend> screen_quad_mesh_;
    std::shared_ptr<MeshBackend> norm_cube_mesh_;

    BlendState bs_default_;
    BlendState bs_alpha_blend_;
};
