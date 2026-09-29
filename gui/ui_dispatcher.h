// ui_dispatcher.h - позволяет фоновому потоку безопасно выполнить код в
// потоке интерфейса: message-only окно + PostMessage, аналог Qt::QueuedConnection.
// Используется контроллером, чтобы фоновый поток конвертации мог обновлять
// Model только через UI-поток (Model не потокобезопасна намеренно - как и Qt-объекты).
#pragma once
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace song2notes {

class UiDispatcher {
public:
    UiDispatcher();
    ~UiDispatcher();
    UiDispatcher(const UiDispatcher&) = delete;
    UiDispatcher& operator=(const UiDispatcher&) = delete;

    void post(std::function<void()> fn);

private:
    HWND hwnd_ = nullptr;
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

}  // namespace song2notes
