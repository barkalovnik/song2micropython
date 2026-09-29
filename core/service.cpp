#include "service.h"

#include "errors.h"
#include "events.h"
#include "exporter_registry.h"
#include "pipeline.h"

namespace song2notes {

void ConversionService::validate(const ConversionSettings& settings) {
    if (settings.end.has_value() && *settings.end <= settings.start) {
        throw ConversionError("Конец фрагмента должен быть больше его начала.");
    }
    if (settings.speed <= 0) {
        throw ConversionError("Скорость должна быть положительной.");
    }
}

std::vector<TrackInfo> ConversionService::inspect(const std::wstring& path) const {
    return loaders_.create(path).list_tracks(path);
}

ConversionResult ConversionService::convert(const std::wstring& path, const ConversionSettings& settings,
                                             const std::function<void(const std::wstring&)>& progress) const {
    validate(settings);
    const SegmentLoader& loader = loaders_.create(path);            // Factory -> Strategy
    std::vector<NoteSegment> segments = loader.load(path, settings, progress);  // Strategy
    progress(L"Обработка нот...");
    segments = build_pipeline(settings).run(std::move(segments));   // Pipeline
    if (segments.empty()) {
        throw EmptyMelodyError(
            "Не найдено ни одной ноты. Попробуйте другой фрагмент, дорожку или файл.");
    }
    std::vector<NoteEvent> events = segments_to_events(std::move(segments), settings.speed);
    bool truncated = static_cast<int>(events.size()) > settings.max_notes;
    if (truncated) events.resize(static_cast<size_t>(settings.max_notes));

    ConversionResult result;
    result.events = std::move(events);
    result.truncated = truncated;
    return result;
}

std::string ConversionService::render(const std::vector<NoteEvent>& events, const ConversionSettings& settings,
                                       const std::string& source_name) const {
    return get_exporter(settings.exporter_key).export_text(events, settings, source_name);  // Strategy
}

}  // namespace song2notes
