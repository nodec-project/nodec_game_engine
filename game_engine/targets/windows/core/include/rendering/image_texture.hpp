#ifndef NODEC_GAME_ENGINE__RENDERING__IMAGE_TEXTURE_HPP_
#define NODEC_GAME_ENGINE__RENDERING__IMAGE_TEXTURE_HPP_

#include <DirectXTex.h>

#include <nodec/unicode.hpp>

#include "texture_backend.hpp"

class ImageTexture : public TextureBackend {
public:
    ImageTexture(Graphics *gfx, const std::string &path) {
        using namespace DirectX;

        enum class ImageType {
            TGA,
            WIC,
            HDR
        };

        ImageType image_type = ImageType::WIC;

        auto extention_pos = path.find_last_of('.');
        if (extention_pos != std::string::npos) {
            auto extention = path.substr(extention_pos);

            if (extention == ".tga" || extention == ".TGA") {
                image_type = ImageType::TGA;
            }

            if (extention == ".hdr") {
                image_type = ImageType::HDR;
            }
        }

        std::wstring path_wide = nodec::unicode::utf8to16<std::wstring>(path);

        ScratchImage image;
        switch (image_type) {
        case ImageType::TGA:
            ThrowIfFailedGfx(
                LoadFromTGAFile(path_wide.c_str(), &metadata_, image),
                gfx, __FILE__, __LINE__);
            break;
        case ImageType::HDR:
            ThrowIfFailedGfx(
                LoadFromHDRFile(path_wide.c_str(), &metadata_, image),
                gfx, __FILE__, __LINE__);
            break;
        default:
        case ImageType::WIC:
            ThrowIfFailedGfx(
                LoadFromWICFile(path_wide.c_str(), WIC_FLAGS::WIC_FLAGS_NONE, &metadata_, image),
                gfx, __FILE__, __LINE__);
            break;
        }

        // create the resource view on the texture
        ThrowIfFailedGfx(
            CreateShaderResourceView(&gfx->device(), image.GetImages(), image.GetImageCount(), metadata_, &shader_resource_view_),
            gfx, __FILE__, __LINE__);

        initialize(shader_resource_view_.Get(), metadata_.width, metadata_.height);
    }

private:
    DirectX::TexMetadata metadata_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_;
};

#endif