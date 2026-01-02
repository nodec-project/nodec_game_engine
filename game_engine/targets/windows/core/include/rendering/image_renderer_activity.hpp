#ifndef NODEC_GAME_ENGINE__RENDERING__IMAGE_RENDERER_ACTIVITY_HPP_
#define NODEC_GAME_ENGINE__RENDERING__IMAGE_RENDERER_ACTIVITY_HPP_

#include <DirectXMath.h>

#include <nodec/array_view.hpp>
#include <nodec/gfx/gfx.hpp>
#include <nodec_rendering/components/image_renderer.hpp>
#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene/scene_entity.hpp>

#include "camera_state.hpp"
#include "draw_command.hpp"

class ImageDrawCommand : public DrawCommand {
public:
    ImageDrawCommand() {}

    void set(const DirectX::XMMATRIX &matrix_m,
             std::shared_ptr<TextureBackend> image,
             std::shared_ptr<MaterialBackend> material,
             const nodec::Vector4f &color) {
        matrix_m_ = matrix_m;
        image_ = image;
        material_ = material;
        color_ = color;
    }

    void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
              SceneRendererContext &renderer_context, Graphics &gfx) override {
        using namespace DirectX;
        renderer_context.bs_alpha_blend().bind();
        // renderer_context.bs_default().bind();

        auto matrix_m_inverse = DirectX::XMMatrixInverse(nullptr, matrix_m_);
        auto matrix_mvp = matrix_m_ * matrix_v * matrix_p;

        auto &cb_model_properties = renderer_context.cb_model_properties();

        XMStoreFloat4x4(&cb_model_properties.data().matrix_m, matrix_m_);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_m_inverse, matrix_m_inverse);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_mvp, matrix_mvp);
        cb_model_properties.apply();

        auto backup_image = material_->get_texture_entry("image");
        auto backup_color = material_->get_vector4_property("color");

        material_->set_texture_entry(
            "image", {image_, {nodec_rendering::Sampler::FilterMode::Bilinear, nodec_rendering::Sampler::WrapMode::Clamp}});
        material_->set_vector4_property("color", color_);

        renderer_context.bind_material(material_.get(), true);

        auto &mesh = renderer_context.quad_mesh();
        mesh.bind(&gfx);
        gfx.DrawIndexed(static_cast<UINT>(mesh.triangles.size()));

        if (backup_image) {
            material_->set_texture_entry("image", *backup_image);
        }
        if (backup_color) {
            material_->set_vector4_property("color", *backup_color);
        }
    }

    DirectX::XMMATRIX matrix_m_;
    std::shared_ptr<TextureBackend> image_;
    std::shared_ptr<MaterialBackend> material_;
    nodec::Vector4f color_;
};

class ImageRendererActivity {
public:
    ImageRendererActivity()
        : command_(new ImageDrawCommand()) {
    }

    ImageDrawCommand *get_commands_if_needed(
        const CameraState &camera_state,
        nodec_scene::SceneEntity entity,
        const nodec_rendering::components::ImageRenderer &renderer,
        const nodec_scene::components::LocalToWorld &local_to_world) {
        using namespace DirectX;
        using namespace nodec;

        auto &image = renderer.image;
        auto &material = renderer.material;
        if (!image || !material) return nullptr;

        auto image_backend = std::static_pointer_cast<TextureBackend>(image);
        auto material_backend = std::static_pointer_cast<MaterialBackend>(material);
        auto shader_backend = std::static_pointer_cast<ShaderBackend>(material->shader());
        if (!shader_backend) return nullptr;

        const auto width = image_backend->width() / (renderer.pixels_per_unit + std::numeric_limits<float>::epsilon());
        const auto height = image_backend->height() / (renderer.pixels_per_unit + std::numeric_limits<float>::epsilon());

        // auto local_to_world_matrix = local_to_world.value * gfx::scale_matrix(width, height, 1.0f);
        const gfx::BoundingBox bounds(Vector3f::zero, Vector3f(width, height, 0.0f));
        if (!nodec::gfx::intersects(camera_state.frustum(), bounds, local_to_world.value)) {
            return nullptr;
        }

        auto matrix_m = XMMatrixScaling(width, height, 1.f) * XMMATRIX(local_to_world.value.m);
        command_->set(matrix_m,
                      image_backend,
                      material_backend,
                      renderer.color);
        return command_.get();
    }

private:
    std::unique_ptr<ImageDrawCommand> command_;
};

#endif