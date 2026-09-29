// segment_loader.h - интерфейс загрузчика формата.
// [Pattern: Strategy | Role: Strategy] Сейчас есть только MidiLoader, но
// сервис работает через эту абстракцию, так что добавить новый формат
// (например, снова MP3) можно новым классом, не трогая остальной код.
#pragma once
#include <functional>
#include <string>
#include <vector>

#include "domain.h"

namespace song2notes {

class SegmentLoader {
public:
    virtual ~SegmentLoader() = default;

    virtual std::vector<NoteSegment> load(
        const std::wstring& path, const ConversionSettings& settings,
        const std::function<void(const std::wstring&)>& progress) const = 0;

    virtual std::vector<TrackInfo> list_tracks(const std::wstring& path) const = 0;
};

}  // namespace song2notes
