"""Интерфейс загрузчиков.

Паттерн: [Strategy] — роль «Strategy». Каждый формат файла (MIDI, аудио)
реализует один и тот же интерфейс, а сервис работает только с абстракцией.
"""
from __future__ import annotations

from abc import ABC, abstractmethod
from pathlib import Path
from typing import ClassVar

from ..domain import ConversionSettings, NoteSegment, ProgressCallback, TrackInfo


class SegmentLoader(ABC):
    """[Pattern: Strategy | Role: Strategy] Читает файл и выдаёт одноголосные сегменты."""

    #: Расширения файлов (с точкой, в нижнем регистре), которые умеет читать загрузчик.
    extensions: ClassVar[frozenset[str]]

    @abstractmethod
    def load(
        self, path: Path, settings: ConversionSettings, progress: ProgressCallback
    ) -> list[NoteSegment]:
        """Возвращает сегменты в абсолютном времени файла (секунды)."""

    def list_tracks(self, path: Path) -> list[TrackInfo]:
        """Дорожки файла для выбора пользователем. По умолчанию — нет выбора."""
        return []
