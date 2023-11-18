#include <physics/physics_system_backend.hpp>

#include <nodec/math/gfx.hpp>

#include <nodec_bullet3_compat/nodec_bullet3_compat.hpp>
#include <nodec_physics/components/central_force.hpp>
#include <nodec_physics/components/impluse_force.hpp>
#include <nodec_physics/components/physics_shape.hpp>
#include <nodec_physics/components/rigid_body.hpp>
#include <nodec_physics/components/static_rigid_body.hpp>
#include <nodec_physics/components/trigger_body.hpp>
#include <nodec_physics/components/velocity_force.hpp>
#include <nodec_scene/components/local_to_world.hpp>

#include <physics/collision_object_activity.hpp>
#include <physics/ghost_object_backend.hpp>
#include <physics/rigid_body_backend.hpp>

namespace {

std::unique_ptr<btCollisionShape>
make_collision_shape(nodec_physics::components::PhysicsShape &shape,
                     const nodec::Vector3f &world_shape_scale) {
    using namespace nodec_physics::components;

    std::unique_ptr<btCollisionShape> collision_shape;

    switch (shape.shape_type) {
    case PhysicsShape::ShapeType::Box: {
        // Make the unit box shape. Then set the size using SetLocalScaling().
        collision_shape.reset(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)));
        const auto size = world_shape_scale * shape.size;
        collision_shape->setLocalScaling(btVector3(size.x, size.y, size.z));
    } break;

    case PhysicsShape::ShapeType::Sphere: {
        collision_shape.reset(new btSphereShape(1.f));
        const auto radius = std::max({world_shape_scale.x, world_shape_scale.y, world_shape_scale.z}) * shape.radius;
        collision_shape->setLocalScaling(btVector3(radius, radius, radius));
    } break;

    case PhysicsShape::ShapeType::Capsule: {
        const auto scale = std::max({world_shape_scale.x, world_shape_scale.y, world_shape_scale.z});
        collision_shape.reset(new btCapsuleShape(scale * shape.radius, scale * shape.height));
    } break;

    // Nothing to do here.
    default: break;
    }
    return collision_shape;
}
} // namespace

void PhysicsSystemBackend::on_stepped(nodec_world::World &world) {
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec_physics::components;
    using namespace nodec_physics;

    auto &scene_registry = world.scene().registry();

    // Sync entity -> bullet rigid body.
    scene_registry.view<RigidBody, CollisionObjectActivity, LocalToWorld>().each(
        [&](SceneEntity entity, RigidBody &, CollisionObjectActivity &activity, LocalToWorld &local_to_world) {
            auto rigid_body_backend = collision_object_cast<RigidBodyBackend>(activity.collision_object_backend.get());
            assert(rigid_body_backend);
            using namespace nodec;

            math::gfx::TRSComponents world_trs;
            math::gfx::decompose_trs(local_to_world.value, world_trs);
            rigid_body_backend->update_transform_if_different(world_trs.translation, world_trs.rotation);
        });

    scene_registry.view<TriggerBody, PhysicsShape, LocalToWorld>(type_list<CollisionObjectActivity>{})
        .each([&](SceneEntity entity, TriggerBody &, PhysicsShape &shape, LocalToWorld &local_to_world) {
            auto &activity = scene_registry.emplace_component<CollisionObjectActivity>(entity).first;

            math::gfx::TRSComponents world_trs;
            math::gfx::decompose_trs(local_to_world.value, world_trs);

            auto shape_backend = make_collision_shape(shape, world_trs.scale);

            auto ghost_body_backend = std::make_unique<GhostObjectBackend>(entity, std::move(shape_backend), world_trs.translation, world_trs.rotation);

            ghost_body_backend->bind_world(*dynamics_world_);
            activity.collision_object_backend = std::move(ghost_body_backend);
        });

    scene_registry.view<StaticRigidBody, PhysicsShape, LocalToWorld>(type_list<CollisionObjectActivity>{})
        .each([&](SceneEntity entity, StaticRigidBody &, PhysicsShape &shape, LocalToWorld &local_to_world) {
            auto &activity = scene_registry.emplace_component<CollisionObjectActivity>(entity).first;

            math::gfx::TRSComponents world_trs;
            math::gfx::decompose_trs(local_to_world.value, world_trs);

            auto shape_backend = make_collision_shape(shape, world_trs.scale);

            auto rigid_body_backend = std::make_unique<RigidBodyBackend>(entity, RigidBodyBackend::BodyType::Static, 0.f, std::move(shape_backend),
                                                                         world_trs.translation, world_trs.rotation);

            rigid_body_backend->bind_world(*dynamics_world_);
            activity.collision_object_backend = std::move(rigid_body_backend);
        });

    // Make RigidBodyActivity for new rigid bodies.
    scene_registry.view<RigidBody, PhysicsShape, LocalToWorld>(type_list<CollisionObjectActivity>{})
        .each([&](SceneEntity entt, RigidBody &rigid_body, PhysicsShape &shape, LocalToWorld &local_to_world) {
            auto &activity = scene_registry.emplace_component<CollisionObjectActivity>(entt).first;

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

            auto shape_backend = make_collision_shape(shape, world_trs.scale);

            auto rigid_body_backend = std::make_unique<RigidBodyBackend>(entt, body_type, rigid_body.mass, std::move(shape_backend),
                                                                         world_trs.translation, world_trs.rotation);

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

            activity.collision_object_backend = std::move(rigid_body_backend);
        });

    {
        auto view = scene_registry.view<CollisionObjectActivity>(type_list<RigidBody, StaticRigidBody>{});
        scene_registry.remove_components<CollisionObjectActivity>(view.begin(), view.end());
    }

    // --- Apply forces ---
    // ImpulseForce
    {
        auto view = scene_registry.view<RigidBody, CollisionObjectActivity, ImpulseForce>();

        view.each([&](SceneEntity, RigidBody &, CollisionObjectActivity &activity, ImpulseForce &force) {
            auto rigid_body_backend = collision_object_cast<RigidBodyBackend>(activity.collision_object_backend.get());
            assert(rigid_body_backend);
            rigid_body_backend->native().applyCentralImpulse(to_bt_vector3(force.force));
        });

        scene_registry.remove_components<ImpulseForce>(view.begin(), view.end());
    }
    // CentralForce
    {
        auto view = scene_registry.view<RigidBody, CollisionObjectActivity, CentralForce>();

        view.each([&](SceneEntity, RigidBody &, CollisionObjectActivity &activity, CentralForce &force) {
            auto rigid_body_backend = collision_object_cast<RigidBodyBackend>(activity.collision_object_backend.get());
            assert(rigid_body_backend);
            rigid_body_backend->native().applyCentralForce(to_bt_vector3(force.force));
        });

        scene_registry.remove_components<CentralForce>(view.begin(), view.end());
    }
    // VelocityForce
    {
        auto view = scene_registry.view<RigidBody, CollisionObjectActivity, VelocityForce>();
        view.each([&](SceneEntity, RigidBody &, CollisionObjectActivity &activity, VelocityForce &force) {
            auto rigid_body_backend = collision_object_cast<RigidBodyBackend>(activity.collision_object_backend.get());
            assert(rigid_body_backend);
            auto &native = rigid_body_backend->native();
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
    scene_registry.view<RigidBody, CollisionObjectActivity, LocalToWorld>().each(
        [&](SceneEntity, RigidBody &rigid_body, CollisionObjectActivity &activity, LocalToWorld &local_to_world) {
            auto rigid_body_backend = collision_object_cast<RigidBodyBackend>(activity.collision_object_backend.get());
            assert(rigid_body_backend);

            if (rigid_body_backend->body_type() != RigidBodyBackend::BodyType::Dynamic) return;

            auto &native = rigid_body_backend->native();

            btTransform rb_trfm;
            rigid_body_backend->native().getMotionState()->getWorldTransform(rb_trfm);

            rb_trfm.getOpenGLMatrix(local_to_world.value.m);
            local_to_world.dirty = true;

            rigid_body.linear_velocity = to_vector3(native.getLinearVelocity());
            rigid_body.angular_velocity = to_vector3(native.getAngularVelocity());
        });
}

nodec::optional<nodec_physics::RayCastHit> PhysicsSystemBackend::ray_cast(const nodec::Vector3f &ray_start, const nodec::Vector3f &ray_end) {
    using namespace nodec;
    using namespace nodec_physics;

    btVector3 bt_ray_start(ray_start.x, ray_start.y, ray_start.z);
    btVector3 bt_ray_end(ray_end.x, ray_end.y, ray_end.z);

    btCollisionWorld::ClosestRayResultCallback ray_callback(bt_ray_start, bt_ray_end);

    dynamics_world_->rayTest(bt_ray_start, bt_ray_end, ray_callback);

    if (ray_callback.hasHit()) {
        RayCastHit hit{};
        // TODO: get the entity from the rigid body.
        hit.point = to_vector3(ray_callback.m_hitPointWorld);
        hit.normal = to_vector3(ray_callback.m_hitNormalWorld);
        return hit;
    }

    return nodec::nullopt;
}

void PhysicsSystemBackend::contact_test(nodec_scene::SceneEntity entity, std::function<void(nodec_physics::CollisionInfo &)> callback) {
    auto &scene_registry = world_.scene().registry();

    auto activity = scene_registry.try_get_component<CollisionObjectActivity>(entity);
    if (!activity) return;

    auto &collision_body = *activity->collision_object_backend;

    struct Callback : public btCollisionWorld::ContactResultCallback {
        Callback(std::function<void(nodec_physics::CollisionInfo &)> callback)
            : callback(callback) {}

        btScalar addSingleResult(btManifoldPoint &cp,
                                 const btCollisionObjectWrapper *col_obj0_wrap,
                                 int part_id0,
                                 int index0,
                                 const btCollisionObjectWrapper *col_obj1_wrap,
                                 int part_id1,
                                 int index1) override {
            using namespace nodec;
            using namespace nodec_physics;

            CollisionInfo collision_info{};

            auto *body0 = static_cast<const RigidBodyBackend *>(col_obj0_wrap->getCollisionObject()->getUserPointer());
            auto *body1 = static_cast<const RigidBodyBackend *>(col_obj1_wrap->getCollisionObject()->getUserPointer());

            collision_info.self = body0->entity();
            collision_info.other = body1->entity();

            callback(collision_info);

            return 0.f;
        }

        std::function<void(nodec_physics::CollisionInfo &)> callback;
    };
    dynamics_world_->contactTest(&collision_body.native_collision_object(), Callback(callback));
}