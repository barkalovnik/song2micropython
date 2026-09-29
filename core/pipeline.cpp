#include "pipeline.h"

#include <algorithm>
#include <array>

#include "notes.h"

namespace song2notes {

std::vector<NoteSegment> CropStep::process(const std::vector<NoteSegment>& segments) const {
    std::vector<NoteSegment> out;
    for (const auto& s : segments) {
        if (s.end <= start_ || (end_.has_value() && s.start >= *end_)) continue;
        double new_start = std::max(s.start, start_);
        double new_end = end_.has_value() ? std::min(s.end, *end_) : s.end;
        out.push_back({new_start - start_, new_end - start_, s.pitch});
    }
    return out;
}

std::vector<NoteSegment> MinDurationStep::process(const std::vector<NoteSegment>& segments) const {
    std::vector<NoteSegment> out;
    for (const auto& s : segments) {
        if (s.duration() >= min_seconds_) out.push_back(s);
    }
    return out;
}

int FitRangeStep::fold(int pitch) {
    while (pitch < notes::PITCH_LOW) pitch += 12;
    while (pitch > notes::PITCH_HIGH) pitch -= 12;
    return pitch;
}

int FitRangeStep::best_shift(const std::vector<NoteSegment>& segments) {
    int best = 0;
    int best_count = -1;
    for (int shift = -48; shift <= 48; shift += 12) {
        int count = 0;
        for (const auto& s : segments) {
            if (s.pitch + shift >= notes::PITCH_LOW && s.pitch + shift <= notes::PITCH_HIGH) ++count;
        }
        // При равном числе попавших нот предпочитаем меньший по модулю сдвиг
        // (ближе к исходной октаве) - как и в Python-версии.
        if (count > best_count || (count == best_count && std::abs(shift) < std::abs(best))) {
            best_count = count;
            best = shift;
        }
    }
    return best;
}

std::vector<NoteSegment> FitRangeStep::process(const std::vector<NoteSegment>& segments) const {
    if (segments.empty()) return {};
    int shift = transpose_.has_value() ? *transpose_ : best_shift(segments);
    std::vector<NoteSegment> out;
    out.reserve(segments.size());
    for (const auto& s : segments) out.push_back({s.start, s.end, fold(s.pitch + shift)});
    return out;
}

Pipeline build_pipeline(const ConversionSettings& settings) {
    std::vector<std::unique_ptr<ProcessingStep>> steps;
    steps.push_back(std::make_unique<CropStep>(settings.start, settings.end));
    steps.push_back(std::make_unique<MinDurationStep>(settings.min_note_ms));
    steps.push_back(std::make_unique<FitRangeStep>(settings.transpose));
    return Pipeline(std::move(steps));
}

}  // namespace song2notes
