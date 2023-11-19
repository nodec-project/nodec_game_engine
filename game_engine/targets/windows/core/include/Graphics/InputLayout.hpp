#ifndef NODEC_GAME_ENGINE__GRAPHICS__INPUT_LAYOUT_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__INPUT_LAYOUT_HPP_

#include "Graphics.hpp"

class InputLayout {
public:
    InputLayout(Graphics &gfx,
                const D3D11_INPUT_ELEMENT_DESC *input_element_descs,
                UINT num_of_elements,
                const void *shader_bytecode,
                SIZE_T shader_bytecode_size)
        : gfx_(gfx) {
        ThrowIfFailedGfx(
            gfx.device().CreateInputLayout(
                input_element_descs,
                num_of_elements,
                shader_bytecode,
                shader_bytecode_size,
                &input_layout_),
            &gfx, __FILE__, __LINE__);
    }

    void bind() {
        gfx_.context().IASetInputLayout(input_layout_.Get());
    }

private:
    Graphics &gfx_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_;
};

#endif