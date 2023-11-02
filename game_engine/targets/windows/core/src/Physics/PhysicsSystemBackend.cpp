#include <Physics/PhysicsSystemBackend.hpp>
#include <Physics/RigidBodyActivity.hpp>
#include <Physics/RigidBodyBackend.hpp>

#include <nodec_bullet3_compat/nodec_bullet3_compat.hpp>
#include <nodec_physics/components/central_force.hpp>
#include <nodec_physics/components/impluse_force.hpp>
#include <nodec_physics/components/physics_shape.hpp>
#include <nodec_physics/components/rigid_body.hpp>
#include <nodec_physics/components/velocity_force.hpp>
#include <nodec_scene/components/local_to_world.hpp>

#include <nodec/math/gfx.hpp>

bool operator==(const btVector3 &left, const nodec::Vector3f &right) {
    return left.x() == right.x && left.y() == right.y && left.z() == right.z;
}

bool operator!=(const btVector3 &left, const nodec::Vector3f &right) {
    return !(left == right);
}

void PhysicsSystemBackend::on_stepped(nodec_world::World &world) {
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec_physics::components;
    using namespace nodec_physics;

    auto &scene_registry = world.scene().registry();

    // Pre process for step of simulation.
    scene_registry.view<RigidBody, PhysicsShape, LocalToWorld>().each(
        [&](SceneEntity entt, RigidBody &rigid_body, PhysicsShape &shape, LocalToWorld &local_to_world) {
            Vector3f world_position;
            Quaternionf world_rotation;
            Vector3f world_scale;

            math::gfx::decompose_trs(local_to_world.value, world_position, world_rotation, world_scale);

            RigidBodyActivity *activity;
            {
                // Emplace an activity component, if not.
                auto result = scene_registry.emplace_component<RigidBodyActivity>(entt);
                activity = &result.first;

                if (result.second) {
                    // When the activity is first created.

                    auto rigid_body_backend = std::make_unique<RigidBodyBackend>(entt, rigid_body.mass, shape,
                                                                                 world_position, world_rotation, world_scale);

                    rigid_body_backend->bind_world(*dynamics_world_);

                    btVector3 linear_factor(
                        (rigid_body.constraints & RigidBodyConstraints::FreezePositionX) ? 0.f : 1.f,
                        (rigid_body.constraints & RigidBodyConstraints::FreezePositionY) ? 0.f : 1.f,
                        (rigid_body.constraints & RigidBodyConstraints::FreezePositionZ) ? 0.f : 1.f);

                    rigid_body_backend->native().setLinearFactor(linear_factor);

                    btVector3 angular_factor(
                        (rigid_body.constraints & RigidBodyConstraints::FreezeRotationX) ? 0.f : 1.f,
                        (rigid_body.constraints & RigidBodyConstraints::FreezeRotationY) ? 0.f : 1.f,
                        (rigid_body.constraints & RigidBodyConstraints::FreezeRotationZ) ? 0.f : 1.f);
                    rigid_body_backend->native().setAngularFactor(angular_factor);

                    activity->rigid_body_backend = std::move(rigid_body_backend);
                }
            }

            {
                btTransform rb_trfm;
                activity->rigid_body_backend->native().getMotionState()->getWorldTransform(rb_trfm);

                auto rb_position = to_vector3(rb_trfm.getOrigin());
                auto rb_rotation = to_quaternion(rb_trfm.getRotation());

                // Sync entity -> bullet rigid body.
                if (!math::approx_equal(world_position, rb_position)
                    || !math::approx_equal_rotation(world_rotation, rb_rotation, math::default_rel_tol<float>, 0.001f)) {
                    activity->rigid_body_backend->update_transform(world_position, world_rotation);
                }
            }
        });

    {
        std::vector<SceneEntity> to_deleted;

        scene_registry.view<RigidBodyActivity>().each([&](auto entt, RigidBodyActivity &activity) {
            if (scene_registry.all_of<RigidBody, PhysicsShape>(entt)) return;

            to_deleted.push_back(entt);
        });

        scene_registry.remove_component<RigidBodyActivity>(to_deleted.begin(), to_deleted.end());
    }

    // --- Apply forces ---
    // ImpulseForce
    {
        auto view = scene_registry.view<RigidBodyActivity, ImpulseForce>();

        view.each([&](auto entt, RigidBodyActivity &activity, ImpulseForce &force) {
            activity.rigid_body_backend->native().applyCentralImpulse(to_bt_vector3(force.force));
        });

        scene_registry.remove_components<ImpulseForce>(view.begin(), view.end());
    }
    // CentralForce
    {
        auto view = scene_registry.view<RigidBodyActivity, CentralForce>();

        view.each([&](auto entt, RigidBodyActivity &activity, CentralForce &force) {
            activity.rigid_body_backend->native().applyCentralForce(to_bt_vector3(force.force));
        });

        scene_registry.remove_components<CentralForce>(view.begin(), view.end());
    }
    // VelocityForce
    {
        auto view = scene_registry.view<RigidBodyActivity, VelocityForce>();
        view.each([&](SceneEntity, RigidBodyActivity &activity, VelocityForce &force) {
            auto &native = activity.rigid_body_backend->native();
            const auto impulse = native.getMass() * force.value;
            native.applyCentralImpulse(to_bt_vector3(impulse));
        });
        scene_registry.remove_components<VelocityForce>(view.begin(), view.end());
    }
    // END Apply forces ---

    // Step simulation.
    {
        const auto delta_time = world.clock().delta_time();
        if (delta_time == 0.f) return;
        dynamics_world_->stepSimulation(delta_time, 10);
    }

    // Sync rigid body -> entity.
    scene_registry.view<RigidBody, RigidBodyActivity, LocalToWorld>().each(
        [&](SceneEntity, RigidBody &rigid_body, RigidBodyActivity &activity, LocalToWorld &local_to_world) {
            // An entity with zero mass is static object.
            if (rigid_body.mass == 0.0f) return;

            auto &native = activity.rigid_body_backend->native();

            btTransform rb_trfm;
            activity.rigid_body_backend->native().getMotionState()->getWorldTransform(rb_trfm);

            rb_trfm.getOpenGLMatrix(local_to_world.value.m);
            local_to_world.dirty = true;

            rigid_body.linear_velocity = to_vector3(native.getLinearVelocity());
            rigid_body.angular_velocity = to_vector3(native.getAngularVelocity());
        });

    // https://github.com/bulletphysics/bullet3/issues/1745
    {
        const int num_of_manifolds = dynamics_world_->getDispatcher()->getNumManifolds();

        for (int i = 0; i < num_of_manifolds; ++i) {
            auto *contact_manifold = dynamics_world_->getDispatcher()->getManifoldByIndexInternal(i);

            const int num_of_contacts = contact_manifold->getNumContacts();
            if (num_of_contacts == 0) continue;

            const auto *body0 = static_cast<const RigidBodyBackend *>(contact_manifold->getBody0()->getUserPointer());
            const auto *body1 = static_cast<const RigidBodyBackend *>(contact_manifold->getBody1()->getUserPointer());

            CollisionInfo collision_info{};

            collision_info.entity0 = body0->entity();
            collision_info.entity1 = body1->entity();

            collision_stay_signal_(collision_info);
        }
    }
}
