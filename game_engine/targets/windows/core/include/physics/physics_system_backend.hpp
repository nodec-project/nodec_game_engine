#ifndef NODEC_GAME_ENGINE__PHYSICS__PHYSICS_SYSTEM_BACKEND_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__PHYSICS_SYSTEM_BACKEND_HPP_

#include <nodec_physics/systems/physics_system.hpp>
#include <nodec_world/world.hpp>

#include <btBulletDynamicsCommon.h>

#include <memory>

class PhysicsSystemBackend final : public nodec_physics::systems::PhysicsSystem {
public:
    PhysicsSystemBackend(nodec_world::World &world)
        : PhysicsSystem(collision_stay_signal_) {
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

private:
    void on_stepped(nodec_world::World &world);

private:
    CollisionSignal collision_stay_signal_;

    std::unique_ptr<btDefaultCollisionConfiguration> collision_config_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btBroadphaseInterface> overlapping_pair_cache_;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
    std::unique_ptr<btDynamicsWorld> dynamics_world_;
};

#endif