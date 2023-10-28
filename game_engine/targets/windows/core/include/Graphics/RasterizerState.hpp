#ifndef NODEC_GAME_ENGINE__GRAPHICS__RASTERIZER_STATE_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__RASTERIZER_STATE_HPP_

#include "Graphics.hpp"

class RasterizerState {
public:
    RasterizerState(Graphics &gfx, D3D11_CULL_MODE cull_mode, D3D11_FILL_MODE fill_mode = D3D11_FILL_SOLID)
        : gfx_(gfx) {
        D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
        desc.CullMode = cull_mode;
        desc.FillMode = fill_mode;

        ThrowIfFailedGfx(gfx.device().CreateRasterizerState(&desc, &rasterizer_state_),
                         &gfx, __FILE__, __LINE__);
    }

    void bind() {
        gfx_.context().RSSetState(rasterizer_state_.Get());
    }

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_state_;
};

#endif