#pragma once

#include "Graphics.hpp"

class ConstantBuffer {
public:
    ConstantBuffer(Graphics* pGraphics, UINT sizeBytes, const void* pSysMem)
        : mSizeBytes(sizeBytes) {
        D3D11_BUFFER_DESC cbd;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbd.MiscFlags = 0u;
        cbd.ByteWidth = sizeBytes;
        cbd.StructureByteStride = 0u;

        D3D11_SUBRESOURCE_DATA csd = {};
        csd.pSysMem = pSysMem;

        ThrowIfFailedGfx(
            pGraphics->device().CreateBuffer(&cbd, &csd, &mpConstantBuffer),
            pGraphics, __FILE__, __LINE__
        );
    }


    void Update(Graphics* pGraphics, const void* pSysMem) {
        D3D11_MAPPED_SUBRESOURCE msr;
        ThrowIfFailedGfx(
            pGraphics->context().Map(
                mpConstantBuffer.Get(), 0u,
                D3D11_MAP_WRITE_DISCARD, 0u,
                &msr
            ),
            pGraphics, __FILE__, __LINE__
        );
        memcpy(msr.pData, pSysMem, mSizeBytes);
        pGraphics->context().Unmap(mpConstantBuffer.Get(), 0u);
    }


    void BindVS(Graphics* pGraphics, UINT slot) {
        pGraphics->context().VSSetConstantBuffers(slot, 1u, mpConstantBuffer.GetAddressOf());

        // NOTE: The following code is too heavy to run for each model.
        // const auto logs = pGraphics->info_logger().Dump();
        // if (!logs.empty()) {
        //     nodec::logging::WarnStream(__FILE__, __LINE__)
        //         << "[ConstantBuffer::BindVS] >>> DXGI Logs:"
        //         << logs;
        // }
    }

    void BindPS(Graphics* pGraphics, UINT slot) {
        pGraphics->context().PSSetConstantBuffers(slot, 1u, mpConstantBuffer.GetAddressOf());

        // NOTE: The following code is too heavy to run for each model.
        // const auto logs = pGraphics->info_logger().Dump();
        // if (!logs.empty()) {
        //     nodec::logging::WarnStream(__FILE__, __LINE__)
        //         << "[ConstantBuffer::BindPS] >>> DXGI Logs:"
        //         << logs;
        // }
    }

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> mpConstantBuffer;
    UINT mSizeBytes;

private:
    NODEC_DISABLE_COPY(ConstantBuffer)
};

