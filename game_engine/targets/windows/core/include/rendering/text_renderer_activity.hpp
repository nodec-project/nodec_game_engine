#ifndef NODEC_GAME_ENGINE__RENDERING__TEXT_RENDERER_ACTIVITY_HPP_
#define NODEC_GAME_ENGINE__RENDERING__TEXT_RENDERER_ACTIVITY_HPP_

#include <DirectXMath.h>

#include <nodec/array_view.hpp>
#include <nodec/gfx/gfx.hpp>
#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_rendering/components/text_renderer.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene/scene_entity.hpp>

#include "../Font/FontBackend.hpp"
#include "camera_state.hpp"
#include "draw_command.hpp"

class TextDrawCommand : public DrawCommand {
public:
    TextDrawCommand() {}

    void set(const DirectX::XMMATRIX &matrix_m,
             const nodec_rendering::components::TextRenderer *text_renderer) {
        matrix_m_ = matrix_m;
        text_renderer_ = text_renderer;
        material_ = std::static_pointer_cast<MaterialBackend>(text_renderer_->material);
        shader_ = std::static_pointer_cast<ShaderBackend>(material_->shader());
    }

    void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
              SceneRendererContext &renderer_context, Graphics &gfx) override {
        using namespace DirectX;
        renderer_context.bs_alpha_blend().bind();
        // renderer_context.bs_default().bind(&gfx);

        auto material = static_cast<MaterialBackend *>(text_renderer_->material.get());
        assert(material);

        auto font = static_cast<FontBackend *>(text_renderer_->font.get());
        assert(font);

        auto backup_mask = material->get_texture_entry("mask");
        auto backup_color = material->get_vector4_property("color");

        const auto u32_text = nodec::unicode::utf8to32<std::u32string>(text_renderer_->text);
        const float pixels_per_unit = static_cast<float>(text_renderer_->pixels_per_unit);
        const float pixel_size = text_renderer_->pixel_size;

        auto &font_character_database = renderer_context.font_character_database();

        float offset_x = 0.0f;
        float offset_y = 0.0f;

        for (const auto &chCode : u32_text) {
            if (chCode == '\n') {
                offset_y -= pixel_size / pixels_per_unit;
                offset_x = 0.0f;
                continue;
            }

            const auto &character = font_character_database.Get(font->GetFace(), pixel_size, chCode);

            float pos_x = offset_x + character.bearing.x / pixels_per_unit;
            float pos_y = offset_y - (character.size.y - character.bearing.y) / pixels_per_unit;
            float w = character.size.x / pixels_per_unit;
            float h = character.size.y / pixels_per_unit;

            offset_x += (character.advance >> 6) / pixels_per_unit;

            if (!character.pFontTexture) {
                continue;
            }

            material->set_texture_entry("mask",
                                        {character.pFontTexture, {nodec_rendering::Sampler::FilterMode::Bilinear, nodec_rendering::Sampler::WrapMode::Clamp}});
            material->set_vector4_property("color", text_renderer_->color);

            renderer_context.bind_material(material, true);

            XMMATRIX matrix_m_ch{matrix_m_};
            matrix_m_ch = XMMatrixScaling(w / 2, h / 2, 1.0f) * XMMatrixTranslation(pos_x + w / 2, pos_y + h / 2, 0.0f) * matrix_m_ch;

            // matrixM
            auto matrix_m_inverse = XMMatrixInverse(nullptr, matrix_m_ch);

            // DirectX Math using row-major representation
            // HLSL using column-major representation
            auto matrix_mvp = matrix_m_ch * matrix_v * matrix_p;

            auto &cb_model_properties = renderer_context.cb_model_properties();

            XMStoreFloat4x4(&cb_model_properties.data().matrix_m, matrix_m_ch);
            XMStoreFloat4x4(&cb_model_properties.data().matrix_m_inverse, matrix_m_inverse);
            XMStoreFloat4x4(&cb_model_properties.data().matrix_mvp, matrix_mvp);

            cb_model_properties.apply();

            auto &mesh = renderer_context.screen_quad_mesh();
            mesh.bind(&gfx);
            gfx.DrawIndexed(static_cast<UINT>(mesh.triangles.size()));
        } // End foreach character.
        if (backup_mask) {
            material->set_texture_entry("mask", *backup_mask);
        }

        if (backup_color) {
            material->set_vector4_property("color", *backup_color);
        }
    }

    DirectX::XMMATRIX matrix_m_;
    const nodec_rendering::components::TextRenderer *text_renderer_;
    std::shared_ptr<MaterialBackend> material_;
    std::shared_ptr<ShaderBackend> shader_;
};

class TextRendererActivity {
public:
    TextRendererActivity()
        : command_(std::make_unique<TextDrawCommand>()) {
    }

    TextDrawCommand *get_command_if_needed(
        const CameraState &camera_state,
        nodec_scene::SceneEntity entity,
        const nodec_rendering::components::TextRenderer &renderer,
        const nodec_scene::components::LocalToWorld &local_to_world) {
        using namespace DirectX;

        auto &material = renderer.material;
        auto &font = renderer.font;
        if (!material || !font) return nullptr;

        auto material_backend = std::static_pointer_cast<MaterialBackend>(material);
        auto shader_backend = std::static_pointer_cast<ShaderBackend>(material->shader());
        if (!shader_backend) return nullptr;

        auto matrix_m = XMMATRIX(local_to_world.value.m);
        command_->set(matrix_m, &renderer);

        return command_.get();
    }

private:
    std::unique_ptr<TextDrawCommand> command_;
};

#endif