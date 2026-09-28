#!/usr/bin/env python3
"""
song2notes.py - превращает MIDI / MP3 / WAV в одноголосную мелодию для пищалки
на Raspberry Pi Pico (MicroPython). Запускать на КОМПЬЮТЕРЕ.

Установка:
    pip install pretty_midi            # для .mid / .midi
    pip install librosa numpy          # для .mp3 / .wav / .ogg / .flac (нужен ещё ffmpeg для mp3)

Примеры:
    python song2notes.py song.mid --list                 # показать дорожки MIDI
    python song2notes.py song.mid                        # авто-выбор дорожки
    python song2notes.py song.mid --track 2 --bpm-scale 1.2
    python song2notes.py song.mp3 --start 30 --end 60    # 30 секунд с 0:30
    python song2notes.py song.mp3 --pin 15 -o main.py

Результат (по умолчанию <имя>_melody.py) загружается на Pico как main.py.

ВАЖНО: пищалка играет только ОДИН звук одновременно. Из MIDI берётся самая
верхняя нота аккорда (обычно это мелодия). Из MP3 определяется высота тона
одного голоса; с полной аранжировкой (гитара, бас, барабаны, вокал вместе)
результат будет шумным. Для MP3 лучше подавать вокал/соло отдельно или
использовать MIDI-версию песни.
"""
import argparse
import sys
from collections import Counter
from pathlib import Path

NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
LO, HI = 55, 96  # диапазон, в котором пищалка звучит нормально (G3..C7)


def midi_to_freq(n):
    return int(round(440.0 * 2 ** ((n - 69) / 12)))


def midi_to_name(n):
    return f"{NAMES[n % 12]}{n // 12 - 1}"


# ---------------------------------------------------------------- MIDI

def skyline(notes):
    """Одноголосие: в каждый момент берём самую высокую звучащую ноту.
    notes: [(start, end, pitch)] -> [(start, end, pitch)]"""
    notes = sorted(notes)
    bounds = sorted({t for s, e, _ in notes for t in (s, e)})
    out, active, idx = [], [], 0
    for t0, t1 in zip(bounds, bounds[1:]):
        while idx < len(notes) and notes[idx][0] <= t0 + 1e-9:
            active.append((idx, notes[idx]))
            idx += 1
        active = [a for a in active if a[1][1] > t0 + 1e-9]
        if not active:
            continue
        i, (_, _, p) = max(active, key=lambda a: a[1][2])
        if out and out[-1][3] == i and abs(out[-1][1] - t0) < 1e-9:
            out[-1][1] = t1  # продолжение той же ноты
        else:
            out.append([t0, t1, p, i])
    return [(a, b, p) for a, b, p, _ in out]


def load_midi(path, track, list_only):
    try:
        import pretty_midi
    except ImportError:
        sys.exit("Нужен pretty_midi:  pip install pretty_midi")
    pm = pretty_midi.PrettyMIDI(str(path))
    cands = [(i, ins) for i, ins in enumerate(pm.instruments)
             if not ins.is_drum and ins.notes]
    if not cands:
        sys.exit("В MIDI нет мелодических дорожек.")

    def mean_pitch(ins):
        return sum(n.pitch for n in ins.notes) / len(ins.notes)

    if list_only:
        print("Дорожки:")
        for i, ins in cands:
            print(f"  {i}: {ins.name or '(без имени)':25s} нот: {len(ins.notes):5d}  "
                  f"средняя высота: {midi_to_name(int(mean_pitch(ins)))}")
        sys.exit(0)

    if track is None:
        # Автовыбор: больше всего нот среди не-басовых дорожек
        non_bass = [c for c in cands if mean_pitch(c[1]) >= 55] or cands
        track = max(non_bass, key=lambda c: len(c[1].notes))[0]
        print(f"Выбрана дорожка {track} (укажите --track N, чтобы сменить)")
    ins = pm.instruments[track]
    return skyline([(n.start, n.end, n.pitch) for n in ins.notes])


def crop(segs, start, end):
    out = []
    for s, e, p in segs:
        if e <= start or (end is not None and s >= end):
            continue
        s2 = max(s, start)
        e2 = e if end is None else min(e, end)
        out.append((s2 - start, e2 - start, p))
    return out


# ---------------------------------------------------------------- Audio

def load_audio(path, start, end, min_note_ms):
    try:
        import numpy as np
        import librosa
    except ImportError:
        sys.exit("Нужны librosa и numpy:  pip install librosa numpy  (и ffmpeg для mp3)")
    sr, hop = 22050, 256
    dur = None if end is None else end - start
    print("Загружаю аудио...")
    y, sr = librosa.load(str(path), sr=sr, mono=True, offset=start, duration=dur)
    print("Определяю высоту тона (это может занять минуту)...")
    f0, voiced, _ = librosa.pyin(
        y, fmin=librosa.note_to_hz('C2'), fmax=librosa.note_to_hz('C7'),
        sr=sr, frame_length=2048, hop_length=hop)
    midi = np.where(voiced & ~np.isnan(f0), np.round(librosa.hz_to_midi(f0)), -1)
    midi = midi.astype(int).tolist()

    # Сглаживание: голосование по окну в 5 кадров
    sm = []
    for i in range(len(midi)):
        win = midi[max(0, i - 2): i + 3]
        sm.append(Counter(win).most_common(1)[0][0])

    onsets = set(librosa.onset.onset_detect(y=y, sr=sr, hop_length=hop).tolist())
    fsec = hop / sr

    segs, cur_start, cur_p = [], 0, sm[0] if sm else -1
    for i in range(1, len(sm) + 1):
        end_run = i == len(sm) or sm[i] != cur_p or (i in onsets and cur_p >= 0)
        if end_run:
            if cur_p >= 0:
                segs.append((cur_start * fsec, i * fsec, cur_p))
            if i < len(sm):
                cur_start, cur_p = i, sm[i]
    return segs


# ---------------------------------------------------------------- Обработка

def clean(segs, min_ms):
    return [(s, e, p) for s, e, p in segs if (e - s) * 1000 >= min_ms]


def fit_range(segs, transpose):
    if not segs:
        return segs
    if transpose is None:
        best = max(range(-48, 49, 12),
                   key=lambda k: (sum(LO <= p + k <= HI for _, _, p in segs), -abs(k)))
        transpose = best
        if transpose:
            print(f"Авто-транспонирование: {transpose:+d} полутонов")
    out = []
    for s, e, p in segs:
        p += transpose
        while p < LO:
            p += 12
        while p > HI:
            p -= 12
        out.append((s, e, p))
    return out


def to_events(segs, speed):
    """-> [(midi_или_None, мс)]"""
    ev, cursor = [], 0.0
    for s, e, p in sorted(segs):
        if s < cursor:
            s = cursor
        if e <= s:
            continue
        if s - cursor > 0.02:
            ev.append((None, int((s - cursor) * 1000 / speed)))
        else:
            s = cursor  # маленькая щель - просто растягиваем ноту
        ev.append((p, max(1, int((e - s) * 1000 / speed))))
        cursor = e
    return ev


TEMPLATE = '''# Сгенерировано song2notes.py из: __SRC__
# Нот: __COUNT__, длительность: ~__TOTAL__ с
from machine import Pin, PWM
import time

buzzer = PWM(Pin(__PIN__))

# (частота в Гц, длительность в мс); частота 0 - пауза
melody = [
__MELODY__
]

def play(melody, articulation=0.9):
    for freq, ms in melody:
        if freq > 0:
            buzzer.freq(freq)
            buzzer.duty_u16(32768)
        time.sleep_ms(int(ms * articulation))
        buzzer.duty_u16(0)
        time.sleep_ms(ms - int(ms * articulation))

try:
    play(melody)
finally:
    buzzer.deinit()
'''


def write_output(events, out_path, src, pin):
    lines = []
    for p, ms in events:
        if p is None:
            lines.append(f"    (0, {ms}),")
        else:
            lines.append(f"    ({midi_to_freq(p)}, {ms}),  # {midi_to_name(p)}")
    total = sum(ms for _, ms in events) / 1000
    text = (TEMPLATE.replace('__SRC__', src)
            .replace('__COUNT__', str(len(events)))
            .replace('__TOTAL__', f"{total:.1f}")
            .replace('__PIN__', str(pin))
            .replace('__MELODY__', "\n".join(lines)))
    Path(out_path).write_text(text, encoding='utf-8')


def main():
    ap = argparse.ArgumentParser(description="MIDI/MP3 -> мелодия для пищалки MicroPython")
    ap.add_argument("input", help=".mid / .midi / .mp3 / .wav / .ogg / .flac")
    ap.add_argument("-o", "--output", help="выходной .py (по умолчанию <имя>_melody.py)")
    ap.add_argument("--pin", type=int, default=26, help="номер GPIO пищалки (по умолчанию 26)")
    ap.add_argument("--track", type=int, help="номер дорожки MIDI (см. --list)")
    ap.add_argument("--list", action="store_true", help="показать дорожки MIDI и выйти")
    ap.add_argument("--start", type=float, default=0.0, help="начало фрагмента, сек")
    ap.add_argument("--end", type=float, default=None, help="конец фрагмента, сек")
    ap.add_argument("--speed", type=float, default=1.0, help="множитель скорости (2 = вдвое быстрее)")
    ap.add_argument("--transpose", type=int, default=None,
                    help="сдвиг в полутонах (по умолчанию подбирается автоматически)")
    ap.add_argument("--min-note-ms", type=int, default=60,
                    help="отбрасывать ноты короче N мс (по умолчанию 60)")
    ap.add_argument("--max-notes", type=int, default=1500,
                    help="обрезать мелодию до N нот, чтобы влезла в память Pico")
    args = ap.parse_args()

    src = Path(args.input)
    if not src.exists():
        sys.exit(f"Файл не найден: {src}")
    ext = src.suffix.lower()

    if ext in ('.mid', '.midi'):
        segs = load_midi(src, args.track, args.list)
        segs = crop(segs, args.start, args.end)
    else:
        if args.list:
            sys.exit("--list работает только для MIDI.")
        segs = load_audio(src, args.start, args.end, args.min_note_ms)

    segs = clean(segs, args.min_note_ms)
    if not segs:
        sys.exit("Не удалось найти ни одной ноты. Попробуйте другой фрагмент или файл.")
    segs = fit_range(segs, args.transpose)
    events = to_events(segs, args.speed)

    if len(events) > args.max_notes:
        print(f"Мелодия обрезана: {len(events)} -> {args.max_notes} нот "
              f"(--max-notes или --end, чтобы изменить)")
        events = events[:args.max_notes]

    out = args.output or f"{src.stem}_melody.py"
    write_output(events, out, src.name, args.pin)
    total = sum(ms for _, ms in events) / 1000
    print(f"Готово: {out}  ({len(events)} нот, ~{total:.1f} с)")
    print("Загрузите файл на Pico (например, как main.py) через Thonny.")


if __name__ == "__main__":
    main()
