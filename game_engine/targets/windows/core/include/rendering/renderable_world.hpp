#ifndef NODEC_GAME_ENGINE__RENDERING__RENDERABLE_WORLD_HPP_
#define NODEC_GAME_ENGINE__RENDERING__RENDERABLE_WORLD_HPP_

#include <cmath>
#include <memory>

#include <DirectXMath.h>
#include <btBulletCollisionCommon.h>

#include <nodec/gfx/frustum.hpp>
#include <nodec/math/gfx.hpp>
#include <nodec_rendering/components/camera.hpp>

#include "mesh_backend.hpp"

inline std::unique_ptr<btConvexHullShape> bt_make_frustum_shape(const btVector3 &position, const btVector3 &look_at,
                                                                const btVector3 &up, float fov, float aspect, float near_distance, float far_distance) {
    // 左手系の外積を使用して右方向ベクトルを計算
    btVector3 right = up.cross(look_at).normalized();
    btVector3 forward = look_at.normalized();
    btVector3 near_center = position + forward * near_distance;
    btVector3 far_center = position + forward * far_distance;

    float near_height = 2 * near_distance * std::tan(fov / 2);
    float near_width = near_height * aspect;
    float far_height = 2 * far_distance * std::tan(fov / 2);
    float far_width = far_height * aspect;

    btVector3 near_top_left = near_center + (up * (near_height / 2)) - (right * (near_width / 2));
    btVector3 near_top_right = near_center + (up * (near_height / 2)) + (right * (near_width / 2));
    btVector3 near_bottom_left = near_center - (up * (near_height / 2)) - (right * (near_width / 2));
    btVector3 near_bottom_right = near_center - (up * (near_height / 2)) + (right * (near_width / 2));

    btVector3 far_top_left = far_center + (up * (far_height / 2)) - (right * (far_width / 2));
    btVector3 far_top_right = far_center + (up * (far_height / 2)) + (right * (far_width / 2));
    btVector3 far_bottom_left = far_center - (up * (far_height / 2)) - (right * (far_width / 2));
    btVector3 far_bottom_right = far_center - (up * (far_height / 2)) + (right * (far_width / 2));

    std::unique_ptr<btConvexHullShape> shape = std::make_unique<btConvexHullShape>();
    shape->addPoint(near_top_left);
    shape->addPoint(near_top_right);
    shape->addPoint(near_bottom_left);
    shape->addPoint(near_bottom_right);
    shape->addPoint(far_top_left);
    shape->addPoint(far_top_right);
    shape->addPoint(far_bottom_left);
    shape->addPoint(far_bottom_right);

    return shape;
}

class RenderableWorld {
public:
    RenderableWorld() {
        collision_configuration_ = std::make_unique<btDefaultCollisionConfiguration>();
        dispatcher_ = std::make_unique<btCollisionDispatcher>(collision_configuration_.get());
        broadphase_ = std::make_unique<btDbvtBroadphase>();
        collision_world_ = std::make_unique<btCollisionWorld>(dispatcher_.get(), broadphase_.get(), collision_configuration_.get());
    }

    btCollisionWorld &collision_world() {
        return *collision_world_;
    }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> collision_configuration_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btBroadphaseInterface> broadphase_;
    std::unique_ptr<btCollisionWorld> collision_world_;
};

class CameraState {
public:
    CameraState()
        : aspect_(1.f), matrix_p_(DirectX::XMMatrixIdentity()), matrix_p_inverse_(DirectX::XMMatrixIdentity()),
          matrix_v_(DirectX::XMMatrixIdentity()), matrix_v_inverse_(DirectX::XMMatrixIdentity()) {
        // frustum_object_ = std::make_unique<btCollisionObject>();
    }

    void update_projection(const nodec_rendering::components::Camera &camera, float aspect) {
        camera_ = camera;
        using namespace DirectX;
        switch (camera.projection) {
        case nodec_rendering::components::Camera::Projection::Perspective:
            // frustum_shape_ = bt_make_frustum_shape(btVector3(0, 0, 0), btVector3(0, 0, 1), btVector3(0, 1, 0),
            //                                        camera.fov_angle, aspect, camera.near_clip_plane, camera.far_clip_plane);

            matrix_p_ = XMMatrixPerspectiveFovLH(
                XMConvertToRadians(camera.fov_angle),
                aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;
        case nodec_rendering::components::Camera::Projection::Orthographic:
            // frustum_shape_ = std::make_unique<btBoxShape>(btVector3(camera.ortho_width / 2, camera.ortho_width / 2, camera.far_clip_plane));

            matrix_p_ = XMMatrixOrthographicLH(
                camera.ortho_width, camera.ortho_width / aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;

        default:
            matrix_p_ = XMMatrixIdentity();
            break;
        }
        matrix_p_inverse_ = XMMatrixInverse(nullptr, matrix_p_);

        aspect_ = aspect;

        // frustum_object_->setCollisionShape(frustum_shape_.get());
    }

    void update_transform(const nodec::Matrix4x4f &camera_local_to_world) {
        using namespace DirectX;
        using namespace nodec;
        matrix_v_inverse_ = XMMATRIX(camera_local_to_world.m);
        matrix_v_ = XMMatrixInverse(nullptr, matrix_v_inverse_);

        // btTransform transform;
        // transform.setFromOpenGLMatrix(camera_local_to_world.m);
        // frustum_object_->setWorldTransform(transform);

        nodec::math::gfx::TRSComponents trs;
        nodec::math::gfx::decompose_trs(camera_local_to_world, trs);

        const auto up = nodec::math::gfx::rotate(Vector3f(0.f, 1.f, 0.f), trs.rotation);
        const auto right = nodec::math::gfx::rotate(Vector3f(1.f, 0.f, 0.f), trs.rotation);
        const auto forward = nodec::math::gfx::rotate(Vector3f(0.f, 0.f, 1.f), trs.rotation);
        
        switch (camera_.projection) {
        case nodec_rendering::components::Camera::Projection::Perspective:
            nodec::gfx::set_frustum_from_camera_lh(trs.translation, forward, up, right,
                                                   aspect_, camera_.fov_angle, camera_.near_clip_plane, camera_.far_clip_plane, frustum_);
            break;
        default:
            break;
        }
    }

    const DirectX::XMMATRIX &matrix_p() const {
        return matrix_p_;
    }

    const DirectX::XMMATRIX &matrix_p_inverse() const {
        return matrix_p_inverse_;
    }

    const DirectX::XMMATRIX &matrix_v() const {
        return matrix_v_;
    }

    const DirectX::XMMATRIX &matrix_v_inverse() const {
        return matrix_v_inverse_;
    }

    const nodec::gfx::Frustum &frustum() const {
        return frustum_;
    }

    // btCollisionObject *frustum_object() const {
    //     return frustum_object_.get();
    // }

private:
    float aspect_;
    nodec::gfx::Frustum frustum_;
    nodec_rendering::components::Camera camera_;
    DirectX::XMMATRIX matrix_p_;
    DirectX::XMMATRIX matrix_p_inverse_;
    DirectX::XMMATRIX matrix_v_;
    DirectX::XMMATRIX matrix_v_inverse_;
};

struct CameraActivity {
    std::unique_ptr<CameraState> state;
};
//
//class RenderableObject {
//public:
//    RenderableObject(nodec_scene::SceneEntity entity)
//        : collision_world_(nullptr), entity_(entity) {
//        collision_object_ = std::make_unique<btCollisionObject>();
//        collision_object_->setUserPointer(this);
//    }
//
//    ~RenderableObject() {
//        if (collision_world_) {
//            collision_world_->removeCollisionObject(collision_object_.get());
//        }
//    }
//
//    void update_bounds_if_needed(const nodec::BoundingBox &bounds,
//                                 const nodec::Matrix4x4f &local_to_world) {
//        if (bounds_ != bounds || !collision_shape_) {
//            bounds_ = bounds;
//            auto collision_shape = std::make_unique<btBoxShape>(btVector3(bounds.extents.x, bounds.extents.y, bounds.extents.z));
//            collision_object_->setCollisionShape(collision_shape.get());
//            collision_shape_ = std::move(collision_shape);
//        }
//        auto local_to_world_new = local_to_world * nodec::math::gfx::translation_matrix(bounds.center);
//        if (local_to_world_ != local_to_world_new) {
//            local_to_world_ = local_to_world_new;
//            btTransform transform;
//            transform.setFromOpenGLMatrix(local_to_world_.m);
//            collision_object_->setWorldTransform(transform);
//        }
//    }
//
//    void bind_world(btCollisionWorld &world) {
//        if (collision_world_) {
//            return;
//        }
//        collision_world_ = &world;
//        world.addCollisionObject(collision_object_.get());
//    }
//
//    void reset_bounds() {
//        if (collision_world_) {
//            collision_world_->removeCollisionObject(collision_object_.get());
//            collision_world_ = nullptr;
//        }
//    }
//
//    nodec_scene::SceneEntity entity() const {
//        return entity_;
//    }
//
//private:
//    nodec::BoundingBox bounds_;
//    btCollisionWorld *collision_world_;
//    std::unique_ptr<btBoxShape> collision_shape_;
//    std::unique_ptr<btCollisionObject> collision_object_;
//    nodec::Matrix4x4f local_to_world_;
//    nodec_scene::SceneEntity entity_;
//};
//
//struct RenderableObjectActivity {
//    std::unique_ptr<RenderableObject> object;
//};

#endif