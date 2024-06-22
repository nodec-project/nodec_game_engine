#ifndef NODEC_GAME_ENGINE__RENDERING__SCENE_RENDERING_CONTEXT_HPP_
#define NODEC_GAME_ENGINE__RENDERING__SCENE_RENDERING_CONTEXT_HPP_

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

#include <d3d11.h>
#include <wrl.h>

#include <nodec_rendering/cull_mode.hpp>

#include "../graphics/RasterizerState.hpp"
#include "../graphics/geometry_buffer.hpp"
#include "../graphics/graphics.hpp"

class SceneRenderingContext {
public:
    SceneRenderingContext(std::uint32_t target_width, std::uint32_t target_height, Graphics &gfx);

    GeometryBuffer &geometry_buffer(const std::string &name) {
        return geometry_buffer(name, target_width_, target_height_);
    }

    GeometryBuffer &geometry_buffer(const std::string &name, std::uint32_t width, std::uint32_t height) {
        auto &buffer = geometry_buffers_[name];
        if (!buffer) {
            buffer.reset(new GeometryBuffer(&gfx_, width, height));
        }
        return *buffer;
    }

    void swap_geometry_buffers(const std::string &lhs, const std::string &rhs) {
        std::swap(geometry_buffers_[lhs], geometry_buffers_[rhs]);
    }

    /**
     * @brief Get shader resource view from managed buffers(including geometry buffers or depth stencil buffer).
     *
     * @param name
     * @return ID3D11ShaderResourceView*
     */
    ID3D11ShaderResourceView *shader_resource_view(const std::string &name) {
        if (name == "$depth") {
            return depth_stencil_srv_.Get();
        }

        {
            auto iter = geometry_buffers_.find(name);
            if (iter == geometry_buffers_.end()) {
                return nullptr;
            }
            return &iter->second->shader_resource_view();
        }
    }

    std::uint32_t target_width() const noexcept {
        return target_width_;
    }

    std::uint32_t target_height() const noexcept {
        return target_height_;
    }

    ID3D11DepthStencilView &depth_stencil_view() {
        return *depth_stencil_view_.Get();
    }

    std::size_t geometry_buffer_count() const noexcept {
        return geometry_buffers_.size();
    }

    decltype(auto) geometry_buffer_begin() const noexcept {
        return geometry_buffers_.begin();
    }

    decltype(auto) geometry_buffer_end() const noexcept {
        return geometry_buffers_.end();
    }

private:
    std::uint32_t target_width_;
    std::uint32_t target_height_;
    Graphics &gfx_;
    std::unordered_map<std::string, std::unique_ptr<GeometryBuffer>> geometry_buffers_;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_stencil_texture_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depth_stencil_srv_;
};

#endif