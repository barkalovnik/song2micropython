"""MODEL из MVC.

Паттерны:
  [MVC — Model]  хранит состояние приложения и ничего не знает ни о View, ни о Controller;
  [Observer]     любое изменение состояния рассылается Qt-сигналами (Subject = модель,
                 Observers = подписавшиеся представления).
"""
from __future__ import annotations

from pathlib import Path

from PyQt6.QtCore import QObject, pyqtSignal

from ..domain import ConversionResult, ConversionSettings, TrackInfo


class ConverterModel(QObject):
    """[Pattern: MVC | Role: Model] + [Observer | Role: Subject]"""

    source_changed = pyqtSignal(object)     # Path | None
    tracks_changed = pyqtSignal(list)       # list[TrackInfo]
    settings_changed = pyqtSignal(object)   # ConversionSettings
    result_changed = pyqtSignal(object)     # ConversionResult | None
    code_changed = pyqtSignal(str)
    busy_changed = pyqtSignal(bool)
    status_changed = pyqtSignal(str)
    error_occurred = pyqtSignal(str)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self._source: Path | None = None
        self._tracks: list[TrackInfo] = []
        self._settings = ConversionSettings()
        self._result: ConversionResult | None = None
        self._code = ""
        self._busy = False

    # --- чтение состояния ---------------------------------------------------
    @property
    def source(self) -> Path | None:
        return self._source

    @property
    def tracks(self) -> list[TrackInfo]:
        return list(self._tracks)

    @property
    def settings(self) -> ConversionSettings:
        return self._settings

    @property
    def result(self) -> ConversionResult | None:
        return self._result

    @property
    def code(self) -> str:
        return self._code

    @property
    def busy(self) -> bool:
        return self._busy

    # --- изменение состояния (вызывает Controller) ------------------------------
    def set_source(self, path: Path | None) -> None:
        self._source = path
        self.source_changed.emit(path)

    def set_tracks(self, tracks: list[TrackInfo]) -> None:
        self._tracks = list(tracks)
        self.tracks_changed.emit(self._tracks)

    def set_settings(self, settings: ConversionSettings) -> None:
        if settings == self._settings:
            return
        self._settings = settings
        self.settings_changed.emit(settings)

    def set_result(self, result: ConversionResult | None) -> None:
        self._result = result
        self.result_changed.emit(result)

    def set_code(self, code: str) -> None:
        self._code = code
        self.code_changed.emit(code)

    def set_busy(self, busy: bool) -> None:
        if busy == self._busy:
            return
        self._busy = busy
        self.busy_changed.emit(busy)

    def set_status(self, message: str) -> None:
        self.status_changed.emit(message)

    def report_error(self, message: str) -> None:
        self.error_occurred.emit(message)
