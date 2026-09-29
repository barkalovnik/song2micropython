// main.cpp - точка входа (WinMain) и Composition Root приложения.
// [Pattern: MVC] здесь создаются и связываются Model, View, Controller.
// [Pattern: Dependency Injection] все зависимости создаются в одном месте
// и передаются друг другу через конструкторы/сигналы.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "core/service.h"
#include "gui/controller.h"
#include "gui/model.h"
#include "gui/ui_dispatcher.h"
#include "gui/view.h"

using namespace song2notes;

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmd_show) {
    ConversionService service;
    ConverterModel model;
    UiDispatcher dispatcher;
    ConverterController controller(model, service, dispatcher);

    MainWindow view;
    if (!view.create(instance)) return 1;
    view.bind_model(model);

    // View -> Controller (намерения пользователя).
    view.open_requested.connect([&](const std::wstring& path) { controller.open_file(path); });
    view.settings_edited.connect([&](const ConversionSettings& s) { controller.edit_settings(s); });
    view.convert_requested.connect([&]() { controller.convert(); });
    view.save_requested.connect([&](const std::wstring& path) { controller.save(path); });

    view.show(cmd_show);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
