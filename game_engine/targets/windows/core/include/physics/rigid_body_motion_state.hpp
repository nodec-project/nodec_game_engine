#ifndef NODEC_GAME_ENGINE__PHYSICS__RIGID_BODY_MOTION_STATE_HPP_
#define NODEC_GAME_ENGINE__PHYSICS__RIGID_BODY_MOTION_STATE_HPP_

#include <btBulletDynamicsCommon.h>

class RigidBodyMotionState final : public btMotionState {
public:
    RigidBodyMotionState() {
        current_trfm_.setIdentity();
    }

    /// synchronizes world transform from user to physics
    void getWorldTransform(btTransform &trfm) const override {
        trfm = current_trfm_;
    }

    /// synchronizes world transform from physics to user
    /// Bullet only calls the update of world transform for active objects
    void setWorldTransform(const btTransform &trfm) override {
        current_trfm_ = trfm;
        dirty = true;
    }

    bool dirty{false};

private:
    btTransform current_trfm_;
};

#endif