#include "csv_exporter.h"

#include <sstream>

#include "notes.h"
#include "text_convert.h"

namespace song2notes {

std::string CsvExporter::header(const std::vector<NoteEvent>&, const ConversionSettings&,
                                 const std::string&) const {
    return "midi,note,freq_hz,duration_ms\n";
}

std::string CsvExporter::body(const std::vector<NoteEvent>& events, const ConversionSettings&) const {
    std::ostringstream out;
    for (const auto& e : events) {
        if (!e.pitch.has_value()) {
            out << ",rest,0," << e.duration_ms << "\n";
        } else {
            out << *e.pitch << "," << narrow_utf8(notes::midi_to_name(*e.pitch)) << ","
                << notes::midi_to_freq(*e.pitch) << "," << e.duration_ms << "\n";
        }
    }
    return out.str();
}

}  // namespace song2notes
