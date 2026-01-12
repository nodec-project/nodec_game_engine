#ifndef NODEC_GAME_EDITOR__COMPONENTS__GIZMO_WIRE_HPP_
#define NODEC_GAME_EDITOR__COMPONENTS__GIZMO_WIRE_HPP_

#include <memory>

#include <nodec/vector4.hpp>
#include <nodec_rendering/resources/mesh.hpp>

namespace components {

struct GizmoWire {
    std::shared_ptr<nodec_rendering::resources::Mesh> mesh;
    nodec::Vector4f color{0.0f, 1.0f, 0.0f, 0.5f};
};

}

#endif