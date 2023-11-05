#ifndef NODEC_GAME_ENGINE__PHYSICS__RIGID_BODY_ACTIVITY_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__RIGID_BODY_ACTIVITY_HPP_

#include <memory>

#include "rigid_body_backend.hpp"

struct RigidBodyActivity {
    std::unique_ptr<RigidBodyBackend> rigid_body_backend;
};

#endif