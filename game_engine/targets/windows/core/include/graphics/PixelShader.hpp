#ifndef NODEC_GAME_ENGINE__GRAPHICS__PIXEL_SHADER_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__PIXEL_SHADER_HPP_

#include <d3dcompiler.h>

#include <nodec/unicode.hpp>

#include "graphics.hpp"

class PixelShader {
public:
    PixelShader(Graphics &gfx, const std::string &path)
        : gfx_(gfx) {
        Microsoft::WRL::ComPtr<ID3DBlob> blob;

        ThrowIfFailedGfx(
            D3DReadFileToBlob(nodec::unicode::utf8to16<std::wstring>(path).c_str(), &blob),
            &gfx, __FILE__, __LINE__);

        ThrowIfFailedGfx(
            gfx.device().CreatePixelShader(
                blob->GetBufferPointer(), blob->GetBufferSize(),
                nullptr, &pixel_shader_),
            &gfx, __FILE__, __LINE__);
    }

    void bind() {
        gfx_.context().PSSetShader(pixel_shader_.Get(), nullptr, 0u);
    }

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader_;
};

#endif