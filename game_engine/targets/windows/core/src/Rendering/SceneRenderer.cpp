#include <Font/FontBackend.hpp>
#include <Rendering/SceneRenderer.hpp>

#include <nodec/unicode.hpp>

#include <DirectXMath.h>

// namespace {
//
// D3D11_VIEWPORT make_viewport(float width, float height) {
//     D3D11_VIEWPORT vp;
//     vp.Width = width;
//     vp.Height = height;
//     vp.MinDepth = 0.0f;
//     vp.MaxDepth = 1.0f;
//     vp.TopLeftX = 0.0f;
//     vp.TopLeftY = 0.0f;
//     return vp;
// }
// } // namespace

SceneRenderer::SceneRenderer(Graphics *gfx, nodec::resource_management::ResourceRegistry &resourceRegistry)
    : gfx_(gfx),
      mScenePropertiesCB(gfx, sizeof(SceneProperties), &mSceneProperties),
      mModelPropertiesCB(gfx, sizeof(ModelProperties), &mModelProperties),
      mTextureConfigCB(gfx, sizeof(TextureConfig), &mTextureConfig),
      mBSDefault(BlendState::CreateDefaultBlend(gfx)),
      mBSAlphaBlend(BlendState::CreateAlphaBlend(gfx)),
      mRSCullBack{gfx, D3D11_CULL_BACK},
      mRSCullNone{gfx, D3D11_CULL_NONE},
      mFontCharacterDatabase{gfx} {
    using namespace nodec_rendering::resources;
    using namespace nodec::resource_management;
    using namespace nodec;

    // Get quad mesh from resource registry.
    {
        auto quadMesh = resourceRegistry.get_resource_direct<Mesh>("org.nodec.game-engine/meshes/quad.mesh");

        if (!quadMesh) {
            logging::WarnStream(__FILE__, __LINE__) << "[SceneRenderer] >>> Cannot load the essential resource 'quad.mesh'.\n"
                                                       "Make sure the 'org.nodec.game-engine' resource-package is installed.";
        }

        mQuadMesh = std::static_pointer_cast<MeshBackend>(quadMesh);
    }

    // Make screen quad mesh in NDC space which is not depend on target view size.
    {
        mScreenQuadMesh.reset(new MeshBackend());

        mScreenQuadMesh->vertices.resize(4);
        mScreenQuadMesh->triangles.resize(6);

        mScreenQuadMesh->vertices[0] = {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}};
        mScreenQuadMesh->vertices[1] = {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}};
        mScreenQuadMesh->vertices[2] = {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}};
        mScreenQuadMesh->vertices[3] = {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}};

        mScreenQuadMesh->triangles[0] = 0;
        mScreenQuadMesh->triangles[1] = 1;
        mScreenQuadMesh->triangles[2] = 2;

        mScreenQuadMesh->triangles[3] = 0;
        mScreenQuadMesh->triangles[4] = 2;
        mScreenQuadMesh->triangles[5] = 3;
        mScreenQuadMesh->update_device_memory(gfx_);
    }
}

void SceneRenderer::Render(nodec_scene::Scene &scene, ID3D11RenderTargetView &render_target, SceneRenderingContext &context) {
    // assert(context != nullptr);

    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec_rendering;
    using namespace nodec_rendering::components;
    using namespace nodec_rendering::resources;
    using namespace DirectX;

    mScenePropertiesCB.BindVS(gfx_, SCENE_PROPERTIES_CB_SLOT);
    mScenePropertiesCB.BindPS(gfx_, SCENE_PROPERTIES_CB_SLOT);

    mTextureConfigCB.BindVS(gfx_, TEXTURE_CONFIG_CB_SLOT);
    mTextureConfigCB.BindPS(gfx_, TEXTURE_CONFIG_CB_SLOT);

    SetupSceneLighting(scene);

    // Render the scene per each camera.
    scene.registry().view<const Camera, const Transform>().each([&](SceneEntity cameraEntt, const Camera &camera, const Transform &cameraTrfm) {
        ID3D11RenderTargetView *pCameraRenderTargetView = &render_target;

        // --- Get active post process effects. ---
        std::vector<const PostProcessing::Effect *> activePostProcessEffects;
        {
            const auto *postProcessing = scene.registry().try_get_component<const PostProcessing>(cameraEntt);

            if (postProcessing) {
                for (const auto &effect : postProcessing->effects) {
                    if (effect.enabled && effect.material && effect.material->shader()) {
                        activePostProcessEffects.push_back(&effect);
                    }
                }
            }

            // If some effects, the off-screen buffer is needed.
            if (activePostProcessEffects.size() > 0) {
                auto &buffer = context.geometry_buffer("screen");

                // Set the render target.
                pCameraRenderTargetView = &buffer.render_target_view();
            }
        }

        auto matrixP = XMMatrixIdentity();

        const auto aspect = static_cast<float>(context.target_width()) / context.target_height();
        switch (camera.projection) {
        case Camera::Projection::Perspective:
            matrixP = XMMatrixPerspectiveFovLH(
                XMConvertToRadians(camera.fov_angle),
                aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;
        case Camera::Projection::Orthographic:
            matrixP = XMMatrixOrthographicLH(
                camera.ortho_width, camera.ortho_width / aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;
        default:
            break;
        }
        const auto matrixPInverse = XMMatrixInverse(nullptr, matrixP);

        XMMATRIX matrixVInverse{cameraTrfm.local2world.m};

        auto matrixV = XMMatrixInverse(nullptr, matrixVInverse);

        Render(scene, matrixV, matrixVInverse, matrixP, matrixPInverse,
               pCameraRenderTargetView, context);

        // --- Post Processing ---
        if (activePostProcessEffects.size() > 0) {
            gfx_->context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            for (std::size_t i = 0; i < activePostProcessEffects.size(); ++i) {
                if (i != activePostProcessEffects.size() - 1) {
                    pCameraRenderTargetView = &context.geometry_buffer("screen_back").render_target_view();
                } else {
                    // if last, render target is frame buffer.
                    pCameraRenderTargetView = &render_target;
                }

                // It is assured that material and shader are exists.
                // It is checked at the beginning of rendering pass of camera.
                auto materialBackend = std::static_pointer_cast<MaterialBackend>(activePostProcessEffects[i]->material);
                auto shaderBackend = std::static_pointer_cast<ShaderBackend>(materialBackend->shader());

                SetCullMode(materialBackend->cull_mode());
                materialBackend->bind_constant_buffer(gfx_, MATERIAL_PROPERTIES_CB_SLOT);

                mTextureConfig.texHasFlag = 0x00;
                const auto slotOffset = bind_texture_entries(materialBackend->texture_entries(), mTextureConfig.texHasFlag);
                mTextureConfigCB.Update(gfx_, &mTextureConfig);

                for (int passNum = 0; passNum < shaderBackend->pass_count(); ++passNum) {
                    if (passNum == shaderBackend->pass_count() - 1) {
                        // If last pass.
                        // The render target must be one final target.
                        D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
                        gfx_->context().RSSetViewports(1u, &vp);
                        gfx_->context().OMSetRenderTargets(1, &pCameraRenderTargetView, nullptr);

                    } else {
                        // If halfway pass.
                        // Support the multiple render targets.

                        const auto &targets = shaderBackend->render_targets(passNum);

                        std::vector<ID3D11RenderTargetView *> renderTargets(targets.size());
                        std::vector<D3D11_VIEWPORT> vps(targets.size());
                        for (size_t i = 0; i < targets.size(); ++i) {
                            auto &buffer = context.geometry_buffer(targets[i]);
                            renderTargets[i] = &buffer.render_target_view();
                            gfx_->context().ClearRenderTargetView(renderTargets[i], Vector4f::zero.v);

                            vps[i] = CD3D11_VIEWPORT(0.f, 0.f, buffer.width(), buffer.height());
                        }

                        gfx_->context().OMSetRenderTargets(renderTargets.size(), renderTargets.data(), nullptr);
                        gfx_->context().RSSetViewports(vps.size() , vps.data());
                    }

                    // --- Bind texture resources.
                    // Bind sampler for textures.

                    GetSamplerState({Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}).BindPS(gfx_, slotOffset);
                    const auto &textureResources = shaderBackend->texture_resources(passNum);
                    for (std::size_t i = 0; i < textureResources.size(); ++i) {
                        auto &buffer = context.geometry_buffer(textureResources[i]);
                        auto *view = &buffer.shader_resource_view();
                        gfx_->context().PSSetShaderResources(slotOffset + i, 1u, &view);
                    }
                    shaderBackend->bind(gfx_, passNum);

                    mScreenQuadMesh->bind(gfx_);
                    gfx_->DrawIndexed(mScreenQuadMesh->triangles.size());
                } // End foreach pass.

                context.swap_geometry_buffers("screen", "screen_back");
            } // End foreach effect.
        }
    }); // End foreach camera
}

void SceneRenderer::Render(nodec_scene::Scene &scene, nodec::Matrix4x4f &view, nodec::Matrix4x4f &projection, ID3D11RenderTargetView *pTarget, SceneRenderingContext &context) {
    assert(pTarget != nullptr);

    using namespace DirectX;
    using namespace nodec;

    mScenePropertiesCB.BindVS(gfx_, SCENE_PROPERTIES_CB_SLOT);
    mScenePropertiesCB.BindPS(gfx_, SCENE_PROPERTIES_CB_SLOT);

    mTextureConfigCB.BindVS(gfx_, TEXTURE_CONFIG_CB_SLOT);
    mTextureConfigCB.BindPS(gfx_, TEXTURE_CONFIG_CB_SLOT);

    SetupSceneLighting(scene);

    XMMATRIX matrixV{view.m};
    auto matrixVInverse = XMMatrixInverse(nullptr, matrixV);

    XMMATRIX matrixP{projection.m};
    auto matrixPInverse = XMMatrixInverse(nullptr, matrixP);

    Render(scene, matrixV, matrixVInverse, matrixP, matrixPInverse,
           pTarget, context);
}

void SceneRenderer::Render(nodec_scene::Scene &scene,
                           const DirectX::XMMATRIX &matrixV, const DirectX::XMMATRIX &matrixVInverse,
                           const DirectX::XMMATRIX &matrixP, const DirectX::XMMATRIX &matrixPInverse,
                           ID3D11RenderTargetView *pTarget, SceneRenderingContext &context) {
    assert(pTarget != nullptr);
    using namespace nodec;
    using namespace nodec_scene::components;
    using namespace nodec_rendering::components;
    using namespace nodec_rendering;
    using namespace DirectX;

    // TODO: Culling.

    // At first, we get the active shaders to render for each shader.
    std::vector<ShaderBackend *> activeShaders;
    {
        std::unordered_set<ShaderBackend *> shaders;
        scene.registry().view<const MeshRenderer, const Transform>(type_list<NonVisible>{}).each([&](auto entt, const MeshRenderer &renderer, const Transform &trfm) {
            for (auto &material : renderer.materials) {
                if (!material) continue;
                auto *backend = static_cast<const MaterialBackend *>(material.get());
                auto *shader = static_cast<ShaderBackend *>(backend->shader().get());
                if (!shader) continue;
                shaders.emplace(shader);
            }
        });

        scene.registry().view<const ImageRenderer, const Transform>(type_list<NonVisible>{}).each([&](auto entt, const ImageRenderer &renderer, const Transform &trfm) {
            auto *material = static_cast<const MaterialBackend *>(renderer.material.get());
            if (!material) return;
            auto *shader = static_cast<ShaderBackend *>(material->shader().get());
            if (!shader) return;
            shaders.emplace(shader);
        });

        scene.registry().view<const TextRenderer, const Transform>(type_list<NonVisible>{}).each([&](auto entt, const TextRenderer &renderer, const Transform &transform) {
            if (!renderer.material) return;
            auto *shader = renderer.material->shader().get();
            if (shader == nullptr) return;
            shaders.emplace(static_cast<ShaderBackend *>(shader));
        });

        // now let's sort by priority.
        activeShaders.resize(shaders.size());
        std::copy(shaders.begin(), shaders.end(), activeShaders.begin());
        std::sort(activeShaders.begin(), activeShaders.end(),
                  [](const auto &a, const auto &b) { return a->rendering_priority() < b->rendering_priority(); });
    }

    if (activeShaders.size() == 0) return;

    // Clear render target view with solid color.
    // TODO: Consider skybox.
    {
        const float color[] = {0.1f, 0.1f, 0.1f, 1.0f};
        gfx_->context().ClearRenderTargetView(pTarget, color);
    }

    XMStoreFloat4x4(&mSceneProperties.matrixP, matrixP);
    XMStoreFloat4x4(&mSceneProperties.matrixPInverse, matrixPInverse);

    XMStoreFloat4x4(&mSceneProperties.matrixV, matrixV);
    XMStoreFloat4x4(&mSceneProperties.matrixVInverse, matrixVInverse);

    XMVECTOR scale, rotQuat, trans;
    XMMatrixDecompose(&scale, &rotQuat, &trans, matrixVInverse);

    mSceneProperties.cameraPos.set(
        XMVectorGetByIndex(trans, 0),
        XMVectorGetByIndex(trans, 1),
        XMVectorGetByIndex(trans, 2),
        XMVectorGetByIndex(trans, 3));

    // Update active point lights.
    {
        mSceneProperties.lights.numOfPointLights = 0;
        auto view = scene.registry().view<const Transform, const nodec_rendering::components::PointLight>();
        for (const auto &entt : view) {
            // TODO: Light culling.
            const auto index = mSceneProperties.lights.numOfPointLights;
            if (index >= MAX_NUM_OF_POINT_LIGHTS) break;

            const auto &trfm = view.get<const Transform>(entt);
            const auto &light = view.get<const nodec_rendering::components::PointLight>(entt);
            const auto worldPosition = trfm.local2world * Vector4f(0, 0, 0, 1.0f);

            mSceneProperties.lights.pointLights[index].position.set(worldPosition.x, worldPosition.y, worldPosition.z);
            mSceneProperties.lights.pointLights[index].color.set(light.color.x, light.color.y, light.color.z);
            mSceneProperties.lights.pointLights[index].intensity = light.intensity;
            mSceneProperties.lights.pointLights[index].range = light.range;

            ++mSceneProperties.lights.numOfPointLights;
        }
    }

    mScenePropertiesCB.Update(gfx_, &mSceneProperties);

    // Reset depth buffer.
    gfx_->context().ClearDepthStencilView(&context.depth_stencil_view(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Executes the shader programs.
    for (auto *activeShader : activeShaders) {
        if (activeShader->pass_count() == 1) {
            // One pass shader.
            gfx_->context().OMSetRenderTargets(1, &pTarget, &context.depth_stencil_view());
            D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
            gfx_->context().RSSetViewports(1u, &vp);
            gfx_->context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            activeShader->bind(gfx_);

            // now lets draw the mesh.
            RenderModel(scene, activeShader, matrixV, matrixP);
        } else {
            // Multi pass shader.
            const auto passCount = activeShader->pass_count();
            // first pass
            {
                const auto &targets = activeShader->render_targets(0);

                std::vector<ID3D11RenderTargetView *> renderTargets(targets.size());
                std::vector<D3D11_VIEWPORT> vps(targets.size());
                for (size_t i = 0; i < targets.size(); ++i) {
                    auto &buffer = context.geometry_buffer(targets[i]);
                    renderTargets[i] = &buffer.render_target_view();
                    gfx_->context().ClearRenderTargetView(renderTargets[i], Vector4f::zero.v);

                    vps[i] = CD3D11_VIEWPORT(0.f, 0.f, buffer.width(), buffer.height());
                }

                gfx_->context().OMSetRenderTargets(renderTargets.size(), renderTargets.data(), &context.depth_stencil_view());
                gfx_->context().RSSetViewports(vps.size(), vps.data());
                gfx_->context().ClearDepthStencilView(&context.depth_stencil_view(), D3D11_CLEAR_DEPTH, 1.0f, 0);

                gfx_->context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                activeShader->bind(gfx_, 0);

                RenderModel(scene, activeShader, matrixV, matrixP);
            }

            // medium pass
            // NOTE: This pass maybe not necessary.
            //  Alternatively, we can use the another shader pass.
            {

            }

            // final pass
            {
                const auto passNum = passCount - 1;

                gfx_->context().OMSetRenderTargets(1, &pTarget, nullptr);
                D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
                gfx_->context().RSSetViewports(1u, &vp);
                gfx_->context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                const auto &textureResources = activeShader->texture_resources(passNum);
                for (size_t i = 0; i < textureResources.size(); ++i) {
                    auto &buffer = context.geometry_buffer(textureResources[i]);
                    auto *view = &buffer.shader_resource_view();
                    gfx_->context().PSSetShaderResources(i, 1u, &view);
                }

                activeShader->bind(gfx_, passNum);

                GetSamplerState({Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}).BindPS(gfx_, 0);

                mScreenQuadMesh->bind(gfx_);
                gfx_->DrawIndexed(mScreenQuadMesh->triangles.size());

                unbind_all_shader_resources(textureResources.size());
            }
        }
    }
}

void SceneRenderer::RenderModel(nodec_scene::Scene &scene, ShaderBackend *activeShader,
                                const DirectX::XMMATRIX &matrixV, const DirectX::XMMATRIX &matrixP) {
    if (activeShader == nullptr) return;

    using namespace nodec;
    using namespace nodec_scene::components;
    using namespace nodec_rendering;
    using namespace nodec_rendering::components;
    using namespace nodec_rendering::resources;
    using namespace DirectX;

    mModelPropertiesCB.BindVS(gfx_, MODEL_PROPERTIES_CB_SLOT);
    mModelPropertiesCB.BindPS(gfx_, MODEL_PROPERTIES_CB_SLOT);

    UINT used_slot_max = 0;

    // TODO: Extract sub rebderer class.
    scene.registry().view<const Transform, const MeshRenderer>(type_list<NonVisible>{}).each([&](auto entt, const Transform &trfm, const MeshRenderer &meshRenderer) {
        if (meshRenderer.meshes.size() == 0) return;
        if (meshRenderer.meshes.size() != meshRenderer.materials.size()) return;

        // DirectX Math using row-major representation, row-major memory order.
        // nodec using column-major representation, column-major memory order.
        // HLSL using column-major representation, row-major memory order.
        //
        // nodec -> DirectX Math
        //  Mathematically, when a matrix is converted from column-major representation to row-major representation,
        //  it needs to be transposed.
        //  However, memory ordering of nodec and DirectX Math is different.
        //  Therefore, the matrix is automatically transposed when it is assigned to each other.
        XMMATRIX matrixM{trfm.local2world.m};
        auto matrixMInverse = XMMatrixInverse(nullptr, matrixM);

        auto matrixMVP = matrixM * matrixV * matrixP;

        XMStoreFloat4x4(&mModelProperties.matrixM, matrixM);
        XMStoreFloat4x4(&mModelProperties.matrixMInverse, matrixMInverse);
        XMStoreFloat4x4(&mModelProperties.matrixMVP, matrixMVP);

        mModelPropertiesCB.Update(gfx_, &mModelProperties);

        for (int i = 0; i < meshRenderer.meshes.size(); ++i) {
            auto &mesh = meshRenderer.meshes[i];
            auto &material = meshRenderer.materials[i];
            if (!mesh || !material) continue;

            auto *meshBackend = static_cast<MeshBackend *>(mesh.get());
            auto *materialBackend = static_cast<MaterialBackend *>(material.get());
            auto *shaderBackend = static_cast<ShaderBackend *>(material->shader().get());

            if (shaderBackend != activeShader) continue;

            materialBackend->bind_constant_buffer(gfx_, MATERIAL_PROPERTIES_CB_SLOT);
            SetCullMode(materialBackend->cull_mode());

            mTextureConfig.texHasFlag = 0x00;
            used_slot_max = (std::max)(bind_texture_entries(materialBackend->texture_entries(), mTextureConfig.texHasFlag), used_slot_max);
            mTextureConfigCB.Update(gfx_, &mTextureConfig);

            meshBackend->bind(gfx_);
            gfx_->DrawIndexed(meshBackend->triangles.size());
        } // End foreach mesh
    });   // End foreach mesh renderer

    [&]() {
        if (!mQuadMesh) return;

        mBSAlphaBlend.Bind(gfx_);

        scene.registry().view<const Transform, const ImageRenderer>(type_list<NonVisible>{}).each([&](auto entt, const Transform &trfm, const ImageRenderer &renderer) {
            using namespace DirectX;
            using namespace nodec_rendering;

            auto &image = renderer.image;
            auto &material = renderer.material;
            if (!image || !material) return;

            auto *imageBackend = static_cast<TextureBackend *>(image.get());
            auto *materialBackend = static_cast<MaterialBackend *>(material.get());
            auto *shaderBackend = static_cast<ShaderBackend *>(material->shader().get());

            if (shaderBackend != activeShader) return;

            auto savedAlbedo = materialBackend->get_texture_entry("albedo");
            materialBackend->set_texture_entry("albedo", {image, {Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}});

            materialBackend->bind_constant_buffer(gfx_, MATERIAL_PROPERTIES_CB_SLOT);
            SetCullMode(materialBackend->cull_mode());

            mTextureConfig.texHasFlag = 0x00;
            used_slot_max = (std::max)(bind_texture_entries(materialBackend->texture_entries(), mTextureConfig.texHasFlag), used_slot_max);
            mTextureConfigCB.Update(gfx_, &mTextureConfig);

            XMMATRIX matrixM{trfm.local2world.m};
            const auto width = imageBackend->width() / (renderer.pixels_per_unit + std::numeric_limits<float>::epsilon());
            const auto height = imageBackend->height() / (renderer.pixels_per_unit + std::numeric_limits<float>::epsilon());
            matrixM = XMMatrixScaling(width, height, 1.0f) * matrixM;

            // matrixM
            auto matrixMInverse = XMMatrixInverse(nullptr, matrixM);

            // DirectX Math using row-major representation
            // HLSL using column-major representation
            auto matrixMVP = matrixM * matrixV * matrixP;

            XMStoreFloat4x4(&mModelProperties.matrixM, matrixM);
            XMStoreFloat4x4(&mModelProperties.matrixMInverse, matrixMInverse);
            XMStoreFloat4x4(&mModelProperties.matrixMVP, matrixMVP);

            mModelPropertiesCB.Update(gfx_, &mModelProperties);

            mQuadMesh->bind(gfx_);
            gfx_->DrawIndexed(mQuadMesh->triangles.size());

            if (savedAlbedo) {
                materialBackend->set_texture_entry("albedo", *savedAlbedo);
            }
        });

        mBSDefault.Bind(gfx_);
    }();

    [&]() {
        mBSAlphaBlend.Bind(gfx_);
        scene.registry().view<const Transform, const TextRenderer>(type_list<NonVisible>{}).each([&](auto entt, const Transform &trfm, const TextRenderer &renderer) {
            using namespace DirectX;
            using namespace nodec_rendering;

            auto fontBackend = std::static_pointer_cast<FontBackend>(renderer.font);
            if (!fontBackend) return;

            auto materialBackend = std::static_pointer_cast<MaterialBackend>(renderer.material);
            if (!materialBackend) return;

            auto shaderBackend = std::static_pointer_cast<ShaderBackend>(materialBackend->shader());
            if (shaderBackend.get() != activeShader) return;

            auto savedMask = materialBackend->get_texture_entry("mask");
            auto savedAlbedo = materialBackend->get_vector4_property("albedo");

            const auto u32Text = nodec::unicode::utf8to32<std::u32string>(renderer.text);
            const float pixelsPerUnit = renderer.pixels_per_unit;

            float offsetX = 0.0f;
            float offsetY = 0.0f;
            for (const auto &chCode : u32Text) {
                if (chCode == '\n') {
                    offsetY -= renderer.pixel_size / pixelsPerUnit;
                    offsetX = 0.0f;
                    continue;
                }

                const auto &character = mFontCharacterDatabase.Get(fontBackend->GetFace(), renderer.pixel_size, chCode);

                float posX = offsetX + character.bearing.x / pixelsPerUnit;
                float posY = offsetY - (character.size.y - character.bearing.y) / pixelsPerUnit;
                float w = character.size.x / pixelsPerUnit;
                float h = character.size.y / pixelsPerUnit;

                offsetX += (character.advance >> 6) / pixelsPerUnit;

                if (!character.pFontTexture) {
                    continue;
                }

                materialBackend->set_texture_entry("mask", {character.pFontTexture, {Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}});
                materialBackend->set_vector4_property("albedo", renderer.color);

                materialBackend->bind_constant_buffer(gfx_, 3);
                SetCullMode(materialBackend->cull_mode());

                mTextureConfig.texHasFlag = 0x00;
                used_slot_max = std::max(bind_texture_entries(materialBackend->texture_entries(), mTextureConfig.texHasFlag), used_slot_max);
                mTextureConfigCB.Update(gfx_, &mTextureConfig);

                XMMATRIX matrixM{trfm.local2world.m};
                matrixM = XMMatrixScaling(w / 2, h / 2, 1.0f) * XMMatrixTranslation(posX + w / 2, posY + h / 2, 0.0f) * matrixM;

                // matrixM
                auto matrixMInverse = XMMatrixInverse(nullptr, matrixM);

                // DirectX Math using row-major representation
                // HLSL using column-major representation
                auto matrixMVP = matrixM * matrixV * matrixP;

                XMStoreFloat4x4(&mModelProperties.matrixM, matrixM);
                XMStoreFloat4x4(&mModelProperties.matrixMInverse, matrixMInverse);
                XMStoreFloat4x4(&mModelProperties.matrixMVP, matrixMVP);

                mModelPropertiesCB.Update(gfx_, &mModelProperties);
                mScreenQuadMesh->bind(gfx_);
                gfx_->DrawIndexed(mScreenQuadMesh->triangles.size());
            }

            if (savedMask) {
                materialBackend->set_texture_entry("mask", *savedMask);
            }

            if (savedAlbedo) {
                materialBackend->set_vector4_property("albedo", *savedAlbedo);
            }
        });

        mBSDefault.Bind(gfx_);
    }();

    unbind_all_shader_resources(used_slot_max);
}

UINT SceneRenderer::bind_texture_entries(const std::vector<TextureEntry> &textureEntries, uint32_t &texHasFlag) {
    UINT slot = 0;

    for (auto &entry : textureEntries) {
        if (!entry.texture) {
            // texture not setted.
            // skip bind texture,
            // but bind sampler because the The Pixel Shader unit expects a Sampler to be set at Slot 0.
            auto &defaultSamplerState = GetSamplerState({});
            defaultSamplerState.BindPS(gfx_, slot);
            defaultSamplerState.BindVS(gfx_, slot);

            ++slot;
            continue;
        }

        auto *textureBackend = static_cast<TextureBackend *>(entry.texture.get());
        {
            auto *view = &textureBackend->shader_resource_view();
            gfx_->context().VSSetShaderResources(slot, 1u, &view);
            gfx_->context().PSSetShaderResources(slot, 1u, &view);
        }

        auto &samplerState = GetSamplerState(entry.sampler);
        samplerState.BindVS(gfx_, slot);
        samplerState.BindPS(gfx_, slot);

        texHasFlag |= 0x01 << slot;

        ++slot;
    }
    return slot;
}