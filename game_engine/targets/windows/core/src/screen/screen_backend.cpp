#include <screen/screen_backend.hpp>

void ScreenBackend::set_title(const std::string &title) {
    title_ = title;
    if (window_) {
        try {
            window_->set_title(title_);
        } catch (const std::exception &e) {
            nodec::logging::error(__FILE__, __LINE__)
                << "[ScreenBackend] >>> Exception has been occurred while Window::set_title().\n"
                << "detail: " << e.what();
        } catch (...) {
            nodec::logging::error(__FILE__, __LINE__)
                << "[ScreenBackend] >>> Unknown exception has been occurred while Window::set_title().";
        }
    }
}