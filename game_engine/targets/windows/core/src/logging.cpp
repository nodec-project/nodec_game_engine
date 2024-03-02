#include "logging.hpp"

#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>

#include <nodec/formatter.hpp>
#include <nodec/logging/formatters/simple_formatter.hpp>
#include <nodec/logging/logging.hpp>

namespace {

class FileHandler {
public:
    FileHandler() {
        start_time_ = std::chrono::system_clock::now();
        log_file_.open("output.log", std::ios::binary);
        if (!log_file_) {
            throw std::runtime_error(nodec::ErrorFormatter<std::runtime_error>(__FILE__, __LINE__)
                                     << "Logging initialize failed. cannot open the log file.");
        }
    }

    void operator()(const nodec::logging::LogRecord &record) {
        if (record.level < nodec::logging::Level::Info) return;
        std::lock_guard<std::mutex> lock(io_mutex_);
        using namespace nodec::logging::formatters;
        using namespace std::chrono;
        try {
            log_file_ <<std::left << std::setw(4) << duration_cast<milliseconds>(record.time - start_time_).count() << " "
                      << formatter_(record) << "\n";
            log_file_.flush();
        } catch (...) {
            // It is not possible to log exceptions that occur during log writing, so ignore them.
        }
    }

private:
    std::mutex io_mutex_;
    std::ofstream log_file_;
    nodec::logging::formatters::SimpleFormatter formatter_;
    std::chrono::system_clock::time_point start_time_;
};

} // namespace

void init_logging(nodec::logging::Level level) {
    static bool initialized = false;
    assert(!initialized);

    using namespace nodec::logging;

    auto logger = get_logger();

    logger->set_level(level);
    logger->add_handler(std::make_shared<FileHandler>());

    logger->info(__FILE__, __LINE__) << "Logging successfully initialized.\n"
                                     << "log_level: " << level;
}