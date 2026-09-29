// skyline.h - сведение полифонической дорожки к одному голосу: в каждый
// момент времени берётся самая высокая из звучащих нот (обычно это мелодия).
#pragma once
#include <vector>

#include "domain.h"
#include "midi_reader.h"

namespace song2notes {

std::vector<NoteSegment> skyline(const std::vector<RawNote>& notes);

}  // namespace song2notes
