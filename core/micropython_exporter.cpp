#include "micropython_exporter.h"

#include <sstream>

#include "notes.h"
#include "text_convert.h"

namespace song2notes {

std::string MicroPythonExporter::header(const std::vector<NoteEvent>& events,
                                         const ConversionSettings& settings,
                                         const std::string& source_name) const {
    long long total_ms = 0;
    for (const auto& e : events) total_ms += e.duration_ms;

    std::ostringstream out;
    out << "# Сгенерировано song2notes из: " << source_name << "\n";
    out << "# Нот: " << events.size() << ", длительность: ~" << (total_ms / 1000.0) << " с\n";
    out << "from machine import Pin, PWM\n";
    out << "import time\n\n";
    out << "buzzer = PWM(Pin(" << settings.pin << "))\n\n";
    out << "# (частота в Гц, длительность в мс); частота 0 - пауза\n";
    out << "melody = [\n";
    return out.str();
}

std::string MicroPythonExporter::body(const std::vector<NoteEvent>& events,
                                       const ConversionSettings&) const {
    std::ostringstream out;
    for (const auto& e : events) {
        if (!e.pitch.has_value()) {
            out << "    (0, " << e.duration_ms << "),\n";
        } else {
            out << "    (" << notes::midi_to_freq(*e.pitch) << ", " << e.duration_ms << "),  # "
                << narrow_utf8(notes::midi_to_name(*e.pitch)) << "\n";
        }
    }
    return out.str();
}

std::string MicroPythonExporter::footer(const std::vector<NoteEvent>&, const ConversionSettings&) const {
    return "]\n"
           "\n"
           "def play(melody, articulation=0.9):\n"
           "    for freq, ms in melody:\n"
           "        if freq > 0:\n"
           "            buzzer.freq(freq)\n"
           "            buzzer.duty_u16(32768)\n"
           "        time.sleep_ms(int(ms * articulation))\n"
           "        buzzer.duty_u16(0)\n"
           "        time.sleep_ms(ms - int(ms * articulation))\n"
           "\n"
           "try:\n"
           "    play(melody)\n"
           "finally:\n"
           "    buzzer.deinit()\n";
}

}  // namespace song2notes
