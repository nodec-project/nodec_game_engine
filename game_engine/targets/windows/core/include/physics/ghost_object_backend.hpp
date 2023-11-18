#ifndef NODEC_GAME_ENGINE__PHYSICS__GHOST_OBJECT_BACKEND_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__GHOST_OBJECT_BACKEND_HPP_

#include <memory>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "collision_object_backend.hpp"

class GhostObjectBackend final : public CollisionObjectBackend {
public:
    GhostObjectBackend(nodec_scene::SceneEntity entity,
                       std::unique_ptr<btCollisionShape> collision_shape,
                       const nodec::Vector3f &start_position,
                       const nodec::Quaternionf &start_rotation)
        : CollisionObjectBackend(this, entity),
          collision_shape_(std::move(collision_shape)) {
        btTransform start_trfm;
        start_trfm.setIdentity();
        start_trfm.setOrigin(btVector3(start_position.x, start_position.y, start_position.z));
        start_trfm.setRotation(btQuaternion(start_rotation.x, start_rotation.y, start_rotation.z, start_rotation.w));

        native_.reset(new btGhostObject());
        native_->setCollisionShape(collision_shape_.get());
        native_->setWorldTransform(start_trfm);
    }

    ~GhostObjectBackend() override {
        if (world_) {
            world_->removeCollisionObject(native_.get());
        }
    }

    btCollisionObject &native_collision_object() const override {
        return *native_;
    }

    btGhostObject &native() const {
        return *native_;
    }

    void bind_world(btDynamicsWorld &world) {
        world.addCollisionObject(native_.get());
        world_ = &world;
    }

    void unbind_world(btDynamicsWorld &world) {
        assert(world_ == &world);
        world_->removeCollisionObject(native_.get());
        world_ = nullptr;
    }

private:
    std::unique_ptr<btCollisionShape> collision_shape_;
    std::unique_ptr<btGhostObject> native_;
    btDynamicsWorld *world_{nullptr};
};

#endif