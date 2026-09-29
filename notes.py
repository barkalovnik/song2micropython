"""Работа с нотами: номер MIDI <-> название и частота."""
from __future__ import annotations

from typing import Final

NOTE_NAMES: Final[tuple[str, ...]] = (
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
)
#: Диапазон, в котором пищалка звучит нормально (G3..C7).
PITCH_LOW: Final[int] = 55
PITCH_HIGH: Final[int] = 96


def midi_to_freq(pitch: int) -> int:
    """Частота ноты в Гц (A4 = MIDI 69 = 440 Гц)."""
    return int(round(440.0 * 2 ** ((pitch - 69) / 12)))


def midi_to_name(pitch: int) -> str:
    """Название ноты, например 69 -> 'A4'."""
    return f"{NOTE_NAMES[pitch % 12]}{pitch // 12 - 1}"
