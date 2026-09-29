"""Точка сборки приложения (Composition Root).

Паттерны:
  [MVC]                  здесь создаются и связываются Model, View, Controller;
  [Dependency Injection] все зависимости создаются в одном месте и передаются
                         объектам через конструкторы.
"""
from __future__ import annotations

import sys

from PyQt6.QtWidgets import QApplication

from ..exporters import all_exporters
from ..service import ConversionService
from .controller import ConverterController
from .model import ConverterModel
from .view import ExporterOption, MainWindow


def run() -> int:
    app = QApplication(sys.argv)

    service = ConversionService()
    model = ConverterModel()
    view = MainWindow(
        model,
        exporters=[ExporterOption(e.key, e.label, e.file_suffix) for e in all_exporters()],
        extensions=service.supported_extensions(),
    )
    controller = ConverterController(model, service)

    # View -> Controller (намерения пользователя)
    view.open_requested.connect(controller.open_file)
    view.settings_edited.connect(controller.edit_settings)
    view.convert_requested.connect(controller.convert)
    view.save_requested.connect(controller.save)

    app.aboutToQuit.connect(controller.shutdown)
    view.show()
    return app.exec()
