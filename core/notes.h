// notes.h - перевод номера ноты MIDI в частоту и название.
#pragma once
#include <string>

namespace song2notes::notes {

// Диапазон, в котором пищалка звучит нормально (G3..C7).
constexpr int PITCH_LOW = 55;
constexpr int PITCH_HIGH = 96;

int midi_to_freq(int pitch);
std::wstring midi_to_name(int pitch);

}  // namespace song2notes::notes
