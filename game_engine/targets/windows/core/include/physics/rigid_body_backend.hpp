#ifndef NODEC_GAME_ENGINE__PHYSICS__RIGID_BODY_BACKEND_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__RIGID_BODY_BACKEND_HPP_

#include <algorithm>
#include <cassert>
#include <memory>

#include <nodec/math/math.hpp>
#include <nodec_bullet3_compat/nodec_bullet3_compat.hpp>
#include <nodec_physics/components/physics_shape.hpp>
#include <nodec_physics/components/rigid_body.hpp>
#include <nodec_scene/scene_registry.hpp>

#include <btBulletDynamicsCommon.h>

#include "collision_object_backend.hpp"
#include "rigid_body_motion_state.hpp"

class RigidBodyBackend final : public CollisionObjectBackend {
public:
    enum class BodyType {
        Static,
        Dynamic,
        Kinematic,
    };

public:
    RigidBodyBackend(nodec_scene::SceneEntity entity,
                     BodyType body_type,
                     float mass,
                     std::unique_ptr<btCollisionShape> collision_shape,
                     const nodec::Vector3f &start_position,
                     const nodec::Quaternionf &start_rotation)
        : CollisionObjectBackend(this, entity),
          body_type_(body_type), collision_shape_(std::move(collision_shape)) {
        if (body_type == BodyType::Static) {
            mass = 0.f;
        }

        btVector3 local_inertia(0, 0, 0);
        if (mass != 0.f) {
            collision_shape_->calculateLocalInertia(mass, local_inertia);
        }
        btTransform start_trfm;
        start_trfm.setIdentity();
        start_trfm.setOrigin(btVector3(start_position.x, start_position.y, start_position.z));
        start_trfm.setRotation(btQuaternion(start_rotation.x, start_rotation.y, start_rotation.z, start_rotation.w));

        motion_state_.reset(new RigidBodyMotionState());
        motion_state_->setWorldTransform(start_trfm);
        native_.reset(new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, motion_state_.get(), collision_shape_.get(), local_inertia)));

        if (body_type == BodyType::Kinematic) {
            native_->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            // KinematicRigidBody is always active. StaticRigidBody will never be active even if set as below.
            native_->setActivationState(DISABLE_DEACTIVATION);
        }

        native_->setUserPointer(this);
    }

    ~RigidBodyBackend() {
        if (dynamic_world_) {
            dynamic_world_->removeRigidBody(native_.get());
        }
    }

    BodyType body_type() const noexcept {
        return body_type_;
    }

    btRigidBody &native() const {
        return *native_;
    }

    btCollisionObject& native_collision_object() const override {
        return *native_;
    }

    void bind_world(btDynamicsWorld &world) {
        world.addRigidBody(native_.get());
        dynamic_world_ = &world;
    }

    void unbind_world(btDynamicsWorld &world) {
        assert(&world == dynamic_world_);
        dynamic_world_->removeRigidBody(native_.get());
    }

    void update_transform_if_different(const nodec::Vector3f &position, const nodec::Quaternionf &rotation) {
        using namespace nodec;

        btTransform rb_trfm;
        motion_state_->getWorldTransform(rb_trfm);

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

        // NOTE: We need to also set the trfm of rigid body not only motion state.
        //  Why?
        native_->setWorldTransform(rb_trfm_updated);
        motion_state_->setWorldTransform(rb_trfm_updated);

        native_->activate();
    }

private:
    BodyType body_type_;
    std::unique_ptr<RigidBodyMotionState> motion_state_;
    std::unique_ptr<btCollisionShape> collision_shape_;
    std::unique_ptr<btRigidBody> native_;
    btDynamicsWorld *dynamic_world_{nullptr};
};

#endif