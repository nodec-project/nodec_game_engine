#ifndef NODEC_GAME_ENGINE__RENDERING__CB_MODEL_PROPERTIES_HPP_
#define NODEC_GAME_ENGINE__RENDERING__CB_MODEL_PROPERTIES_HPP_

#include <DirectXMath.h>

#include "../graphics/ConstantBuffer.hpp"
#include "../graphics/graphics.hpp"

class CBModelProperties {
public:
    struct ModelProperties {
        DirectX::XMFLOAT4X4 matrix_mvp;
        DirectX::XMFLOAT4X4 matrix_m;
        DirectX::XMFLOAT4X4 matrix_m_inverse;
    };

    CBModelProperties(Graphics &gfx)
        : buffer_(gfx, sizeof(ModelProperties), &data_) {}

    ConstantBuffer &buffer() noexcept {
        return buffer_;
    }

    ModelProperties &data() noexcept {
        return data_;
    }

    void apply() {
        buffer_.update(&data_);
    }

private:
    ModelProperties data_;
    ConstantBuffer buffer_;
};

#endif