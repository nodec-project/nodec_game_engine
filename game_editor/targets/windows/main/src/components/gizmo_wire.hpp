#ifndef NODEC_GAME_EDITOR__COMPONENTS__GIZMO_WIRE_HPP_
#define NODEC_GAME_EDITOR__COMPONENTS__GIZMO_WIRE_HPP_

#include <memory>

#include <nodec_rendering/resources/mesh.hpp>

namespace components {

struct GizmoWire {
    std::shared_ptr<nodec_rendering::resources::Mesh> mesh;
};

}

#endif