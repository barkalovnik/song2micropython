"""Доменные модели.

Паттерн: [Value Object] — все классы неизменяемы (frozen), сравниваются по
значению. Это позволяет безопасно передавать их между потоками и сигналами Qt.
"""
from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass

#: Колбэк для сообщений о прогрессе (используется загрузчиками и сервисом).
ProgressCallback = Callable[[str], None]


@dataclass(frozen=True, slots=True)
class NoteSegment:
    """Нота на временной шкале: секунды и номер ноты MIDI."""

    start: float
    end: float
    pitch: int

    @property
    def duration(self) -> float:
        return self.end - self.start


@dataclass(frozen=True, slots=True)
class NoteEvent:
    """Событие мелодии: нота (или пауза, если pitch is None) и длительность в мс."""

    pitch: int | None
    duration_ms: int


@dataclass(frozen=True, slots=True)
class TrackInfo:
    """Описание дорожки MIDI для выбора в интерфейсе."""

    index: int
    name: str
    note_count: int
    mean_pitch: int


@dataclass(frozen=True, slots=True)
class ConversionSettings:
    """Все параметры конвертации в одном объекте."""

    start: float = 0.0
    end: float | None = None
    track: int | None = None          # None = выбрать автоматически
    speed: float = 1.0
    transpose: int | None = None      # None = подобрать автоматически
    min_note_ms: int = 60
    max_notes: int = 1500
    pin: int = 26
    exporter_key: str = "micropython"


@dataclass(frozen=True, slots=True)
class ConversionResult:
    """Результат конвертации."""

    events: tuple[NoteEvent, ...]
    truncated: bool

    @property
    def total_ms(self) -> int:
        return sum(e.duration_ms for e in self.events)
