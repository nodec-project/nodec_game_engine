#ifndef NODEC_GAME_ENGINE__GRAPHICS__VERTEX_BUFFER_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__VERTEX_BUFFER_HPP_

#include "graphics.hpp"

/**
 * vertices in device memory
 */
class VertexBuffer {
public:
    VertexBuffer(Graphics *pGraphics, UINT sizeBytes, UINT strideBytes, const void *pSysMem)
        : mSizeBytes(sizeBytes), mStrideBytes(strideBytes) {
        D3D11_BUFFER_DESC bd = {};
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.CPUAccessFlags = 0u;
        bd.MiscFlags = 0u;
        bd.ByteWidth = sizeBytes;
        bd.StructureByteStride = strideBytes;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = pSysMem;
        ThrowIfFailedGfx(
            pGraphics->device().CreateBuffer(&bd, &sd, &pVertexBuffer),
            pGraphics, __FILE__, __LINE__);
    }

    void Bind(Graphics *pGraphics) {
        const UINT offset = 0u;

        pGraphics->context().IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &mStrideBytes, &offset);

        // NOTE: The following code is too heavy to run for each model.
        // const auto logs = pGraphics->info_logger().Dump();
        // if (!logs.empty()) {
        //     nodec::logging::WarnStream(__FILE__, __LINE__)
        //         << "[VertexShader::Bind] >>> DXGI Logs:"
        //         << logs;
        // }
    }

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
    UINT mSizeBytes;
    UINT mStrideBytes;

private:
    NODEC_DISABLE_COPY(VertexBuffer)
};

#endif