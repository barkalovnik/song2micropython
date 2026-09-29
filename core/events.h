// events.h - превращение сегментов на временной шкале в события (нота/пауза + мс).
#pragma once
#include <vector>

#include "domain.h"

namespace song2notes {

std::vector<NoteEvent> segments_to_events(std::vector<NoteSegment> segments, double speed);

}  // namespace song2notes
