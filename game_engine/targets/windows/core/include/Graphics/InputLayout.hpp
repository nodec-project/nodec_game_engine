#pragma once

#include "Graphics.hpp"

class InputLayout
{
public:
    InputLayout(
        Graphics* pGraphics,
        const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
        UINT numElements,
        const void* pShaderBytecode,
        SIZE_T bytecodeLength) {

        ThrowIfFailedGfx(
            pGraphics->device().CreateInputLayout(
                pInputElementDescs,
                numElements,
                pShaderBytecode,
                bytecodeLength,
                &mpInputLayout
            ),
            pGraphics, __FILE__, __LINE__
        );

    }

    void Bind(Graphics* pGraphics) {
        pGraphics->context().IASetInputLayout(mpInputLayout.Get());

        // NOTE: The following code is too heavy to run for each model.
        // const auto logs = pGraphics->info_logger().Dump();
        // if (!logs.empty()) {
        //     nodec::logging::WarnStream(__FILE__, __LINE__)
        //         << "[InputLayout::Bind] >>> DXGI Logs:"
        //         << logs;
        // }
    }

private:
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mpInputLayout;
};