#ifndef NODEC_GAME_ENGINE__PHYSICS__COLLISION_OBJECT_BACKEND_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__COLLISION_OBJECT_BACKEND_HPP_

#include <btBulletDynamicsCommon.h>

#include <nodec/type_info.hpp>
#include <nodec_scene/scene_entity.hpp>

class CollisionObjectBackend {
public:
    template<typename T>
    CollisionObjectBackend(T *, nodec_scene::SceneEntity entity)
        : type_info_(nodec::type_id<T>()), entity_(entity) {
    }

    virtual ~CollisionObjectBackend() = default;

    nodec_scene::SceneEntity entity() const noexcept {
        return entity_;
    }

    const nodec::type_info &type_info() const noexcept {
        return type_info_;
    }

    virtual btCollisionObject &native_collision_object() const = 0;

private:
    const nodec::type_info &type_info_;
    nodec_scene::SceneEntity entity_;
};

template<typename T>
T *collision_object_cast(CollisionObjectBackend *backend) {
    if (backend->type_info() != nodec::type_id<T>()) {
        return nullptr;
    }

    return static_cast<T *>(backend);
}

#endif