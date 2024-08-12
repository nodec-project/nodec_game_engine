#ifndef NODEC_GAME_ENGINE__RENDERING__CAMERA_STATE_HPP_
#define NODEC_GAME_ENGINE__RENDERING__CAMERA_STATE_HPP_

#include <memory>

#include <DirectXMath.h>

#include <nodec/gfx/frustum.hpp>
#include <nodec/gfx/gfx.hpp>
#include <nodec_rendering/components/camera.hpp>

class CameraState {
public:
    CameraState()
        : aspect_(1.f), matrix_p_(DirectX::XMMatrixIdentity()), matrix_p_inverse_(DirectX::XMMatrixIdentity()),
          matrix_v_(DirectX::XMMatrixIdentity()), matrix_v_inverse_(DirectX::XMMatrixIdentity()) {
        // frustum_object_ = std::make_unique<btCollisionObject>();
    }

    void update_projection(const nodec_rendering::components::Camera &camera, float aspect) {
        using namespace DirectX;

        camera_ = camera;
        switch (camera.projection) {
        case nodec_rendering::components::Camera::Projection::Perspective:
            matrix_p_ = XMMatrixPerspectiveFovLH(
                XMConvertToRadians(camera.fov_angle),
                aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;
        case nodec_rendering::components::Camera::Projection::Orthographic:
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
    }

    void update_transform(const nodec::Matrix4x4f &camera_local_to_world) {
        using namespace DirectX;
        using namespace nodec;
        matrix_v_inverse_ = XMMATRIX(camera_local_to_world.m);
        matrix_v_ = XMMatrixInverse(nullptr, matrix_v_inverse_);

        nodec::gfx::TRSComponents trs;
        nodec::gfx::decompose_trs(camera_local_to_world, trs);

        const auto up = nodec::gfx::rotate(Vector3f(0.f, 1.f, 0.f), trs.rotation);
        const auto right = nodec::gfx::rotate(Vector3f(1.f, 0.f, 0.f), trs.rotation);
        const auto forward = nodec::gfx::rotate(Vector3f(0.f, 0.f, 1.f), trs.rotation);

        switch (camera_.projection) {
        case nodec_rendering::components::Camera::Projection::Perspective:
            nodec::gfx::set_frustum_from_projection_lh(trs.translation, forward, up, right,
                                                       aspect_, camera_.fov_angle, camera_.near_clip_plane, camera_.far_clip_plane, frustum_);
            break;
        case nodec_rendering::components::Camera::Projection::Orthographic:
            nodec::gfx::set_frustum_from_orthographic(trs.translation, forward, up, right,
                                                      camera_.ortho_width, camera_.ortho_width / aspect_, camera_.near_clip_plane, camera_.far_clip_plane, frustum_);
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

#endif