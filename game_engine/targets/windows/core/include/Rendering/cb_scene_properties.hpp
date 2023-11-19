#ifndef NODEC_GAME_ENGINE__RENDERING__CB_SCENE_PROPERTIES_HPP_
#define NODEC_GAME_ENGINE__RENDERING__CB_SCENE_PROPERTIES_HPP_

#include <cstdint>

#include <nodec/vector3.hpp>
#include <nodec/vector4.hpp>

#include <DirectXMath.h>

#include "../Graphics/ConstantBuffer.hpp"
#include "../Graphics/Graphics.hpp"

class CBSceneProperties {
public:
    struct DirectionalLight {
        nodec::Vector3f direction;
        float intensity;
        nodec::Vector4f color;
        std::uint32_t enabled{0x00};
        std::uint32_t padding[3];
    };

    struct PointLight {
        nodec::Vector3f position;
        float range;
        nodec::Vector3f color;
        float intensity;
    };

    struct SceneLighting {
        nodec::Vector4f ambient_color;
        std::uint32_t num_of_point_lights;
        std::uint32_t padding[3];
        DirectionalLight directional;
        PointLight point_lights[1024];
    };

    struct SceneProperties {
        nodec::Vector4f camera_position;
        DirectX::XMFLOAT4X4 matrix_p;
        DirectX::XMFLOAT4X4 matrix_p_inverse;
        DirectX::XMFLOAT4X4 matrix_v;
        DirectX::XMFLOAT4X4 matrix_v_inverse;
        SceneLighting lights;
    };

    CBSceneProperties(Graphics &gfx)
        : buffer_(gfx, sizeof(SceneProperties), &data_) {}

    ConstantBuffer &buffer() noexcept {
        return buffer_;
    }

    SceneProperties &data() noexcept {
        return data_;
    }

    void apply() {
        buffer_.update(&data_);
    }

private:
    SceneProperties data_;
    ConstantBuffer buffer_;
};

#endif