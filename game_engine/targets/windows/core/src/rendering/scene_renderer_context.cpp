#include <rendering/scene_renderer_context.hpp>

SceneRendererContext::SceneRendererContext(std::shared_ptr<nodec::logging::Logger> logger, Graphics &gfx,
                                           nodec::resource_management::ResourceRegistry &resource_registry)
    : logger_(logger), gfx_(gfx),
      font_character_database_(&gfx),
      cb_scene_properties_(gfx),
      cb_model_properties_(gfx),
      cb_texture_config_(gfx), rs_cull_none_(gfx, D3D11_CULL_NONE),
      rs_cull_front_(gfx, D3D11_CULL_FRONT),
      rs_cull_back_(gfx, D3D11_CULL_BACK),
      bs_default_(BlendState::CreateDefaultBlend(gfx)),
      bs_alpha_blend_(BlendState::CreateAlphaBlend(gfx)) {
    using namespace nodec_rendering::resources;
    using namespace nodec::resource_management;
    using namespace nodec;

    // Get quad mesh from resource registry.
    {
        // 0.5 x 0.5 quad.
        auto quad_mesh = resource_registry.get_resource_direct<Mesh>("org.nodec.game-engine/meshes/quad.mesh");

        if (!quad_mesh) {
            logger_->fatal(__FILE__, __LINE__) << "Cannot load the essential resource 'quad.mesh'.\n"
                                                  "Make sure the 'org.nodec.game-engine' resource-package is installed.";
            throw std::runtime_error("Cannot load the essential resource 'quad.mesh'.");
        }

        quad_mesh_ = std::static_pointer_cast<MeshBackend>(quad_mesh);
    }

    {
        auto norm_cube_mesh = resource_registry.get_resource_direct<Mesh>("org.nodec.game-engine/meshes/norm-cube.mesh");
        if (!norm_cube_mesh) {
            logger_->fatal(__FILE__, __LINE__) << "Cannot load the essential resource 'norm-cube.mesh'.\n"
                                                  "Make sure the 'org.nodec.game-engine' resource-package is installed.";
            throw std::runtime_error("Cannot load the essential resource 'norm-cube.mesh'.");
        }
        norm_cube_mesh_ = std::static_pointer_cast<MeshBackend>(norm_cube_mesh);
    }

    // Make screen quad mesh in NDC space which is not depend on target view size.
    {
        screen_quad_mesh_.reset(new MeshBackend());

        screen_quad_mesh_->vertices.resize(4);
        screen_quad_mesh_->triangles.resize(6);

        screen_quad_mesh_->vertices[0] = {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}};
        screen_quad_mesh_->vertices[1] = {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}};
        screen_quad_mesh_->vertices[2] = {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}};
        screen_quad_mesh_->vertices[3] = {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}};

        screen_quad_mesh_->triangles[0] = 0;
        screen_quad_mesh_->triangles[1] = 1;
        screen_quad_mesh_->triangles[2] = 2;

        screen_quad_mesh_->triangles[3] = 0;
        screen_quad_mesh_->triangles[4] = 2;
        screen_quad_mesh_->triangles[5] = 3;
        screen_quad_mesh_->update_device_memory(&gfx_);
    }
}