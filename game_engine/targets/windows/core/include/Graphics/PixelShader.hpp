#pragma once

#include "Graphics.hpp"

#include <nodec/unicode.hpp>

#include <d3dcompiler.h>


class PixelShader {
public:
    PixelShader(Graphics* pGraphics, const std::string& path) {
        Microsoft::WRL::ComPtr<ID3DBlob> pBlob;

        ThrowIfFailedGfx(
            D3DReadFileToBlob(nodec::unicode::utf8to16<std::wstring>(path).c_str(), &pBlob),
            pGraphics, __FILE__, __LINE__
        );

        ThrowIfFailedGfx(
            pGraphics->device().CreatePixelShader(
                pBlob->GetBufferPointer(), pBlob->GetBufferSize(),
                nullptr, &mpPixelShader
            ),
            pGraphics, __FILE__, __LINE__
        );
    }

    void Bind(Graphics* pGraphics) {
        pGraphics->context().PSSetShader(mpPixelShader.Get(), nullptr, 0u);

        // NOTE: The following code is too heavy to run for each model.
        // const auto logs = pGraphics->info_logger().Dump();
        // if (!logs.empty()) {
        //     nodec::logging::WarnStream(__FILE__, __LINE__)
        //         << "[PixelShader::Bind] >>> DXGI Logs:"
        //         << logs;
        // }
    }

private:
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mpPixelShader;
};