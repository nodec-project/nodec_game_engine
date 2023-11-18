#ifndef NODEC_GAME_ENGINE__PHYSICS__COLLISION_OBJECT_ACTIVITY_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__COLLISION_OBJECT_ACTIVITY_HPP_

#include <memory>

#include "collision_object_backend.hpp"

struct CollisionObjectActivity {
    std::unique_ptr<CollisionObjectBackend> collision_object_backend;
};

#endif