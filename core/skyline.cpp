#include "skyline.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace song2notes {

namespace {
constexpr double kEps = 1e-9;

struct Span {
    double start;
    double end;
    int pitch;
    size_t note_id;
};

struct ActiveNote {
    size_t id;
    RawNote note;
};
}  // namespace

std::vector<NoteSegment> skyline(const std::vector<RawNote>& notes_in) {
    if (notes_in.empty()) return {};

    std::vector<RawNote> notes = notes_in;
    std::sort(notes.begin(), notes.end(), [](const RawNote& a, const RawNote& b) {
        if (a.start != b.start) return a.start < b.start;
        return a.end < b.end;
    });

    std::set<double> bound_set;
    for (const auto& n : notes) {
        bound_set.insert(n.start);
        bound_set.insert(n.end);
    }
    std::vector<double> bounds(bound_set.begin(), bound_set.end());

    std::vector<Span> spans;
    std::vector<ActiveNote> active;
    size_t idx = 0;

    for (size_t bi = 0; bi + 1 < bounds.size(); ++bi) {
        double t0 = bounds[bi];
        double t1 = bounds[bi + 1];

        while (idx < notes.size() && notes[idx].start <= t0 + kEps) {
            active.push_back({idx, notes[idx]});
            ++idx;
        }
        active.erase(std::remove_if(active.begin(), active.end(),
                                     [&](const ActiveNote& a) { return a.note.end <= t0 + kEps; }),
                     active.end());
        if (active.empty()) continue;

        const ActiveNote& best = *std::max_element(
            active.begin(), active.end(),
            [](const ActiveNote& a, const ActiveNote& b) { return a.note.pitch < b.note.pitch; });

        if (!spans.empty() && spans.back().note_id == best.id && std::abs(spans.back().end - t0) < kEps) {
            spans.back().end = t1;  // продолжение той же ноты
        } else {
            spans.push_back({t0, t1, best.note.pitch, best.id});
        }
    }

    std::vector<NoteSegment> result;
    result.reserve(spans.size());
    for (const auto& s : spans) result.push_back({s.start, s.end, s.pitch});
    return result;
}

}  // namespace song2notes
