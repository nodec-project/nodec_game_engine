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

#include "rigid_body_motion_state.hpp"

class RigidBodyBackend final {
public:
    enum class BodyType {
        Static,
        Dynamic,
        Kinematic,
    };

    RigidBodyBackend(nodec_scene::SceneEntity entity,
                     BodyType body_type,
                     float mass,
                     const nodec_physics::components::PhysicsShape &shape,
                     const nodec::Vector3f &start_position,
                     const nodec::Quaternionf &start_rotation,
                     const nodec::Vector3f &world_shape_scale)
        : entity_(entity), body_type_(body_type) {
        using namespace nodec_physics::components;

        switch (shape.shape_type) {
        case PhysicsShape::ShapeType::Box: {
            // Make the unit box shape. Then set the size using SetLocalScaling().
            collision_shape_.reset(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)));
            const auto size = world_shape_scale * shape.size;
            collision_shape_->setLocalScaling(btVector3(size.x, size.y, size.z));
        } break;

        case PhysicsShape::ShapeType::Sphere: {
            collision_shape_.reset(new btSphereShape(1.f));
            const auto radius = std::max({world_shape_scale.x, world_shape_scale.y, world_shape_scale.z}) * shape.radius;
            collision_shape_->setLocalScaling(btVector3(radius, radius, radius));
        } break;

        case PhysicsShape::ShapeType::Capsule: {
            const auto scale = std::max({world_shape_scale.x, world_shape_scale.y, world_shape_scale.z});
            collision_shape_.reset(new btCapsuleShape(scale * shape.radius, scale * shape.height));
        } break;

        // Nothing to do here.
        default: break;
        }

        if (body_type_ == BodyType::Static) {
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

        native_->setUserPointer(this);

        if (body_type_ == BodyType::Kinematic) {
            native_->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            // KinematicRigidBody is always active. StaticRigidBody will never be active even if set as below.
            native_->setActivationState(DISABLE_DEACTIVATION);
        }
    }

    ~RigidBodyBackend() {
        if (dynamic_world_) {
            dynamic_world_->removeRigidBody(native_.get());
        }
    }

    nodec_scene::SceneEntity entity() const noexcept {
        return entity_;
    }

    BodyType body_type() const noexcept {
        return body_type_;
    }

    btRigidBody &native() {
        return *native_;
    }

    const btRigidBody &native() const {
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
    nodec_scene::SceneEntity entity_;
    BodyType body_type_;
    std::unique_ptr<RigidBodyMotionState> motion_state_;
    std::unique_ptr<btCollisionShape> collision_shape_;
    std::unique_ptr<btRigidBody> native_;
    btDynamicsWorld *dynamic_world_{nullptr};
};

#endif