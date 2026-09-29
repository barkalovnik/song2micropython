// midi_loader.h - загрузчик MIDI: выбор дорожки + сведение к одноголосию.
// [Pattern: Strategy | Role: ConcreteStrategy]
#pragma once
#include "segment_loader.h"

namespace song2notes {

class MidiLoader : public SegmentLoader {
public:
    std::vector<NoteSegment> load(const std::wstring& path, const ConversionSettings& settings,
                                   const std::function<void(const std::wstring&)>& progress) const override;
    std::vector<TrackInfo> list_tracks(const std::wstring& path) const override;

private:
    static int mean_pitch(const struct RawTrack& track);
    static int auto_select(const std::vector<RawTrack>& tracks, const std::vector<int>& indices);
};

}  // namespace song2notes
