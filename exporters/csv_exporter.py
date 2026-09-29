"""Экспорт в CSV (для таблиц, отладки и других языков).

Паттерны: [Template Method] — «ConcreteClass»; [Strategy] — «ConcreteStrategy».
"""
from __future__ import annotations

from collections.abc import Sequence
from typing import ClassVar

from ..domain import ConversionSettings, NoteEvent
from ..notes import midi_to_freq, midi_to_name
from .base import Exporter


class CsvExporter(Exporter):
    """[Pattern: Template Method | Role: ConcreteClass] Таблица: нота, частота, длительность."""

    key: ClassVar[str] = "csv"
    label: ClassVar[str] = "Таблица CSV"
    file_suffix: ClassVar[str] = ".csv"

    def header(
        self, events: Sequence[NoteEvent], settings: ConversionSettings, source_name: str
    ) -> str:
        return "midi,note,freq_hz,duration_ms"

    def body(self, events: Sequence[NoteEvent], settings: ConversionSettings) -> str:
        rows: list[str] = []
        for e in events:
            if e.pitch is None:
                rows.append(f",rest,0,{e.duration_ms}")
            else:
                rows.append(f"{e.pitch},{midi_to_name(e.pitch)},{midi_to_freq(e.pitch)},{e.duration_ms}")
        return "\n".join(rows)
