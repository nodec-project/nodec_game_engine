#ifndef NODEC_GAME_ENGINE__PHYSICS__PHYSICS_SYSTEM_BACKEND_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__PHYSICS_SYSTEM_BACKEND_HPP_

#include <memory>

#include <nodec_physics/systems/physics_system.hpp>
#include <nodec_world/world.hpp>

#include <btBulletDynamicsCommon.h>

class PhysicsSystemBackend final : public nodec_physics::systems::PhysicsSystem {
public:
    PhysicsSystemBackend(nodec_world::World &world)
        : world_(world) {
        collision_config_.reset(new btDefaultCollisionConfiguration());
        dispatcher_.reset(new btCollisionDispatcher(collision_config_.get()));
        overlapping_pair_cache_.reset(new btDbvtBroadphase());
        solver_.reset(new btSequentialImpulseConstraintSolver());
        dynamics_world_.reset(new btDiscreteDynamicsWorld(dispatcher_.get(), overlapping_pair_cache_.get(), solver_.get(), collision_config_.get()));

        world.stepped().connect([&](auto &world) {
            on_stepped(world);
        });
    }

    ~PhysicsSystemBackend() {
    }

    nodec::optional<nodec_physics::RayCastHit> ray_cast(const nodec::Vector3f &ray_start, const nodec::Vector3f &ray_end) override;

    void contact_test(nodec_scene::SceneEntity entity, std::function<void(nodec_physics::CollisionInfo &)> callback) override;

private:
    void on_stepped(nodec_world::World &world);

private:
    nodec_world::World &world_;

    std::unique_ptr<btDefaultCollisionConfiguration> collision_config_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btBroadphaseInterface> overlapping_pair_cache_;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
    std::unique_ptr<btDynamicsWorld> dynamics_world_;
};

#endif