#ifndef NODEC_GAME_ENGINE__RENDERING__MESH_RENDERER_BACKEND_HPP_
#define NODEC_GAME_ENGINE__RENDERING__MESH_RENDERER_BACKEND_HPP_

#include <DirectXMath.h>

#include "scene_renderer_context.hpp"

class DrawCommand {
public:
    virtual ~DrawCommand() {}
    virtual void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
                      SceneRendererContext &, Graphics &) = 0;
};

#endif