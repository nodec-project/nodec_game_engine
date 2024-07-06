#ifndef NODEC_GAME_ENGINE__GRAPHICS__BLEND_STATE_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__BLEND_STATE_HPP_

#include "graphics.hpp"

class BlendState {
public:
    static BlendState CreateDefaultBlend(Graphics &gfx) {
        return {gfx, CD3D11_BLEND_DESC{CD3D11_DEFAULT{}}};
    }

    static BlendState CreateAlphaBlend(Graphics &gfx) {
        D3D11_RENDER_TARGET_BLEND_DESC alpha_blend_desc = {};
        alpha_blend_desc.BlendEnable = true;
        alpha_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
        alpha_blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        alpha_blend_desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        alpha_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_MIN;
        alpha_blend_desc.SrcBlendAlpha = D3D11_BLEND_ZERO;
        alpha_blend_desc.DestBlendAlpha = D3D11_BLEND_ONE;
        alpha_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        D3D11_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = TRUE;
        desc.RenderTarget[0] = alpha_blend_desc;

        const D3D11_RENDER_TARGET_BLEND_DESC default_blend_desc = {
            FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
            D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
            D3D11_COLOR_WRITE_ENABLE_ALL};

        for (UINT i = 1; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            desc.RenderTarget[i] = default_blend_desc;
        }

        return {gfx, desc};
    }

    BlendState(Graphics &gfx, const D3D11_BLEND_DESC &desc)
        : gfx_(gfx) {
        ThrowIfFailedGfx(
            gfx_.device().CreateBlendState(&desc, &blend_state_),
            &gfx_, __FILE__, __LINE__);
    }

    void bind() {
        gfx_.context().OMSetBlendState(blend_state_.Get(), nullptr, 0xFFFFFFFFu);
    }

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state_;
};

#endif