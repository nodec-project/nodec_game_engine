#pragma once

#include "graphics.hpp"

class IndexBuffer
{
public:
    IndexBuffer(Graphics* pGraphics, UINT size, const void* pSysMem) {
        D3D11_BUFFER_DESC ibd = {};
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.CPUAccessFlags = 0u;
        ibd.MiscFlags = 0u;
        ibd.ByteWidth = size * sizeof(uint16_t);
        ibd.StructureByteStride = sizeof(uint16_t);
        D3D11_SUBRESOURCE_DATA isd = {};
        isd.pSysMem = pSysMem;

        ThrowIfFailedGfx(
            pGraphics->device().CreateBuffer(&ibd, &isd, &pIndexBuffer),
            pGraphics, __FILE__, __LINE__
        );
    }

    void Bind(Graphics* pGraphics) {
        pGraphics->context().IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);

        // NOTE: The following code is too heavy to run for each model.
        // const auto logs = pGraphics->info_logger().Dump();
        // if (!logs.empty()) {
        //     nodec::logging::WarnStream(__FILE__, __LINE__)
        //         << "[IndexBuffer::Bind] >>> DXGI Logs:"
        //         << logs;
        // }
    }

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;

private:
    NODEC_DISABLE_COPY(IndexBuffer)
};