"""Экспорт в готовый скрипт MicroPython.

Паттерны: [Template Method] — роль «ConcreteClass»; [Strategy] — «ConcreteStrategy».
"""
from __future__ import annotations

from collections.abc import Sequence
from typing import ClassVar

from ..domain import ConversionSettings, NoteEvent
from ..notes import midi_to_freq, midi_to_name
from .base import Exporter


class MicroPythonExporter(Exporter):
    """[Pattern: Template Method | Role: ConcreteClass] Скрипт для Raspberry Pi Pico."""

    key: ClassVar[str] = "micropython"
    label: ClassVar[str] = "Скрипт MicroPython"
    file_suffix: ClassVar[str] = ".py"

    def header(
        self, events: Sequence[NoteEvent], settings: ConversionSettings, source_name: str
    ) -> str:
        total = sum(e.duration_ms for e in events) / 1000
        return "\n".join([
            f"# Сгенерировано song2notes из: {source_name}",
            f"# Нот: {len(events)}, длительность: ~{total:.1f} с",
            "from machine import Pin, PWM",
            "import time",
            "",
            f"buzzer = PWM(Pin({settings.pin}))",
            "",
            "# (частота в Гц, длительность в мс); частота 0 - пауза",
            "melody = [",
        ])

    def body(self, events: Sequence[NoteEvent], settings: ConversionSettings) -> str:
        lines: list[str] = []
        for e in events:
            if e.pitch is None:
                lines.append(f"    (0, {e.duration_ms}),")
            else:
                lines.append(
                    f"    ({midi_to_freq(e.pitch)}, {e.duration_ms}),  # {midi_to_name(e.pitch)}"
                )
        return "\n".join(lines)

    def footer(self, events: Sequence[NoteEvent], settings: ConversionSettings) -> str:
        return "\n".join([
            "]",
            "",
            "def play(melody, articulation=0.9):",
            "    for freq, ms in melody:",
            "        if freq > 0:",
            "            buzzer.freq(freq)",
            "            buzzer.duty_u16(32768)",
            "        time.sleep_ms(int(ms * articulation))",
            "        buzzer.duty_u16(0)",
            "        time.sleep_ms(ms - int(ms * articulation))",
            "",
            "try:",
            "    play(melody)",
            "finally:",
            "    buzzer.deinit()",
        ])
