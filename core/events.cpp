#include "events.h"

#include <algorithm>

namespace song2notes {

namespace {
constexpr double kMinRestSeconds = 0.02;  // щели короче 20 мс не считаем паузой
}

std::vector<NoteEvent> segments_to_events(std::vector<NoteSegment> segments, double speed) {
    std::sort(segments.begin(), segments.end(),
              [](const NoteSegment& a, const NoteSegment& b) { return a.start < b.start; });

    std::vector<NoteEvent> events;
    double cursor = 0.0;
    for (const auto& seg : segments) {
        double note_start = seg.start;
        double gap = seg.start - cursor;
        if (gap > kMinRestSeconds) {
            events.push_back({std::nullopt, static_cast<int>(gap * 1000 / speed)});
        } else {
            note_start = cursor;
        }
        if (seg.end <= note_start) continue;
        int ms = std::max(1, static_cast<int>((seg.end - note_start) * 1000 / speed));
        events.push_back({seg.pitch, ms});
        cursor = seg.end;
    }
    return events;
}

}  // namespace song2notes
