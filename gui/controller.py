"""CONTROLLER из MVC.

Паттерны:
  [MVC — Controller]        принимает намерения пользователя от View, вызывает
                            сервис и обновляет Model;
  [Dependency Injection]    модель и сервис передаются в конструктор;
  [Facade] (используется)   вся бизнес-логика — за ConversionService.
"""
from __future__ import annotations

from dataclasses import replace
from pathlib import Path

from PyQt6.QtCore import QObject, QThread

from ..domain import ConversionResult, ConversionSettings
from ..errors import ConversionError
from ..service import ConversionService
from .model import ConverterModel
from .worker import ConversionWorker


class ConverterController(QObject):
    """[Pattern: MVC | Role: Controller]"""

    def __init__(
        self, model: ConverterModel, service: ConversionService, parent: QObject | None = None
    ) -> None:
        super().__init__(parent)
        self._model = model
        self._service = service
        self._thread: QThread | None = None
        self._worker: ConversionWorker | None = None

    # --- намерения пользователя (слоты для сигналов View) ----------------------
    def open_file(self, path: Path) -> None:
        try:
            tracks = self._service.inspect(path)
        except ConversionError as exc:
            self._model.report_error(str(exc))
            return
        self._model.set_source(path)
        self._model.set_result(None)
        self._model.set_code("")
        self._model.set_settings(replace(self._model.settings, track=None))
        self._model.set_tracks(tracks)
        self._model.set_status(f"Открыт файл: {path.name}")

    def edit_settings(self, settings: ConversionSettings) -> None:
        old = self._model.settings
        self._model.set_settings(settings)
        # Смена формата или пина не требует повторного анализа — только перерисовки текста.
        if self._model.result and (
            old.exporter_key != settings.exporter_key or old.pin != settings.pin
        ):
            self._render()

    def convert(self) -> None:
        source = self._model.source
        if source is None or self._model.busy:
            return
        self._model.set_busy(True)
        self._model.set_status("Конвертация...")

        thread = QThread(self)
        worker = ConversionWorker(self._service, source, self._model.settings)
        worker.moveToThread(thread)
        thread.started.connect(worker.run)
        worker.progress.connect(self._model.set_status)
        worker.succeeded.connect(self._on_succeeded)
        worker.failed.connect(self._on_failed)
        worker.finished.connect(worker.deleteLater)
        worker.finished.connect(thread.quit)
        thread.finished.connect(thread.deleteLater)
        thread.finished.connect(self._on_thread_finished)
        self._thread, self._worker = thread, worker
        thread.start()

    def save(self, path: Path) -> None:
        try:
            path.write_text(self._model.code, encoding="utf-8")
        except OSError as exc:
            self._model.report_error(f"Не удалось сохранить файл: {exc}")
            return
        self._model.set_status(f"Сохранено: {path}")

    def shutdown(self) -> None:
        """Аккуратно останавливает фоновый поток при выходе из приложения."""
        thread = self._thread
        if thread is not None:
            thread.quit()
            if not thread.wait(3000):
                thread.terminate()
                thread.wait()

    # --- реакции на воркер -------------------------------------------------------
    def _on_succeeded(self, result: object) -> None:
        assert isinstance(result, ConversionResult)
        self._model.set_result(result)
        self._render()
        note = " (обрезано по лимиту нот)" if result.truncated else ""
        self._model.set_status(
            f"Готово: {len(result.events)} событий, ~{result.total_ms / 1000:.1f} с{note}"
        )

    def _on_failed(self, message: str) -> None:
        self._model.set_status("Ошибка")
        self._model.report_error(message)

    def _on_thread_finished(self) -> None:
        self._thread = None
        self._worker = None
        self._model.set_busy(False)

    def _render(self) -> None:
        result, source = self._model.result, self._model.source
        if result is None or source is None:
            return
        try:
            code = self._service.render(result.events, self._model.settings, source.name)
        except ConversionError as exc:
            self._model.report_error(str(exc))
            return
        self._model.set_code(code)
