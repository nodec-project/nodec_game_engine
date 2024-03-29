#pragma once

#include "graphics.hpp"

class ConstantBuffer {
public:
    ConstantBuffer(Graphics &gfx, UINT size_byte, const void *sys_mem_ptr)
        : gfx_(gfx), size_byte_(size_byte) {
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
            gfx_.device().CreateBuffer(&cbd, &csd, &constant_buffer_),
            &gfx_, __FILE__, __LINE__);
    }

    void update(const void *sys_mem_ptr) {
        // NOTE: This function should be designed to be lightweight.
        //  This function will be called frequently during each frame rendering.
        D3D11_MAPPED_SUBRESOURCE msr;
        gfx_.context().Map(
            constant_buffer_.Get(), 0u,
            D3D11_MAP_WRITE_DISCARD, 0u,
            &msr);
        memcpy(msr.pData, sys_mem_ptr, size_byte_);
        gfx_.context().Unmap(constant_buffer_.Get(), 0u);
    }

    void bind(UINT slot) {
        gfx_.context().VSSetConstantBuffers(slot, 1u, constant_buffer_.GetAddressOf());
        gfx_.context().PSSetConstantBuffers(slot, 1u, constant_buffer_.GetAddressOf());
    }

    void bind_vs(UINT slot) {
        // NOTE: This function should be designed to be lightweight.
        //  This function will be called frequently during each frame rendering.
        gfx_.context().VSSetConstantBuffers(slot, 1u, constant_buffer_.GetAddressOf());
    }

    void bind_ps(UINT slot) {
        // NOTE: This function should be designed to be lightweight.
        //  This function will be called frequently during each frame rendering.
        gfx_.context().PSSetConstantBuffers(slot, 1u, constant_buffer_.GetAddressOf());
    }

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer_;
    UINT size_byte_;

private:
    NODEC_DISABLE_COPY(ConstantBuffer)
};
