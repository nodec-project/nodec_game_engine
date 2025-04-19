#include <engine.hpp>

// // Windows.hをuWebSocketsより前にインクルードして競合を防ぐ
// #define NOMINMAX
// #define WIN32_LEAN_AND_MEAN
// #include <Windows.h>

// #include <thread>
// #include <uwebsockets/App.h>

#include "animation/animation.hpp"

// // WebSocketコネクション用の空構造体 (voidの代わり)
// struct WsPerSocketData {
//     // 必要に応じてセッションデータをここに追加できます
// };

Engine::Engine(nodec_application::impl::ApplicationImpl &app)
    : logger_(nodec::logging::get_logger("engine")) {
    using namespace nodec_world;
    using namespace nodec_world::impl;
    using namespace nodec_screen;
    using namespace nodec_input;
    using namespace nodec_resources;
    using namespace nodec_scene_serialization;
    using namespace nodec_physics::systems;

    logger_->info(__FILE__, __LINE__) << "Created!";

    // // --- uWebSockets テストコード (WebSocket + REST API) ---
    // logger_->info(__FILE__, __LINE__) << "Starting uWebSockets server on port 8080...";

    // // 別スレッドでWebSocket & REST APIサーバーを起動
    // websocket_thread_ = std::thread([this]() {
    //     uWS::App()
    //         // REST API エンドポイント
    //         .get("/api/status", [this](auto *res, auto *req) {
    //             logger_->info(__FILE__, __LINE__) << "REST API: GET /api/status called";
                
    //             // JSONレスポンスを作成
    //             std::string json = "{\"status\":\"running\",\"engine\":\"Solreno\",\"version\":\"0.1.0\"}";
                
    //             // JSONヘッダーを設定してレスポンスを送信
    //             res->writeHeader("Content-Type", "application/json");
    //             res->end(json);
    //         })
    //         .post("/api/command", [this](auto *res, auto *req) {
    //             logger_->info(__FILE__, __LINE__) << "REST API: POST /api/command called";
                
    //             // リクエストボディを読み込むための準備
    //             std::string buffer;
                
    //             // リクエストボディデータを受信したときの処理
    //             res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
    //                 // データをバッファに追加
    //                 buffer.append(data.data(), data.length());
                    
    //                 // 最後のデータチャンクを受信したら処理を実行
    //                 if (last) {
    //                     logger_->info(__FILE__, __LINE__) << "Command received: " << buffer;
                        
    //                     // コマンド処理の例（実際の実装はここに追加）
    //                     std::string response = "{\"result\":\"success\",\"message\":\"Command processed\"}";
                        
    //                     // レスポンスの送信
    //                     res->writeHeader("Content-Type", "application/json");
    //                     res->end(response);
    //                 }
    //             });
                
    //             // データ受信中にエラーが発生した場合
    //             res->onAborted([]() {
    //                 // リクエストが中断された場合の処理
    //             });
    //         })
    //         .get("/api/info", [this](auto *res, auto *req) {
    //             logger_->info(__FILE__, __LINE__) << "REST API: GET /api/info called";
                
    //             // クエリパラメータの取得例
    //             std::string_view query = req->getQuery();
    //             logger_->info(__FILE__, __LINE__) << "Query parameters: " << query;
                
    //             // 簡易的なエンジン情報JSONを作成
    //             std::string json = "{\"name\":\"Solreno Engine\",\"description\":\"Game engine with WebSocket and REST API support\"}";
                
    //             // レスポンスを送信
    //             res->writeHeader("Content-Type", "application/json");
    //             res->end(json);
    //         })
    //         // WebSocket ハンドラ（既存のコード）
    //         .ws<WsPerSocketData>("/*", {
    //             // 接続イベント
    //             .open = [this](auto *ws) {
    //                 logger_->info(__FILE__, __LINE__) << "WebSocket client connected";

    //                 // 接続時にウェルカムメッセージを送信
    //                 ws->send("Welcome to Solreno Engine WebSocket Server", uWS::OpCode::TEXT); },

    //             // メッセージ受信イベント
    //             .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
    //                 logger_->info(__FILE__, __LINE__) << "WebSocket message received: " << message;

    //                 // エコーバック
    //                 std::string response = "Echo: ";
    //                 response += message;
    //                 ws->send(response, uWS::OpCode::TEXT); },
    //             // 切断イベント
    //             .close = [this](auto *ws, int code, std::string_view message) { logger_->info(__FILE__, __LINE__) << "WebSocket client disconnected: " << code; },
    //         })
    //         .listen(8080, [this](auto *listen_socket) {
    //             if (listen_socket) {
    //                 logger_->info(__FILE__, __LINE__) << "WebSocket and REST API server listening on port 8080";
    //             } else {
    //                 logger_->error(__FILE__, __LINE__) << "Failed to start server";
    //             }
    //         })
    //         .run(); // イベントループを実行
    // });

    // --- 通常のエンジン初期化処理 ---
    imgui_manager_.reset(new ImguiManager);

    font_library_.reset(new FontLibrary);

    // --- screen ---
    screen_.reset(new ScreenBackend());

    // --- world ---
    world_.reset(new WorldImpl());

    // --- input ---
    input_devices_.reset(new nodec_input::InputDevices());

    keyboard_device_system_ = &input_devices_->emplace_device_system<KeyboardDeviceSystem>();
    mouse_device_system_ = &input_devices_->emplace_device_system<MouseDeviceSystem>();

    // --- resources ---
    resources_.reset(new ResourcesBackend());

    resources_->setup_on_boot();

    // --- scene serialization ---
    scene_serialization_.reset(new SceneSerialization());
    scene_serialization_backend_.reset(new SceneSerializationBackend(&resources_->registry(), *scene_serialization_));
    entity_loader_.reset(new nodec_scene_serialization::impl::EntityLoaderImpl(*scene_serialization_, world_->scene(), resources_->registry()));

    // --- others ---
    physics_system_.reset(new PhysicsSystemBackend(*world_));

    visibility_system_.reset(new nodec_rendering::systems::VisibilitySystem(world_->scene()));
    prefab_load_system_.reset(new nodec_scene_serialization::systems::PrefabLoadSystem(world_->scene(), *entity_loader_));

    animation_component_registry_.reset(new nodec_animation::ComponentRegistry());
    setup_animation_component_registry(*animation_component_registry_);
    animator_system_.reset(new nodec_animation::systems::AnimatorSystem(*animation_component_registry_));

    world_->stepped().connect([=](nodec_world::World &world) {
        on_stepped(world);
    });

    // inspector_server_.reset(new InspectorServer());

    // --- Export the services to application.
    app.add_service<Screen>(screen_);
    app.add_service<World>(world_);
    app.add_service<InputDevices>(input_devices_);
    app.add_service<Resources>(resources_);
    app.add_service<SceneSerialization>(scene_serialization_);
    app.add_service<EntityLoader>(entity_loader_);
    app.add_service<PhysicsSystem>(physics_system_);
    app.add_service<nodec_animation::ComponentRegistry>(animation_component_registry_);
}

Engine::~Engine() {
    logger_->info(__FILE__, __LINE__) << "Destroyed!";

    // // WebSocketスレッドの終了処理
    // logger_->info(__FILE__, __LINE__) << "Stopping WebSocket server...";

    // // uWebSocketsのイベントループを停止する方法は限られているため、
    // // スレッド自体を強制終了します
    // if (websocket_thread_.joinable()) {
    //     // 警告: 通常、スレッドの強制終了は推奨されませんが、
    //     // uWebSocketsのループを正常に終了させるためのAPIが限られているため
    //     // この方法を使用します。実際のプロダクションコードでは、より適切な
    //     // 終了メカニズムを実装するべきです。
    //     TerminateThread(websocket_thread_.native_handle(), 0);
    //     websocket_thread_.join();
    //     logger_->info(__FILE__, __LINE__) << "WebSocket server stopped";
    // }

    // TODO: Consider to unload all modules before backends.

    // unload all scene entities.
    world_->scene().registry().clear();
}

void Engine::setup() {
    using namespace nodec;

    window_.reset(new Window(
        screen_->size().x, screen_->size().y,
        screen_->resolution().x, screen_->resolution().y,
        unicode::utf8to16<std::wstring>(screen_->title()).c_str(),
        &keyboard_device_system_->device(), &mouse_device_system_->device()));

    screen_->setup(window_.get());

    resources_->setup_on_runtime(window_->graphics(), *font_library_, *scene_serialization_);

    scene_renderer_.reset(new SceneRenderer(world_->scene(), window_->graphics(), resources_->registry()));

    audio_platform_.reset(new AudioPlatform());

    scene_audio_system_.reset(new SceneAudioSystem(*audio_platform_, world_->scene().registry()));

    scene_rendering_context_.reset(new SceneRenderingContext(window_->graphics().width(), window_->graphics().height(), window_->graphics()));
}

void Engine::on_stepped(nodec_world::World &world) {
    scene_audio_system_->update(world_->scene().registry());
    animator_system_->update(world_->scene().registry(), world.clock().delta_time());
}

void Engine::frame_begin() {
    window_->graphics().begin_frame();
}

void Engine::frame_end() {
    using namespace nodec::entities;
    using namespace nodec_scene::components;

    // Emplacing the entities then update these transforms.
    prefab_load_system_->update();
    entity_loader_->update();

    // Update transform
    {
        auto root = world_->scene().hierarchy_system().root_hierarchy().first;
        while (root != null_entity) {
            nodec_scene::systems::update_transform(world_->scene().registry(), root);
            root = world_->scene().registry().get_component<Hierarchy>(root).next;
        }
    }

    scene_renderer_->render(world_->scene(),
                            window_->graphics().render_target_view(),
                            *scene_rendering_context_);

    window_->graphics().end_frame();
}
