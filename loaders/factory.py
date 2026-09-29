"""Выбор загрузчика по расширению файла.

Паттерны: [Factory] (простая фабрика) + [Registry] — реестр «расширение -> загрузчик».
"""
from __future__ import annotations

from collections.abc import Iterable
from pathlib import Path

from ..errors import UnsupportedFormatError
from .audio_loader import AudioLoader
from .base import SegmentLoader
from .midi_loader import MidiLoader


class LoaderFactory:
    """[Pattern: Factory | Role: Creator] Создаёт (выдаёт) нужную Strategy для файла."""

    def __init__(self, loaders: Iterable[SegmentLoader] = ()) -> None:
        self._by_ext: dict[str, SegmentLoader] = {}
        for loader in loaders:
            self.register(loader)

    def register(self, loader: SegmentLoader) -> None:
        for ext in loader.extensions:
            self._by_ext[ext] = loader

    def create(self, path: Path) -> SegmentLoader:
        try:
            return self._by_ext[path.suffix.lower()]
        except KeyError:
            supported = ", ".join(self.supported_extensions())
            raise UnsupportedFormatError(
                f"Формат «{path.suffix}» не поддерживается. Доступно: {supported}"
            ) from None

    def supported_extensions(self) -> list[str]:
        return sorted(self._by_ext)


def default_loader_factory() -> LoaderFactory:
    return LoaderFactory([MidiLoader(), AudioLoader()])
