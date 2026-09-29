"""Базовый экспортёр.

Паттерны:
  [Template Method] — export() задаёт скелет (header -> body -> footer),
                      подклассы переопределяют отдельные шаги;
  [Strategy]        — экспортёры взаимозаменяемы, формат выбирается в настройках.
"""
from __future__ import annotations

from abc import ABC, abstractmethod
from collections.abc import Sequence
from typing import ClassVar

from ..domain import ConversionSettings, NoteEvent


class Exporter(ABC):
    """[Pattern: Template Method | Role: AbstractClass] + [Strategy | Role: Strategy]"""

    key: ClassVar[str]          # идентификатор в настройках/CLI
    label: ClassVar[str]        # название для интерфейса
    file_suffix: ClassVar[str]  # расширение выходного файла

    def export(
        self, events: Sequence[NoteEvent], settings: ConversionSettings, source_name: str
    ) -> str:
        """Шаблонный метод: собирает текст из трёх частей."""
        sections = (
            self.header(events, settings, source_name),
            self.body(events, settings),
            self.footer(events, settings),
        )
        return "\n".join(s for s in sections if s) + "\n"

    def header(
        self, events: Sequence[NoteEvent], settings: ConversionSettings, source_name: str
    ) -> str:
        """Необязательный шаг (hook): по умолчанию пусто."""
        return ""

    @abstractmethod
    def body(self, events: Sequence[NoteEvent], settings: ConversionSettings) -> str:
        """Обязательный шаг: основное содержимое."""

    def footer(self, events: Sequence[NoteEvent], settings: ConversionSettings) -> str:
        """Необязательный шаг (hook): по умолчанию пусто."""
        return ""
