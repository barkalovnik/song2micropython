// domain.h - доменные модели.
// [Pattern: Value Object] структуры данных без поведения, передаются по значению.
#pragma once
#include <optional>
#include <string>
#include <vector>

namespace song2notes {

struct NoteSegment {
    double start = 0.0;  // секунды
    double end = 0.0;    // секунды
    int pitch = 0;       // номер ноты MIDI

    double duration() const { return end - start; }
};

struct NoteEvent {
    std::optional<int> pitch;  // нет значения = пауза
    int duration_ms = 0;
};

struct TrackInfo {
    int index = 0;
    std::wstring name;
    int note_count = 0;
    int mean_pitch = 0;
};

struct ConversionSettings {
    double start = 0.0;
    std::optional<double> end;         // нет значения = до конца файла
    std::optional<int> track;          // нет значения = выбрать автоматически
    double speed = 1.0;
    std::optional<int> transpose;      // нет значения = подобрать автоматически
    int min_note_ms = 60;
    int max_notes = 1500;
    int pin = 26;
    std::string exporter_key = "micropython";
};

struct ConversionResult {
    std::vector<NoteEvent> events;
    bool truncated = false;

    long long total_ms() const {
        long long sum = 0;
        for (const auto& e : events) sum += e.duration_ms;
        return sum;
    }
};

}  // namespace song2notes
