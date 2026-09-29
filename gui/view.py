"""VIEW из MVC.

Паттерны:
  [MVC — View]     отображает Model и превращает действия пользователя в сигналы;
  [Observer]       View подписывается на сигналы Model (Observers = слоты _on_*);
  [Passive View]   View не содержит бизнес-логики и не вызывает сервис — только
                   испускает сигналы-намерения, а Controller их обрабатывает.
"""
from __future__ import annotations

from collections.abc import Sequence
from pathlib import Path
from typing import NamedTuple

from PyQt6.QtCore import pyqtSignal
from PyQt6.QtGui import QDragEnterEvent, QDropEvent, QFontDatabase
from PyQt6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QPlainTextEdit,
    QProgressBar,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from ..domain import ConversionResult, ConversionSettings, TrackInfo
from .model import ConverterModel
from .piano_roll import PianoRollWidget

_TRANSPOSE_AUTO = -49  # минимальное значение спинбокса показывается как «авто»


class ExporterOption(NamedTuple):
    """Лёгкое описание формата экспорта для интерфейса."""

    key: str
    label: str
    suffix: str


class MainWindow(QMainWindow):
    """[Pattern: MVC | Role: View] + [Observer | Role: Observer]"""

    # Сигналы-намерения: их слушает Controller
    open_requested = pyqtSignal(object)      # Path
    settings_edited = pyqtSignal(object)     # ConversionSettings
    convert_requested = pyqtSignal()
    save_requested = pyqtSignal(object)      # Path

    def __init__(
        self,
        model: ConverterModel,
        exporters: Sequence[ExporterOption],
        extensions: Sequence[str],
    ) -> None:
        super().__init__()
        self._model = model
        self._exporters = {o.key: o for o in exporters}
        self._extensions = tuple(extensions)

        self.setWindowTitle("song2notes — MIDI/MP3 → мелодия для пищалки")
        self.resize(920, 740)
        self.setAcceptDrops(True)

        self._build_ui(exporters)
        self._bind_model()
        self._apply_settings(model.settings)
        self._sync_enabled()

    # ------------------------------------------------------------------ UI
    def _build_ui(self, exporters: Sequence[ExporterOption]) -> None:
        root = QWidget()
        layout = QVBoxLayout(root)

        # Файл
        file_row = QHBoxLayout()
        self._file_label = QLabel("Перетащите файл сюда или нажмите «Открыть…»")
        self._open_btn = QPushButton("Открыть…")
        self._open_btn.clicked.connect(self._choose_file)
        file_row.addWidget(self._file_label, 1)
        file_row.addWidget(self._open_btn)
        layout.addLayout(file_row)

        # Параметры
        box = QGroupBox("Параметры")
        form = QFormLayout(box)

        self._track = QComboBox()
        self._start = QDoubleSpinBox()
        self._start.setRange(0.0, 36000.0)
        self._start.setDecimals(1)
        self._start.setSuffix(" с")
        self._end = QDoubleSpinBox()
        self._end.setRange(0.0, 36000.0)
        self._end.setDecimals(1)
        self._end.setSuffix(" с")
        self._end.setSpecialValueText("до конца")
        self._speed = QDoubleSpinBox()
        self._speed.setRange(0.25, 4.0)
        self._speed.setSingleStep(0.05)
        self._speed.setDecimals(2)
        self._speed.setPrefix("×")
        self._transpose = QSpinBox()
        self._transpose.setRange(_TRANSPOSE_AUTO, 48)
        self._transpose.setSpecialValueText("авто")
        self._transpose.setSuffix(" полутонов")
        self._min_note = QSpinBox()
        self._min_note.setRange(0, 1000)
        self._min_note.setSuffix(" мс")
        self._max_notes = QSpinBox()
        self._max_notes.setRange(10, 20000)
        self._pin = QSpinBox()
        self._pin.setRange(0, 28)
        self._exporter = QComboBox()
        for option in exporters:
            self._exporter.addItem(option.label, option.key)

        form.addRow("Дорожка MIDI:", self._track)
        form.addRow("Начало фрагмента:", self._start)
        form.addRow("Конец фрагмента:", self._end)
        form.addRow("Скорость:", self._speed)
        form.addRow("Транспонирование:", self._transpose)
        form.addRow("Мин. длительность ноты:", self._min_note)
        form.addRow("Макс. число нот:", self._max_notes)
        form.addRow("Пин пищалки (GP):", self._pin)
        form.addRow("Формат результата:", self._exporter)
        layout.addWidget(box)

        for spin in (self._start, self._end, self._speed):
            spin.valueChanged.connect(self._emit_settings)
        for ispin in (self._transpose, self._min_note, self._max_notes, self._pin):
            ispin.valueChanged.connect(self._emit_settings)
        self._track.currentIndexChanged.connect(self._emit_settings)
        self._exporter.currentIndexChanged.connect(self._emit_settings)

        # Предпросмотр и код
        self._roll = PianoRollWidget()
        layout.addWidget(self._roll)
        self._editor = QPlainTextEdit()
        self._editor.setReadOnly(True)
        self._editor.setFont(QFontDatabase.systemFont(QFontDatabase.SystemFont.FixedFont))
        self._editor.setPlaceholderText("Результат появится после конвертации")
        layout.addWidget(self._editor, 1)

        # Кнопки и прогресс
        bottom = QHBoxLayout()
        self._convert_btn = QPushButton("Преобразовать")
        self._convert_btn.clicked.connect(self.convert_requested)
        self._save_btn = QPushButton("Сохранить…")
        self._save_btn.clicked.connect(self._choose_save)
        self._progress = QProgressBar()
        self._progress.setRange(0, 1)
        self._progress.setTextVisible(False)
        bottom.addWidget(self._convert_btn)
        bottom.addWidget(self._save_btn)
        bottom.addWidget(self._progress, 1)
        layout.addLayout(bottom)

        self.setCentralWidget(root)

    def _bind_model(self) -> None:
        """Подписка на Model (Observer)."""
        m = self._model
        m.source_changed.connect(self._on_source)
        m.tracks_changed.connect(self._populate_tracks)
        m.settings_changed.connect(self._apply_settings)
        m.result_changed.connect(self._on_result)
        m.code_changed.connect(self._editor.setPlainText)
        m.busy_changed.connect(self._on_busy)
        m.status_changed.connect(lambda text: self.statusBar().showMessage(text))
        m.error_occurred.connect(lambda text: QMessageBox.critical(self, "Ошибка", text))

    # ------------------------------------------------- сигналы модели -> UI
    def _on_source(self, path: object) -> None:
        self._file_label.setText(path.name if isinstance(path, Path) else "Файл не выбран")
        self._sync_enabled()

    def _on_result(self, result: object) -> None:
        self._roll.set_events(result.events if isinstance(result, ConversionResult) else ())
        self._sync_enabled()

    def _on_busy(self, busy: bool) -> None:
        self._progress.setRange(0, 0 if busy else 1)  # (0, 0) — «бегущая» полоса
        self._sync_enabled()

    def _populate_tracks(self, tracks: Sequence[TrackInfo]) -> None:
        self._track.blockSignals(True)
        self._track.clear()
        self._track.addItem("Авто", None)
        for t in tracks:
            self._track.addItem(f"{t.index}: {t.name or '(без имени)'} — {t.note_count} нот", t.index)
        wanted = self._model.settings.track
        self._track.setCurrentIndex(0 if wanted is None else max(0, self._track.findData(wanted)))
        self._track.blockSignals(False)
        self._sync_enabled()

    def _apply_settings(self, settings: ConversionSettings) -> None:
        """Модель -> виджеты (сигналы блокируем, чтобы не зациклиться)."""
        widgets = (self._start, self._end, self._speed, self._transpose,
                   self._min_note, self._max_notes, self._pin, self._track, self._exporter)
        for w in widgets:
            w.blockSignals(True)
        self._start.setValue(settings.start)
        self._end.setValue(settings.end or 0.0)
        self._speed.setValue(settings.speed)
        self._transpose.setValue(_TRANSPOSE_AUTO if settings.transpose is None else settings.transpose)
        self._min_note.setValue(settings.min_note_ms)
        self._max_notes.setValue(settings.max_notes)
        self._pin.setValue(settings.pin)
        self._track.setCurrentIndex(
            0 if settings.track is None else max(0, self._track.findData(settings.track))
        )
        self._exporter.setCurrentIndex(max(0, self._exporter.findData(settings.exporter_key)))
        for w in widgets:
            w.blockSignals(False)

    def _sync_enabled(self) -> None:
        has_source = self._model.source is not None
        busy = self._model.busy
        self._open_btn.setEnabled(not busy)
        self._convert_btn.setEnabled(has_source and not busy)
        self._save_btn.setEnabled(self._model.result is not None and not busy)
        self._track.setEnabled(self._track.count() > 1 and not busy)

    # ------------------------------------------------- действия пользователя
    def _collect_settings(self) -> ConversionSettings:
        end = self._end.value()
        transpose = self._transpose.value()
        track = self._track.currentData()
        exporter_key = self._exporter.currentData()
        return ConversionSettings(
            start=self._start.value(),
            end=end if end > 0 else None,
            track=track if isinstance(track, int) else None,
            speed=self._speed.value(),
            transpose=None if transpose == _TRANSPOSE_AUTO else transpose,
            min_note_ms=self._min_note.value(),
            max_notes=self._max_notes.value(),
            pin=self._pin.value(),
            exporter_key=exporter_key if isinstance(exporter_key, str) else "micropython",
        )

    def _emit_settings(self, *_: object) -> None:
        self.settings_edited.emit(self._collect_settings())

    def _choose_file(self) -> None:
        patterns = " ".join(f"*{ext}" for ext in self._extensions)
        name, _ = QFileDialog.getOpenFileName(
            self, "Открыть композицию", "", f"Музыка ({patterns});;Все файлы (*)"
        )
        if name:
            self.open_requested.emit(Path(name))

    def _choose_save(self) -> None:
        option = self._exporters[self._model.settings.exporter_key]
        source = self._model.source
        default = f"{source.stem if source else 'melody'}_melody{option.suffix}"
        name, _ = QFileDialog.getSaveFileName(
            self, "Сохранить результат", default, f"{option.label} (*{option.suffix})"
        )
        if name:
            self.save_requested.emit(Path(name))

    # ------------------------------------------------------------ drag & drop
    def dragEnterEvent(self, event: QDragEnterEvent | None) -> None:
        if event is None:
            return
        mime = event.mimeData()
        if mime is not None and mime.hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent | None) -> None:
        if event is None:
            return
        mime = event.mimeData()
        if mime is None:
            return
        for url in mime.urls():
            if url.isLocalFile():
                self.open_requested.emit(Path(url.toLocalFile()))
                break
