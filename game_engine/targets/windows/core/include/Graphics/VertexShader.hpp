#ifndef NODEC_GAME_ENGINE__GRAPHICS__VERTEX_SHADER_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__VERTEX_SHADER_HPP_

#include <d3dcompiler.h>

#include <nodec/unicode.hpp>

#include "Graphics.hpp"

class VertexShader {
public:
    VertexShader(Graphics &gfx, const std::string &path)
        : gfx_(gfx) {
        ThrowIfFailedGfx(
            D3DReadFileToBlob(nodec::unicode::utf8to16<std::wstring>(path).c_str(), &bytecode_blob_),
            &gfx, __FILE__, __LINE__);

        ThrowIfFailedGfx(
            gfx.device().CreateVertexShader(
                bytecode_blob_->GetBufferPointer(), bytecode_blob_->GetBufferSize(),
                nullptr, &vertex_shader_),
            &gfx, __FILE__, __LINE__);
    }

    void bind() {
        gfx_.context().VSSetShader(vertex_shader_.Get(), nullptr, 0u);
    }

    ID3DBlob &bytecode() {
        return *bytecode_blob_.Get();
    }

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader_;
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode_blob_;
};

#endif