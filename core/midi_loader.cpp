#include "midi_loader.h"

#include <algorithm>

#include "errors.h"
#include "midi_reader.h"
#include "skyline.h"

namespace song2notes {

int MidiLoader::mean_pitch(const RawTrack& track) {
    if (track.notes.empty()) return 0;
    long long sum = 0;
    for (const auto& n : track.notes) sum += n.pitch;
    return static_cast<int>(sum / static_cast<long long>(track.notes.size()));
}

int MidiLoader::auto_select(const std::vector<RawTrack>& tracks, const std::vector<int>& indices) {
    // Больше всего нот среди небасовых дорожек (mean_pitch >= 55), как в Python-версии.
    std::vector<int> non_bass;
    for (int i : indices) {
        if (mean_pitch(tracks[static_cast<size_t>(i)]) >= 55) non_bass.push_back(i);
    }
    const std::vector<int>& pool = non_bass.empty() ? indices : non_bass;
    return *std::max_element(pool.begin(), pool.end(), [&](int a, int b) {
        return tracks[static_cast<size_t>(a)].notes.size() < tracks[static_cast<size_t>(b)].notes.size();
    });
}

std::vector<TrackInfo> MidiLoader::list_tracks(const std::wstring& path) const {
    std::vector<RawTrack> tracks = MidiReader::read(path);
    std::vector<TrackInfo> out;
    for (size_t i = 0; i < tracks.size(); ++i) {
        const RawTrack& t = tracks[i];
        if (t.is_drum || t.notes.empty()) continue;
        out.push_back({static_cast<int>(i), t.name, static_cast<int>(t.notes.size()), mean_pitch(t)});
    }
    return out;
}

std::vector<NoteSegment> MidiLoader::load(const std::wstring& path, const ConversionSettings& settings,
                                           const std::function<void(const std::wstring&)>& progress) const {
    progress(L"Чтение MIDI...");
    std::vector<RawTrack> tracks = MidiReader::read(path);

    std::vector<int> indices;
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (!tracks[i].is_drum && !tracks[i].notes.empty()) indices.push_back(static_cast<int>(i));
    }
    if (indices.empty()) throw ConversionError("В MIDI нет мелодических дорожек.");

    int index = settings.track.has_value() ? *settings.track : auto_select(tracks, indices);
    if (std::find(indices.begin(), indices.end(), index) == indices.end()) {
        throw ConversionError("В файле нет дорожки с таким номером.");
    }

    progress(L"Выделение мелодии из дорожки " + std::to_wstring(index) + L"...");
    return skyline(tracks[static_cast<size_t>(index)].notes);
}

}  // namespace song2notes
