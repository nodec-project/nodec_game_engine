#ifndef NODEC_GAME_ENGINE__RENDERING__CB_TEXTURE_CONFIG_HPP_
#define NODEC_GAME_ENGINE__RENDERING__CB_TEXTURE_CONFIG_HPP_

#include <cstdint>

#include "../Graphics/ConstantBuffer.hpp"
#include "../Graphics/Graphics.hpp"

class CBTextureConfig {
public:
    struct TextureConfig {
        std::uint32_t tex_has_flag;
        std::uint32_t padding[3];
    };

    CBTextureConfig(Graphics &gfx)
        : buffer_(gfx, sizeof(TextureConfig), &data_) {}

    ConstantBuffer &buffer() noexcept {
        return buffer_;
    }

    TextureConfig &data() noexcept {
        return data_;
    }

    void apply() {
        buffer_.update(&data_);
    }

private:
    TextureConfig data_;
    ConstantBuffer buffer_;
};

#endif