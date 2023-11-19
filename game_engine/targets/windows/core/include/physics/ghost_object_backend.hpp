#ifndef NODEC_GAME_ENGINE__PHYSICS__GHOST_OBJECT_BACKEND_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__GHOST_OBJECT_BACKEND_HPP_

#include <memory>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>

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
        native_->setCollisionFlags(native_->getCollisionFlags()
                                   | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        native_->setUserPointer(this);
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

    void update_transform_if_different(const nodec::Vector3f &position, const nodec::Quaternionf &rotation) {
        using namespace nodec;

        auto rb_trfm = native_->getWorldTransform();

        auto rb_position = to_vector3(rb_trfm.getOrigin());
        auto rb_rotation = to_quaternion(rb_trfm.getRotation());

        if (math::approx_equal(position, rb_position)
            && math::approx_equal_rotation(rotation, rb_rotation, math::default_rel_tol<float>, 0.001f)) {
            return;
        }

        btTransform rb_trfm_updated;
        rb_trfm_updated.setIdentity();
        rb_trfm_updated.setOrigin(to_bt_vector3(position));
        rb_trfm_updated.setRotation(to_bt_quaternion(rotation));
        native_->setWorldTransform(rb_trfm_updated);
    }

private:
    std::unique_ptr<btCollisionShape> collision_shape_;
    std::unique_ptr<btGhostObject> native_;
    btDynamicsWorld *world_{nullptr};
};

#endif