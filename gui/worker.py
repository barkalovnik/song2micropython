"""Фоновая конвертация, чтобы интерфейс не зависал на долгом анализе аудио.

Паттерн: [Worker Object] (moveToThread) — объект-исполнитель живёт в отдельном
QThread и общается с остальными только сигналами.
"""
from __future__ import annotations

from pathlib import Path

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from ..domain import ConversionSettings
from ..service import ConversionService


class ConversionWorker(QObject):
    """[Pattern: Worker Object] Выполняет ConversionService.convert вне GUI-потока."""

    progress = pyqtSignal(str)
    succeeded = pyqtSignal(object)   # ConversionResult
    failed = pyqtSignal(str)
    finished = pyqtSignal()

    def __init__(self, service: ConversionService, path: Path, settings: ConversionSettings) -> None:
        super().__init__()
        self._service = service
        self._path = path
        self._settings = settings

    @pyqtSlot()
    def run(self) -> None:
        try:
            result = self._service.convert(self._path, self._settings, self.progress.emit)
            self.succeeded.emit(result)
        except Exception as exc:  # в GUI показываем любую ошибку, а не роняем поток
            self.failed.emit(str(exc) or exc.__class__.__name__)
        finally:
            self.finished.emit()
