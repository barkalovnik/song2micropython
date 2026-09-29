#include "notes.h"

#include <array>
#include <cmath>

namespace song2notes::notes {

namespace {
constexpr std::array<const wchar_t*, 12> kNames = {
    L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B",
};
}

int midi_to_freq(int pitch) {
    return static_cast<int>(std::lround(440.0 * std::pow(2.0, (pitch - 69) / 12.0)));
}

std::wstring midi_to_name(int pitch) {
    int octave = pitch / 12 - 1;
    int index = pitch % 12;
    if (index < 0) index += 12;
    return std::wstring(kNames[static_cast<size_t>(index)]) + std::to_wstring(octave);
}

}  // namespace song2notes::notes
