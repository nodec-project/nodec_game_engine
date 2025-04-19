
#include <thread>

#include <nodec/logging/logger.hpp>

#include <uwebsockets/App.h>

class InspectorServer {
public:
    InspectorServer() {
        thread_ = std::thread([this]() {
            uWS::App()
                .get("/entities/id/:id", [](auto *res, auto *req) {
                    res->writeHeader("Content-Type", "application/json");
                    res->end("{\"id\": 1}");
                })
                .listen(8080, [this](auto *socket) {
                    if (socket) {
                        listen_socket_ = socket;
                    } else {
                    }
                })
                .run();
        });
    }

    ~InspectorServer() {
        if (listen_socket_) {
            us_listen_socket_close(0, listen_socket_);
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
private:
    us_listen_socket_t *listen_socket_{nullptr};
    std::thread thread_;
};