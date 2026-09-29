#include "ui_dispatcher.h"

namespace song2notes {

namespace {
constexpr UINT kDispatchMessage = WM_APP + 1;
const wchar_t* kClassName = L"song2notes.UiDispatcher";
}  // namespace

LRESULT CALLBACK UiDispatcher::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == kDispatchMessage) {
        auto* fn = reinterpret_cast<std::function<void()>*>(lparam);
        (*fn)();
        delete fn;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

UiDispatcher::UiDispatcher() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kClassName;
    RegisterClassW(&wc);
    hwnd_ = CreateWindowExW(0, kClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
}

UiDispatcher::~UiDispatcher() {
    if (hwnd_) DestroyWindow(hwnd_);
}

void UiDispatcher::post(std::function<void()> fn) {
    auto* heap_fn = new std::function<void()>(std::move(fn));
    PostMessageW(hwnd_, kDispatchMessage, 0, reinterpret_cast<LPARAM>(heap_fn));
}

}  // namespace song2notes
