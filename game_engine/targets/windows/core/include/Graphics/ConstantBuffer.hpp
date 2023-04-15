#pragma once

#include "Graphics.hpp"

class ConstantBuffer {
public:
    ConstantBuffer(Graphics *pGraphics, UINT size_byte, const void *sys_mem_ptr)
        : size_byte_(size_byte) {
        D3D11_BUFFER_DESC cbd;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbd.MiscFlags = 0u;
        cbd.ByteWidth = size_byte;
        cbd.StructureByteStride = 0u;

        D3D11_SUBRESOURCE_DATA csd = {};
        csd.pSysMem = sys_mem_ptr;

        ThrowIfFailedGfx(
            pGraphics->device().CreateBuffer(&cbd, &csd, &constant_buffer_),
            pGraphics, __FILE__, __LINE__);
    }

    void update(Graphics *gfx, const void *sys_mem_ptr) {
        // NOTE: This function should be designed to be lightweight.
        //  This function will be called frequently during each frame rendering.
        D3D11_MAPPED_SUBRESOURCE msr;
        gfx->context().Map(
            constant_buffer_.Get(), 0u,
            D3D11_MAP_WRITE_DISCARD, 0u,
            &msr);
        memcpy(msr.pData, sys_mem_ptr, size_byte_);
        gfx->context().Unmap(constant_buffer_.Get(), 0u);
    }

    void bind_vs(Graphics *gfx, UINT slot) {
        // NOTE: This function should be designed to be lightweight.
        //  This function will be called frequently during each frame rendering.
        gfx->context().VSSetConstantBuffers(slot, 1u, constant_buffer_.GetAddressOf());
    }

    void bind_ps(Graphics *gfx, UINT slot) {
        // NOTE: This function should be designed to be lightweight.
        //  This function will be called frequently during each frame rendering.
        gfx->context().PSSetConstantBuffers(slot, 1u, constant_buffer_.GetAddressOf());
    }

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer_;
    UINT size_byte_;

private:
    NODEC_DISABLE_COPY(ConstantBuffer)
};
