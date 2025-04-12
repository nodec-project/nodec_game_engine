#include <rendering/scene_renderer.hpp>
#include <graphics/graphics.hpp>
#include <rendering/mesh_backend.hpp>

#include <DirectXMath.h>
#include <d3dcompiler.h>

// バウンディングボックス描画用の頂点シェーダー
static const char* g_bbox_vs_code = R"(
cbuffer Constants : register(b0)
{
    matrix ViewProj;
    matrix World;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Pos = mul(mul(float4(input.Pos, 1.0), World), ViewProj);
    return output;
}
)";

// バウンディングボックス描画用のピクセルシェーダー
static const char* g_bbox_ps_code = R"(
float4 main() : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
)";

struct ConstantsBuffer
{
    DirectX::XMMATRIX ViewProj;
    DirectX::XMMATRIX World;
    DirectX::XMFLOAT4 Padding[4]; // 64バイト分のパディングを追加
};

OcclusionCullingSystem::OcclusionCullingSystem(Graphics &gfx)
    : gfx_(gfx), current_frame_index_(0)
{
    // 定数バッファの作成
    D3D11_BUFFER_DESC cbd = {};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.MiscFlags = 0;
    cbd.ByteWidth = sizeof(ConstantsBuffer);
    cbd.StructureByteStride = 0;
    gfx_.device().CreateBuffer(&cbd, nullptr, &constants_buffer_);

    // 深度ステンシルステートの作成
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    gfx_.device().CreateDepthStencilState(&dsd, &occlusion_depth_state_);

    // ラスタライザステートの作成
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    gfx_.device().CreateRasterizerState(&rd, &occlusion_raster_state_);

    // シェーダーのコンパイル
    Microsoft::WRL::ComPtr<ID3DBlob> vs_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> ps_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob;

    HRESULT hr = D3DCompile(g_bbox_vs_code, strlen(g_bbox_vs_code), nullptr, nullptr, nullptr, 
                           "main", "vs_5_0", 0, 0, &vs_blob, &error_blob);
    if (FAILED(hr)) {
        // エラー処理（実際の実装ではログ出力など）
        const char* error_msg = error_blob ? static_cast<const char*>(error_blob->GetBufferPointer()) : "Unknown error";
        // エラーメッセージを出力
    }

    hr = D3DCompile(g_bbox_ps_code, strlen(g_bbox_ps_code), nullptr, nullptr, nullptr, 
                   "main", "ps_5_0", 0, 0, &ps_blob, &error_blob);
    if (FAILED(hr)) {
        // エラー処理
        const char* error_msg = error_blob ? static_cast<const char*>(error_blob->GetBufferPointer()) : "Unknown error";
        // エラーメッセージを出力
    }

    // シェーダーオブジェクトの作成
    gfx_.device().CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &bbox_vs_);
    gfx_.device().CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &bbox_ps_);
}

OcclusionCullingSystem::~OcclusionCullingSystem()
{
    // 特に何もする必要はない（ComPtrが自動的にリソースを解放）
}

void OcclusionCullingSystem::register_object(uintptr_t object_id, const DirectX::XMMATRIX &local_to_world, std::shared_ptr<MeshBackend> mesh)
{
    auto it = query_instances_.find(object_id);
    
    if (it == query_instances_.end()) {
        // 新しいオブジェクトの場合
        OcclusionQueryInstance instance;
        instance.local_to_world = local_to_world;
        instance.mesh = mesh;
        instance.visible = true;  // 最初は可視と仮定
        instance.query_issued = false;
        instance.last_visible_frame = current_frame_index_;

        // オクルージョンクエリの作成
        D3D11_QUERY_DESC query_desc = {};
        query_desc.Query = D3D11_QUERY_OCCLUSION;
        query_desc.MiscFlags = 0;
        gfx_.device().CreateQuery(&query_desc, &instance.query);

        query_instances_[object_id] = std::move(instance);
    } else {
        // 既存オブジェクトの更新
        it->second.local_to_world = local_to_world;
        // メッシュは変更されることが少ないので、必要な場合のみ更新
        if (it->second.mesh != mesh) {
            it->second.mesh = mesh;
        }
    }
}

void OcclusionCullingSystem::begin_occlusion_test(const DirectX::XMMATRIX &view_proj)
{
    // 前のフレームのクエリ結果を処理 - タイムアウト付きで強制的に結果を取得
    const UINT time_out = 1000; // 1秒のタイムアウト
    for (auto &pair : query_instances_) {
        auto &instance = pair.second;
        
        if (instance.query_issued) {
            // クエリ結果が利用可能かチェック - タイムアウトを指定
            UINT64 pixel_count = 0;
            HRESULT hr = gfx_.context().GetData(instance.query.Get(), &pixel_count, sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH);
            
            if (hr == S_OK) {
                // 結果が利用可能
                instance.visible = (pixel_count > 0);
                instance.query_issued = false;
                
                if (instance.visible) {
                    instance.last_visible_frame = current_frame_index_;
                }
            }
            // S_FALSEの場合、結果はまだ利用できないので、前のフレームの可視性を維持
            // ただし問題を避けるためにクエリを再発行しない
            else {
                // 以前のクエリがまだ完了していないためスキップ
                continue;
            }
        }
    }
    
    // オクルージョンテスト用の状態設定
    gfx_.context().OMSetDepthStencilState(occlusion_depth_state_.Get(), 0);
    gfx_.context().RSSetState(occlusion_raster_state_.Get());
    
    // シェーダー設定
    gfx_.context().VSSetShader(bbox_vs_.Get(), nullptr, 0);
    gfx_.context().PSSetShader(bbox_ps_.Get(), nullptr, 0);
    
    // 定数バッファ準備
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    HRESULT map_hr = gfx_.context().Map(constants_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    if (FAILED(map_hr)) {
        return; // マッピングに失敗したら終了
    }
    
    ConstantsBuffer* constants = static_cast<ConstantsBuffer*>(mapped_resource.pData);
    constants->ViewProj = view_proj;
    gfx_.context().Unmap(constants_buffer_.Get(), 0);
    
    // 定数バッファをバインド
    gfx_.context().VSSetConstantBuffers(0, 1, constants_buffer_.GetAddressOf());

    // 各オブジェクトに対してオクルージョンクエリを発行
    for (auto &pair : query_instances_) {
        auto &instance = pair.second;
        
        // クエリがまだ処理中の場合はスキップ
        if (instance.query_issued) {
            continue;
        }
        
        // 前のフレームで可視だったか、しばらく見えていないオブジェクトのみクエリを実行
        if (instance.visible || (current_frame_index_ - instance.last_visible_frame) > VISIBILITY_THRESHOLD) {
            try {
                // 定数バッファの更新
                HRESULT hr = gfx_.context().Map(constants_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
                if (FAILED(hr)) {
                    continue; // マッピングに失敗したらこのオブジェクトをスキップ
                }
                
                constants = static_cast<ConstantsBuffer*>(mapped_resource.pData);
                constants->World = instance.local_to_world;
                gfx_.context().Unmap(constants_buffer_.Get(), 0);
                
                // オクルージョンクエリ開始
                gfx_.context().Begin(instance.query.Get());
                
                // バウンディングボックスの描画
                if (instance.mesh) {
                    instance.mesh->bind(&gfx_);
                    gfx_.DrawIndexed(static_cast<UINT>(instance.mesh->triangles.size()));
                }
                
                // オクルージョンクエリ終了
                gfx_.context().End(instance.query.Get());
                instance.query_issued = true;
            }
            catch (...) {
                // エラーが発生した場合、このオブジェクトをスキップ
                continue;
            }
        }
    }
}

bool OcclusionCullingSystem::is_visible(uintptr_t object_id)
{
    auto it = query_instances_.find(object_id);
    if (it != query_instances_.end()) {
        return it->second.visible;
    }
    return true;  // 登録されていないオブジェクトはデフォルトで可視
}

void OcclusionCullingSystem::end_frame()
{
    current_frame_index_++;
}