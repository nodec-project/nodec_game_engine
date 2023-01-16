#include <Physics/PhysicsSystemBackend.hpp>
#include <Physics/RigidBodyActivity.hpp>
#include <Physics/RigidBodyBackend.hpp>

#include <nodec_bullet3_compat/nodec_bullet3_compat.hpp>
#include <nodec_physics/components/impluse_force.hpp>
#include <nodec_physics/components/physics_shape.hpp>
#include <nodec_physics/components/rigid_body.hpp>

#include <nodec/logging.hpp>
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

    // Pre process for step of simulation.
    world.scene().registry().view<RigidBody, PhysicsShape, Transform>().each(
        [&](auto entt, RigidBody &rigid_body, PhysicsShape &shape, Transform &trfm) {
            Vector3f world_position;
            Quaternionf world_rotation;
            Vector3f world_scale;

            math::gfx::decompose_trs(trfm.local2world, world_position, world_rotation, world_scale);

            RigidBodyActivity *activity;
            {
                // Emplace an activity component, if not.
                auto result = world.scene().registry().emplace_component<RigidBodyActivity>(entt);
                activity = &result.first;

                if (result.second) {
                    // When the activity is first created.

                    auto rigid_body_backend = std::make_unique<RigidBodyBackend>(rigid_body.mass, shape, world_scale);
                    rigid_body_backend->update_transform(world_position, world_rotation);
                    activity->prev_world_position = world_position;
                    activity->prev_world_rotation = world_rotation;

                    rigid_body_backend->bind_world(*dynamics_world_);
                    activity->rigid_body_backend = std::move(rigid_body_backend);
                }
            }

            // Sync entity -> bullet rigid body.
            activity->rigid_body_backend->update_transform(world_position, world_rotation);

            activity->prev_world_position = world_position;
            activity->prev_world_rotation = world_rotation;
        });

    {
        std::vector<SceneEntity> to_deleted;

        world.scene().registry().view<RigidBodyActivity>().each([&](auto entt, RigidBodyActivity &activity) {
            if (world.scene().registry().all_of<RigidBody, PhysicsShape>(entt)) {
                return;
            }

            to_deleted.push_back(entt);
        });

        for (auto &entt : to_deleted) {
            world.scene().registry().remove_component<RigidBodyActivity>(entt);
        }
    }

    // Apply force.
    {
        std::vector<SceneEntity> to_deletes;
        world.scene().registry().view<RigidBodyActivity, ImpulseForce>().each([&](auto entt, RigidBodyActivity &activity, ImpulseForce &force) {
            activity.rigid_body_backend->native().applyCentralImpulse(to_bt_vector3(force.force));
            to_deletes.emplace_back(entt);
        });

        for (auto &entt : to_deletes) {
            world.scene().registry().remove_component<ImpulseForce>(entt);
        }
    }

    // Step simulation.
    {
        const auto delta_time = world.clock().delta_time();
        if (delta_time == 0.f) return;
        dynamics_world_->stepSimulation(delta_time, 10);
    }

    // Sync rigid body -> entity.
    world.scene().registry().view<RigidBody, RigidBodyActivity, Transform>().each(
        [&](auto entt, RigidBody &rigid_body, RigidBodyActivity &activity, Transform &trfm) {
            if (rigid_body.mass == 0.0f) return;

            btTransform rb_trfm;
            activity.rigid_body_backend->native().getMotionState()->getWorldTransform(rb_trfm);

            const auto world_position = static_cast<Vector3f>(to_vector3(rb_trfm.getOrigin()));
            const auto world_rotation = static_cast<Quaternionf>(to_quaternion(rb_trfm.getRotation()));

            // activity.prev_world_position = world_position;
            // activity.prev_world_rotation = world_rotation;

            auto delta_position = world_position - activity.prev_world_position;
            auto delta_rotation = math::inv(activity.prev_world_rotation) * world_rotation;

            trfm.local_position += delta_position;
            trfm.local_rotation = delta_rotation * trfm.local_rotation;

            trfm.dirty = true;
        });

    // https://github.com/bulletphysics/bullet3/issues/1745
    {
        const int num_of_manifolds = dynamics_world_->getDispatcher()->getNumManifolds();

        for (int i = 0; i < num_of_manifolds; ++i) {
            auto *contact_manifold = dynamics_world_->getDispatcher()->getManifoldByIndexInternal(i);

            const int num_of_contacts = contact_manifold->getNumContacts();
            if (num_of_contacts == 0) continue;

            CollisionInfo collision_info;
            collision_info.num_of_contacts = num_of_contacts;

            for (int j = 0; j < num_of_contacts; ++j) {
                const auto &point = contact_manifold->getContactPoint(j);
            }

            collision_signal_(collision_info);
        }
    }
}
