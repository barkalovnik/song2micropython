#include "midi_reader.h"

#include <algorithm>
#include <cstring>

#include "errors.h"
#include "io_utils.h"

namespace song2notes {

namespace {

uint16_t read_u16(const uint8_t* d) { return static_cast<uint16_t>((d[0] << 8) | d[1]); }

uint32_t read_u32(const uint8_t* d) {
    return (uint32_t(d[0]) << 24) | (uint32_t(d[1]) << 16) | (uint32_t(d[2]) << 8) | uint32_t(d[3]);
}

// Variable Length Quantity: числа переменной длины, как их кодирует формат MIDI.
uint32_t read_vlq(const uint8_t* data, size_t size, size_t& pos) {
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        if (pos >= size) throw ConversionError("Повреждён MIDI-файл: неожиданный конец данных.");
        uint8_t b = data[pos++];
        value = (value << 7) | (b & 0x7Fu);
        if (!(b & 0x80u)) break;
    }
    return value;
}

// Событие note-on/note-off с абсолютным временем в тиках (внутренний формат разбора).
struct RawEvent {
    long tick = 0;
    bool is_note_on = false;
    uint8_t channel = 0;
    uint8_t pitch = 0;
    uint8_t velocity = 0;
};

struct RawTempoEvent {
    long tick;
    int us_per_quarter;
};

struct TrackParseResult {
    std::wstring name;
    std::vector<RawEvent> events;
    bool any_channel9 = false;
};

TrackParseResult parse_track(const uint8_t* data, size_t length, std::vector<RawTempoEvent>& tempos) {
    TrackParseResult track;
    size_t pos = 0;
    long tick = 0;
    uint8_t running_status = 0;

    while (pos < length) {
        tick += static_cast<long>(read_vlq(data, length, pos));
        if (pos >= length) break;

        uint8_t byte = data[pos];
        uint8_t status;
        if (byte & 0x80u) {
            status = byte;
            ++pos;
        } else {
            status = running_status;
        }
        if (status == 0) throw ConversionError("Повреждён MIDI-файл: событие без статус-байта.");

        if (status == 0xFF) {  // Meta-событие
            if (pos >= length) throw ConversionError("Повреждён MIDI-файл: обрезано мета-событие.");
            uint8_t meta_type = data[pos++];
            uint32_t meta_len = read_vlq(data, length, pos);
            if (pos + meta_len > length) {
                throw ConversionError("Повреждён MIDI-файл: мета-событие выходит за границы дорожки.");
            }
            if (meta_type == 0x51 && meta_len == 3) {  // Set Tempo
                int us = (int(data[pos]) << 16) | (int(data[pos + 1]) << 8) | int(data[pos + 2]);
                tempos.push_back({tick, us});
            } else if ((meta_type == 0x03 || meta_type == 0x04) && track.name.empty() && meta_len > 0) {
                // Название дорожки/инструмента. MIDI хранит его как байты без явной
                // кодировки; берём как есть (латиница/цифры читаются корректно всегда).
                track.name.assign(data + pos, data + pos + meta_len);
            }
            pos += meta_len;
            running_status = 0;  // мета-события сбрасывают running status
            continue;
        }
        if (status == 0xF0 || status == 0xF7) {  // SysEx
            uint32_t sysex_len = read_vlq(data, length, pos);
            if (pos + sysex_len > length) {
                throw ConversionError("Повреждён MIDI-файл: SysEx выходит за границы дорожки.");
            }
            pos += sysex_len;
            running_status = 0;
            continue;
        }

        running_status = status;
        uint8_t hi = status & 0xF0u;
        uint8_t channel = status & 0x0Fu;
        auto need = [&](size_t n) {
            if (pos + n > length) throw ConversionError("Повреждён MIDI-файл: событие обрезано.");
        };

        switch (hi) {
            case 0x80: {  // Note Off
                need(2);
                uint8_t pitch = data[pos] & 0x7Fu;
                pos += 2;
                track.events.push_back({tick, false, channel, pitch, 0});
                if (channel == 9) track.any_channel9 = true;
                break;
            }
            case 0x90: {  // Note On (velocity 0 равнозначен Note Off)
                need(2);
                uint8_t pitch = data[pos] & 0x7Fu;
                uint8_t velocity = data[pos + 1] & 0x7Fu;
                pos += 2;
                track.events.push_back({tick, velocity > 0, channel, pitch, velocity});
                if (channel == 9) track.any_channel9 = true;
                break;
            }
            case 0xA0: need(2); pos += 2; break;  // Polyphonic aftertouch
            case 0xB0: need(2); pos += 2; break;  // Control change
            case 0xC0: need(1); pos += 1; break;  // Program change
            case 0xD0: need(1); pos += 1; break;  // Channel aftertouch
            case 0xE0: need(2); pos += 2; break;  // Pitch bend
            default:
                throw ConversionError("Повреждён MIDI-файл: неизвестное событие.");
        }
    }
    return track;
}

}  // namespace

std::vector<RawTrack> MidiReader::read(const std::wstring& path) {
    std::vector<uint8_t> data = read_file_bytes(path);
    if (data.size() < 14 || std::memcmp(data.data(), "MThd", 4) != 0) {
        throw ConversionError("Это не MIDI-файл (нет заголовка MThd).");
    }
    uint32_t header_len = read_u32(data.data() + 4);
    if (header_len < 6 || data.size() < 8 + static_cast<size_t>(header_len)) {
        throw ConversionError("Повреждён заголовок MIDI-файла.");
    }
    const uint8_t* hdr = data.data() + 8;
    uint16_t ntrks = read_u16(hdr + 2);
    uint16_t division = read_u16(hdr + 4);
    if (division & 0x8000u) {
        throw ConversionError("MIDI-файлы с тайм-кодом SMPTE не поддерживаются.");
    }
    int ppq = division & 0x7FFFu;
    if (ppq == 0) ppq = 96;

    size_t pos = 8 + header_len;
    std::vector<RawTempoEvent> tempos;
    std::vector<TrackParseResult> parsed_tracks;

    for (uint16_t i = 0; i < ntrks && pos + 8 <= data.size(); ++i) {
        if (std::memcmp(data.data() + pos, "MTrk", 4) != 0) {
            // Неизвестный чанк: пропустить нельзя без длины, но такие файлы
            // на практике не встречаются - прекращаем разбор оставшихся дорожек.
            break;
        }
        uint32_t track_len = read_u32(data.data() + pos + 4);
        size_t track_start = pos + 8;
        if (track_start + track_len > data.size()) {
            throw ConversionError("Повреждён MIDI-файл: длина дорожки выходит за границы файла.");
        }
        parsed_tracks.push_back(parse_track(data.data() + track_start, track_len, tempos));
        pos = track_start + track_len;
    }

    if (tempos.empty() || tempos.front().tick != 0) {
        tempos.insert(tempos.begin(), RawTempoEvent{0, 500000});  // по умолчанию 120 BPM
    }
    std::sort(tempos.begin(), tempos.end(), [](const auto& a, const auto& b) { return a.tick < b.tick; });

    // Карта темпа: для каждой точки смены темпа - сколько секунд прошло с начала файла.
    struct TempoPoint {
        long tick;
        double seconds;
        int us_per_quarter;
    };
    std::vector<TempoPoint> tempo_map;
    tempo_map.push_back({tempos[0].tick, 0.0, tempos[0].us_per_quarter});
    for (size_t i = 1; i < tempos.size(); ++i) {
        const TempoPoint& prev = tempo_map.back();
        double delta_seconds = (tempos[i].tick - prev.tick) * (prev.us_per_quarter / 1e6) / ppq;
        tempo_map.push_back({tempos[i].tick, prev.seconds + delta_seconds, tempos[i].us_per_quarter});
    }
    auto tick_to_seconds = [&](long tick) -> double {
        auto it = std::upper_bound(tempo_map.begin(), tempo_map.end(), tick,
                                    [](long t, const TempoPoint& p) { return t < p.tick; });
        --it;  // последняя точка темпа с tick <= запрошенного
        double delta_seconds = (tick - it->tick) * (it->us_per_quarter / 1e6) / ppq;
        return it->seconds + delta_seconds;
    };

    std::vector<RawTrack> result;
    result.reserve(parsed_tracks.size());
    for (auto& parsed : parsed_tracks) {
        RawTrack track;
        track.name = parsed.name;
        track.is_drum = parsed.any_channel9;

        // Активные (ещё не отпущенные) ноты: ключ = канал*128 + высота тона.
        std::vector<std::pair<int, long>> active;
        auto key_of = [](uint8_t ch, uint8_t p) { return int(ch) * 128 + int(p); };

        for (const RawEvent& ev : parsed.events) {
            int key = key_of(ev.channel, ev.pitch);
            auto it = std::find_if(active.begin(), active.end(),
                                    [&](const auto& kv) { return kv.first == key; });
            if (ev.is_note_on) {
                if (it != active.end()) active.erase(it);  // повторный Note On без Off - переоткрываем
                active.push_back({key, ev.tick});
            } else if (it != active.end()) {
                double start_sec = tick_to_seconds(it->second);
                double end_sec = tick_to_seconds(ev.tick);
                if (end_sec > start_sec) {
                    track.notes.push_back({start_sec, end_sec, static_cast<int>(ev.pitch)});
                }
                active.erase(it);
            }
        }
        result.push_back(std::move(track));
    }
    return result;
}

}  // namespace song2notes
