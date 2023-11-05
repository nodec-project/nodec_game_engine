#include <physics/physics_system_backend.hpp>

#include <nodec/math/gfx.hpp>

#include <nodec_bullet3_compat/nodec_bullet3_compat.hpp>
#include <nodec_physics/components/central_force.hpp>
#include <nodec_physics/components/impluse_force.hpp>
#include <nodec_physics/components/physics_shape.hpp>
#include <nodec_physics/components/rigid_body.hpp>
#include <nodec_physics/components/static_rigid_body.hpp>
#include <nodec_physics/components/velocity_force.hpp>
#include <nodec_scene/components/local_to_world.hpp>

#include <physics/rigid_body_activity.hpp>
#include <physics/rigid_body_backend.hpp>

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

    // Sync entity -> bullet rigid body.
    scene_registry.view<RigidBodyActivity, LocalToWorld>().each(
        [&](const SceneEntity &entity, RigidBodyActivity &activity, LocalToWorld &local_to_world) {
            using namespace nodec;
            assert(activity.rigid_body_backend);

            math::gfx::TRSComponents world_trs;
            math::gfx::decompose_trs(local_to_world.value, world_trs);
            activity.rigid_body_backend->update_transform_if_different(world_trs.translation, world_trs.rotation);
        });

    scene_registry.view<StaticRigidBody, PhysicsShape, LocalToWorld>(type_list<RigidBodyActivity>{})
        .each([&](const SceneEntity &entity, StaticRigidBody &, PhysicsShape &shape, LocalToWorld &local_to_world) {
            auto &activity = scene_registry.emplace_component<RigidBodyActivity>(entity).first;

            math::gfx::TRSComponents world_trs;
            math::gfx::decompose_trs(local_to_world.value, world_trs);

            auto rigid_body_backend = std::make_unique<RigidBodyBackend>(entity, RigidBodyBackend::BodyType::Static, 0.f, shape,
                                                                         world_trs.translation, world_trs.rotation, world_trs.scale);

            rigid_body_backend->bind_world(*dynamics_world_);
            activity.rigid_body_backend = std::move(rigid_body_backend);
        });

    // Make RigidBodyActivity for new rigid bodies.
    scene_registry.view<RigidBody, PhysicsShape, LocalToWorld>(type_list<RigidBodyActivity>{})
        .each([&](SceneEntity entt, RigidBody &rigid_body, PhysicsShape &shape, LocalToWorld &local_to_world) {
            auto &activity = scene_registry.emplace_component<RigidBodyActivity>(entt).first;

            math::gfx::TRSComponents world_trs;
            math::gfx::decompose_trs(local_to_world.value, world_trs);

            RigidBodyBackend::BodyType body_type{RigidBodyBackend::BodyType::Dynamic};
            switch (rigid_body.body_type) {
            case RigidBody::BodyType::Dynamic:
                body_type = RigidBodyBackend::BodyType::Dynamic;
                break;
            case RigidBody::BodyType::Kinematic:
                body_type = RigidBodyBackend::BodyType::Kinematic;
                break;
            }

            auto rigid_body_backend = std::make_unique<RigidBodyBackend>(entt, body_type, rigid_body.mass, shape,
                                                                         world_trs.translation, world_trs.rotation, world_trs.scale);

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

            activity.rigid_body_backend = std::move(rigid_body_backend);
        });

    {
        auto view = scene_registry.view<RigidBodyActivity>(type_list<RigidBody, StaticRigidBody>{});
        scene_registry.remove_components<RigidBodyActivity>(view.begin(), view.end());
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
            if (activity.rigid_body_backend->body_type() != RigidBodyBackend::BodyType::Dynamic) return;

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
