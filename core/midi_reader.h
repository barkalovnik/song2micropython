// midi_reader.h - разбор Standard MIDI File (.mid/.midi) "с нуля", без
// внешних библиотек: заголовок MThd, дорожки MTrk, running status,
// мета-события и темп-карта для перевода тиков в секунды.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace song2notes {

struct RawNote {
    double start = 0.0;  // секунды
    double end = 0.0;    // секунды
    int pitch = 0;
};

struct RawTrack {
    std::wstring name;
    bool is_drum = false;  // хотя бы одна нота была на MIDI-канале 10 (ударные)
    std::vector<RawNote> notes;
};

class MidiReader {
public:
    // Бросает ConversionError, если файл повреждён или не является MIDI.
    static std::vector<RawTrack> read(const std::wstring& path);
};

}  // namespace song2notes
