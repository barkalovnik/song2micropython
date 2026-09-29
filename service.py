"""Прикладной сервис.

Паттерн: [Facade] — единая простая точка входа над загрузчиками (Strategy+Factory),
конвейером (Pipeline) и экспортёрами (Template Method). И CLI, и GUI-контроллер
знают только про этот класс.
"""
from __future__ import annotations

from collections.abc import Sequence
from pathlib import Path

from .domain import (
    ConversionResult,
    ConversionSettings,
    NoteEvent,
    ProgressCallback,
    TrackInfo,
)
from .errors import ConversionError, EmptyMelodyError
from .events import segments_to_events
from .exporters import get_exporter
from .loaders import LoaderFactory, default_loader_factory
from .pipeline import build_pipeline


def _ignore_progress(_message: str) -> None:
    """Пустой колбэк прогресса."""


class ConversionService:
    """[Pattern: Facade | Role: Facade] Упрощённый интерфейс ко всей подсистеме конвертации."""

    def __init__(self, loaders: LoaderFactory | None = None) -> None:
        # Dependency Injection: фабрику можно подменить (например, в тестах).
        self._loaders = loaders or default_loader_factory()

    def supported_extensions(self) -> list[str]:
        return self._loaders.supported_extensions()

    def inspect(self, path: Path) -> list[TrackInfo]:
        """Дорожки файла (для MIDI); для аудио — пустой список."""
        return self._loaders.create(path).list_tracks(path)

    def convert(
        self,
        path: Path,
        settings: ConversionSettings,
        progress: ProgressCallback = _ignore_progress,
    ) -> ConversionResult:
        self._validate(settings)
        loader = self._loaders.create(path)                       # Factory -> Strategy
        segments = loader.load(path, settings, progress)          # Strategy
        progress("Обработка нот...")
        segments = build_pipeline(settings).run(segments)         # Pipeline
        if not segments:
            raise EmptyMelodyError(
                "Не найдено ни одной ноты. Попробуйте другой фрагмент, дорожку или файл."
            )
        events = segments_to_events(segments, settings.speed)
        truncated = len(events) > settings.max_notes
        return ConversionResult(tuple(events[: settings.max_notes]), truncated)

    def render(
        self, events: Sequence[NoteEvent], settings: ConversionSettings, source_name: str
    ) -> str:
        """Текст результата в выбранном формате (Strategy: экспортёр)."""
        return get_exporter(settings.exporter_key).export(events, settings, source_name)

    @staticmethod
    def _validate(settings: ConversionSettings) -> None:
        if settings.end is not None and settings.end <= settings.start:
            raise ConversionError("Конец фрагмента должен быть больше его начала.")
        if settings.speed <= 0:
            raise ConversionError("Скорость должна быть положительной.")
