#ifndef NODEC_GAME_EDITOR__EDITOR_WINDOWS__LOG_WINDOW_HPP_
#define NODEC_GAME_EDITOR__EDITOR_WINDOWS__LOG_WINDOW_HPP_

#include <iomanip>
#include <mutex>
#include <queue>
#include <vector>

#include <imgui.h>

#include <imessentials/window.hpp>
#include <nodec/formatter.hpp>
#include <nodec/logging/formatters/simple_formatter.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec/signals/scoped_block.hpp>
#include <nodec/signals/signal.hpp>
#include <nodec/vector2.hpp>

class LogWindow final : public imessentials::BaseWindow {
    static constexpr int MAX_RECORDS = 100;
    static constexpr float MIN_DETAILS_HEIGHT = 50.0f;
    static constexpr float SPLITTER_THICKNESS = 4.0f;

    struct RecordEntry {
        RecordEntry(std::uint32_t id, nodec::logging::Level level, std::string &&formatted_text)
            : id(id), level(level), formatted_text(std::move(formatted_text)) {
        }

        std::uint32_t id;
        nodec::logging::Level level;
        std::string formatted_text;
    };

public:
    LogWindow()
        : BaseWindow("Log", nodec::Vector2f(500, 400)),
          start_time_(std::chrono::system_clock::now()),
          auto_scroll_(true),
          details_height_(100.0f) {}

    void on_gui() override {
        using namespace nodec;
        using namespace nodec::logging;

        {
            std::lock_guard<std::mutex> lock(pending_records_mutex_);

            for (const auto &record : pending_records_) {
                record_entries_.push_back(record);
            }
            pending_records_.clear();
        }

        while (MAX_RECORDS < record_entries_.size()) {
            record_entries_.pop_front();
        }

        // Options menu
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &auto_scroll_);
            ImGui::EndPopup();
        }

        if (ImGui::Button("Options")) {
            ImGui::OpenPopup("Options");
        }
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");

        ImGui::SameLine();
        filter_.Draw("Filter", -100.0f);

        ImGui::Separator();

        // Calculate available height for entries (excluding details, splitter, and toolbar)
        const float available_height = ImGui::GetContentRegionAvail().y;
        const float entries_height = available_height - details_height_ - SPLITTER_THICKNESS;

        ImGui::BeginChild("Entries", ImVec2(0, entries_height), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        if (clear) record_entries_.clear();

        auto draw_record_entry = [&](RecordEntry &entry) {
            ImVec4 color;
            switch (entry.level) {
            case Level::Fatal:
            case Level::Error:
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                break;
            case Level::Warn:
                color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                break;
            default:
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);

            if (ImGui::Selectable(static_cast<std::string>(Formatter() << entry.formatted_text << "##" << entry.id).c_str(),
                                  entry.id == selected_id_, ImGuiSelectableFlags_None, ImVec2(0, 30))) {
                selected_id_ = entry.id;
                selected_entry_text_ = entry.formatted_text;
            }
            ImGui::PopStyleColor();
        };

        if (filter_.IsActive()) {
            for (auto &entry : record_entries_) {
                if (!filter_.PassFilter(entry.formatted_text.c_str())) {
                    continue;
                }

                draw_record_entry(entry);
            }
        } else {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(record_entries_.size()));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                    draw_record_entry(record_entries_[i]);
                }
            }
            clipper.End();
        }

        if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        // Horizontal splitter
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.7f, 0.7f));
        ImGui::Button("##Splitter", ImVec2(-1, SPLITTER_THICKNESS));
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        if (ImGui::IsItemActive()) {
            const float delta = ImGui::GetIO().MouseDelta.y;
            details_height_ -= delta;
            // Clamp details height
            const float max_details_height = available_height - MIN_DETAILS_HEIGHT - SPLITTER_THICKNESS;
            if (details_height_ < MIN_DETAILS_HEIGHT) details_height_ = MIN_DETAILS_HEIGHT;
            if (details_height_ > max_details_height) details_height_ = max_details_height;
        }

        // Details panel with selectable text (InputTextMultiline in ReadOnly mode)
        ImGui::BeginChild("Details", ImVec2(0, 0), false);
        ImGui::InputTextMultiline(
            "##DetailsText",
            const_cast<char *>(selected_entry_text_.c_str()),
            selected_entry_text_.size() + 1,
            ImVec2(-1, -1),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::EndChild();
    }

private:
    const std::chrono::system_clock::time_point start_time_;
    std::mutex pending_records_mutex_;
    std::vector<RecordEntry> pending_records_;

    nodec::logging::HandlerConnection logging_handler_conn_ = nodec::logging::get_logger()->add_handler(
        [&](const nodec::logging::LogRecord &record) {
            // This section may be called in another thread.
            using namespace nodec::logging;
            using namespace nodec::signals;
            using namespace nodec;
            using namespace std::chrono;

            ScopedBlock<HandlerConnection> scoped_block{logging_handler_conn_};

            {
                std::lock_guard<std::mutex> lock(pending_records_mutex_);
                pending_records_.push_back({next_id_++, record.level,
                                            Formatter() << std::left << std::setw(4)
                                                        << duration_cast<milliseconds>(record.time - start_time_).count() << " "
                                                        << formatters::SimpleFormatter()(record)});
            }
        });

    std::deque<RecordEntry> record_entries_;
    std::uint32_t next_id_{0};
    std::uint32_t selected_id_{0xFFFFFFFF};
    std::string selected_entry_text_;
    bool auto_scroll_;
    float details_height_;
    ImGuiTextFilter filter_;
};

#endif