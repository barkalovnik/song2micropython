#include "view.h"

#include <commdlg.h>

#include <algorithm>

#include "../core/exporter_registry.h"
#include "../core/text_convert.h"
#include "format_helpers.h"

namespace song2notes {

namespace {

// Идентификаторы дочерних элементов управления.
enum ControlId : int {
    kOpenBtn = 1001,
    kTrackCombo,
    kStartEdit,
    kEndEdit,
    kSpeedEdit,
    kTransposeEdit,
    kMinNoteEdit,
    kMaxNotesEdit,
    kPinEdit,
    kFormatCombo,
    kCodeEdit,
    kConvertBtn,
    kSaveBtn,
};

constexpr int kMargin = 12;
constexpr int kLabelWidth = 170;
constexpr int kFieldWidth = 220;
constexpr int kRowHeight = 24;
constexpr int kWindowWidth = 760;
constexpr int kWindowHeight = 720;

std::wstring track_label(const TrackInfo& t) {
    std::wstring name = t.name.empty() ? L"(без имени)" : t.name;
    return std::to_wstring(t.index) + L": " + name + L" - " + std::to_wstring(t.note_count) + L" нот";
}

HWND create_label(HWND parent, HINSTANCE inst, const wchar_t* text, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, inst,
                            nullptr);
}

HWND create_edit(HWND parent, HINSTANCE inst, int id, int x, int y, int w, int h) {
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, x, y, w,
                            h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
}

HWND create_combo(HWND parent, HINSTANCE inst, int id, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, x, y,
                            w, h * 8, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst,
                            nullptr);
}

HWND create_button(HWND parent, HINSTANCE inst, int id, const wchar_t* text, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, y, w, h, parent,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
}

struct FontCtx {
    HWND skip;
    HFONT font;
};

BOOL CALLBACK apply_font_proc(HWND h, LPARAM lp) {
    auto* c = reinterpret_cast<FontCtx*>(lp);
    if (h != c->skip) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(c->font), TRUE);
    return TRUE;
}

}  // namespace

bool MainWindow::create(HINSTANCE instance) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"song2notes.MainWindow";
    if (!RegisterClassW(&wc)) return false;

    const DWORD style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
    // Раскладка контролов рассчитана на клиентскую область kWindowWidth x kWindowHeight,
    // поэтому внешний размер окна считаем с учётом рамки и заголовка.
    RECT rc{0, 0, kWindowWidth, kWindowHeight};
    AdjustWindowRectEx(&rc, style, FALSE, 0);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"song2notes — MIDI → мелодия для пищалки", style,
                                 CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                                 nullptr, nullptr, instance, this);
    return hwnd != nullptr && hwnd_ != nullptr;
}

void MainWindow::show(int cmd_show) {
    ShowWindow(hwnd_, cmd_show);
    UpdateWindow(hwnd_);
}

void MainWindow::build_controls(HINSTANCE instance) {
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    int y = kMargin;

    // Строка выбора файла.
    file_label_ = create_label(hwnd_, instance, L"Файл не выбран", kMargin, y + 4,
                                kWindowWidth - 2 * kMargin - 120, kRowHeight);
    open_btn_ = create_button(hwnd_, instance, kOpenBtn, L"Открыть\u2026", kWindowWidth - kMargin - 110, y,
                               110, kRowHeight);
    y += kRowHeight + kMargin;

    auto add_row = [&](const wchar_t* label, int id, bool combo) -> HWND {
        create_label(hwnd_, instance, label, kMargin, y + 3, kLabelWidth, kRowHeight);
        HWND ctrl = combo ? create_combo(hwnd_, instance, id, kMargin + kLabelWidth, y, kFieldWidth, kRowHeight)
                           : create_edit(hwnd_, instance, id, kMargin + kLabelWidth, y, kFieldWidth, kRowHeight);
        y += kRowHeight + 6;
        return ctrl;
    };

    track_combo_ = add_row(L"Дорожка MIDI:", kTrackCombo, true);
    start_edit_ = add_row(L"Начало фрагмента (с):", kStartEdit, false);
    end_edit_ = add_row(L"Конец фрагмента (с, пусто = до конца):", kEndEdit, false);
    speed_edit_ = add_row(L"Скорость (×):", kSpeedEdit, false);
    transpose_edit_ = add_row(L"Транспонирование (полутоны, пусто = авто):", kTransposeEdit, false);
    min_note_edit_ = add_row(L"Мин. длительность ноты (мс):", kMinNoteEdit, false);
    max_notes_edit_ = add_row(L"Макс. число нот:", kMaxNotesEdit, false);
    pin_edit_ = add_row(L"Пин пищалки (GP):", kPinEdit, false);
    format_combo_ = add_row(L"Формат результата:", kFormatCombo, true);

    for (const Exporter* exporter : all_exporters()) {
        std::wstring label = widen_utf8(exporter->label());
        SendMessageW(format_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    SendMessageW(track_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Авто"));

    y += kMargin - 6;
    int bottom_h = 90;  // кнопки + статус внизу окна
    int code_top = y;
    int code_height = kWindowHeight - code_top - bottom_h - kMargin;
    code_edit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY |
                                      ES_AUTOVSCROLL,
                                  kMargin, code_top, kWindowWidth - 2 * kMargin, code_height, hwnd_,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCodeEdit)), instance,
                                  nullptr);
    SendMessageW(code_edit_, EM_SETLIMITTEXT, 0, 0);
    HFONT mono = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH,
                              L"Consolas");
    SendMessageW(code_edit_, WM_SETFONT, reinterpret_cast<WPARAM>(mono), TRUE);

    int buttons_top = code_top + code_height + 10;
    convert_btn_ = create_button(hwnd_, instance, kConvertBtn, L"Преобразовать", kMargin, buttons_top, 150,
                                  kRowHeight + 6);
    save_btn_ = create_button(hwnd_, instance, kSaveBtn, L"Сохранить\u2026", kMargin + 160, buttons_top, 150,
                               kRowHeight + 6);
    status_label_ = create_label(hwnd_, instance, L"Готов к работе.", kMargin, buttons_top + kRowHeight + 14,
                                  kWindowWidth - 2 * kMargin, kRowHeight);

    // Единый шрифт на все контролы (включая безымянные подписи), кроме поля с кодом.
    FontCtx ctx{code_edit_, font};
    EnumChildWindows(hwnd_, apply_font_proc, reinterpret_cast<LPARAM>(&ctx));
}

void MainWindow::bind_model(ConverterModel& model) {
    model_ = &model;
    model.source_changed.connect([this](const std::wstring& p) { on_source_changed(p); });
    model.tracks_changed.connect([this](const std::vector<TrackInfo>& t) { on_tracks_changed(t); });
    model.settings_changed.connect([this](const ConversionSettings& s) { apply_settings(s); });
    model.has_result_changed.connect([this](bool has) { on_result_changed(has); });
    model.code_changed.connect([this](const std::string& c) { on_code_changed(c); });
    model.busy_changed.connect([this](bool b) { on_busy_changed(b); });
    model.status_changed.connect([this](const std::wstring& s) { on_status_changed(s); });
    model.error_occurred.connect([this](const std::wstring& s) { on_error(s); });

    apply_settings(model.settings());
    sync_enabled();
}

ConversionSettings MainWindow::collect_settings() const {
    auto text_of = [](HWND h) {
        int len = GetWindowTextLengthW(h);
        std::wstring buf(static_cast<size_t>(len), L'\0');
        if (len > 0) GetWindowTextW(h, buf.data(), len + 1);
        return buf;
    };

    ConversionSettings s;
    s.start = parse_double(text_of(start_edit_), 0.0);
    s.end = parse_optional_double(text_of(end_edit_));
    s.speed = parse_double(text_of(speed_edit_), 1.0);
    s.transpose = parse_optional_int(text_of(transpose_edit_));
    s.min_note_ms = parse_int(text_of(min_note_edit_), 60);
    s.max_notes = parse_int(text_of(max_notes_edit_), 1500);
    s.pin = parse_int(text_of(pin_edit_), 26);

    int track_idx = static_cast<int>(SendMessageW(track_combo_, CB_GETCURSEL, 0, 0));
    if (track_idx > 0 && model_ != nullptr) {
        const auto& tracks = model_->tracks();
        size_t pos = static_cast<size_t>(track_idx - 1);
        if (pos < tracks.size()) s.track = tracks[pos].index;
    }

    int format_idx = static_cast<int>(SendMessageW(format_combo_, CB_GETCURSEL, 0, 0));
    auto exporters = all_exporters();
    if (format_idx >= 0 && static_cast<size_t>(format_idx) < exporters.size()) {
        s.exporter_key = exporters[static_cast<size_t>(format_idx)]->key();
    }
    return s;
}

void MainWindow::apply_settings(const ConversionSettings& settings) {
    applying_settings_ = true;
    SetWindowTextW(start_edit_, format_double(settings.start, 1).c_str());
    SetWindowTextW(end_edit_, format_optional_double(settings.end, 1).c_str());
    SetWindowTextW(speed_edit_, format_double(settings.speed, 2).c_str());
    SetWindowTextW(transpose_edit_, format_optional_int(settings.transpose).c_str());
    SetWindowTextW(min_note_edit_, format_int(settings.min_note_ms).c_str());
    SetWindowTextW(max_notes_edit_, format_int(settings.max_notes).c_str());
    SetWindowTextW(pin_edit_, format_int(settings.pin).c_str());

    int track_sel = 0;
    if (settings.track.has_value() && model_ != nullptr) {
        const auto& tracks = model_->tracks();
        for (size_t i = 0; i < tracks.size(); ++i) {
            if (tracks[i].index == *settings.track) {
                track_sel = static_cast<int>(i) + 1;
                break;
            }
        }
    }
    SendMessageW(track_combo_, CB_SETCURSEL, static_cast<WPARAM>(track_sel), 0);

    auto exporters = all_exporters();
    int format_sel = 0;
    for (size_t i = 0; i < exporters.size(); ++i) {
        if (exporters[i]->key() == settings.exporter_key) {
            format_sel = static_cast<int>(i);
            break;
        }
    }
    SendMessageW(format_combo_, CB_SETCURSEL, static_cast<WPARAM>(format_sel), 0);
    applying_settings_ = false;
}

void MainWindow::emit_settings() {
    if (applying_settings_) return;
    settings_edited.emit(collect_settings());
}

void MainWindow::sync_enabled() {
    if (model_ == nullptr) return;
    bool has_source = !model_->source().empty();
    bool busy = model_->busy();
    EnableWindow(open_btn_, !busy);
    EnableWindow(convert_btn_, has_source && !busy);
    EnableWindow(save_btn_, model_->result() != nullptr && !busy);

    int track_count = static_cast<int>(SendMessageW(track_combo_, CB_GETCOUNT, 0, 0));
    EnableWindow(track_combo_, track_count > 1 && !busy);
}

void MainWindow::choose_file() {
    wchar_t buffer[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"MIDI-файлы (*.mid;*.midi)\0*.mid;*.midi\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Открыть MIDI-композицию";
    if (GetOpenFileNameW(&ofn)) {
        open_requested.emit(std::wstring(buffer));
    }
}

void MainWindow::choose_save() {
    if (model_ == nullptr) return;
    const Exporter& exporter = get_exporter(model_->settings().exporter_key);
    std::wstring suffix = widen_utf8(exporter.file_suffix());
    std::wstring default_name = L"melody" + suffix;
    if (!model_->source().empty()) {
        std::wstring src = model_->source();
        size_t slash = src.find_last_of(L"\\/");
        size_t dot = src.find_last_of(L'.');
        std::wstring stem = (dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash))
                                 ? src.substr(slash == std::wstring::npos ? 0 : slash + 1,
                                               dot - (slash == std::wstring::npos ? 0 : slash + 1))
                                 : src.substr(slash == std::wstring::npos ? 0 : slash + 1);
        default_name = stem + L"_melody" + suffix;
    }

    wchar_t buffer[MAX_PATH];
    wcsncpy(buffer, default_name.c_str(), MAX_PATH - 1);
    buffer[MAX_PATH - 1] = L'\0';

    std::wstring filter_label = widen_utf8(exporter.label());
    std::wstring filter = filter_label + L'\0' + L"*" + suffix + L'\0';

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = suffix.c_str() + 1;  // без точки
    ofn.lpstrTitle = L"Сохранить результат";
    if (GetSaveFileNameW(&ofn)) {
        save_requested.emit(std::wstring(buffer));
    }
}

void MainWindow::on_command(WPARAM wparam, LPARAM lparam) {
    int id = LOWORD(wparam);
    int code = HIWORD(wparam);
    HWND ctrl = reinterpret_cast<HWND>(lparam);
    (void)ctrl;

    switch (id) {
        case kOpenBtn:
            if (code == BN_CLICKED) choose_file();
            break;
        case kConvertBtn:
            if (code == BN_CLICKED) convert_requested.emit();
            break;
        case kSaveBtn:
            if (code == BN_CLICKED) choose_save();
            break;
        case kTrackCombo:
        case kFormatCombo:
            if (code == CBN_SELCHANGE) emit_settings();
            break;
        case kStartEdit:
        case kEndEdit:
        case kSpeedEdit:
        case kTransposeEdit:
        case kMinNoteEdit:
        case kMaxNotesEdit:
        case kPinEdit:
            if (code == EN_CHANGE) emit_settings();
            break;
        default:
            break;
    }
}

void MainWindow::on_source_changed(const std::wstring& path) {
    std::wstring text = path.empty() ? L"Файл не выбран" : filename_only(path);
    SetWindowTextW(file_label_, text.c_str());
    sync_enabled();
}

void MainWindow::on_tracks_changed(const std::vector<TrackInfo>& tracks) {
    SendMessageW(track_combo_, CB_RESETCONTENT, 0, 0);
    SendMessageW(track_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Авто"));
    for (const auto& t : tracks) {
        std::wstring label = track_label(t);
        SendMessageW(track_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    int sel = 0;
    if (model_ != nullptr && model_->settings().track.has_value()) {
        for (size_t i = 0; i < tracks.size(); ++i) {
            if (tracks[i].index == *model_->settings().track) {
                sel = static_cast<int>(i) + 1;
                break;
            }
        }
    }
    SendMessageW(track_combo_, CB_SETCURSEL, static_cast<WPARAM>(sel), 0);
    sync_enabled();
}

void MainWindow::on_result_changed(bool) { sync_enabled(); }

void MainWindow::on_code_changed(const std::string& code) {
    SetWindowTextW(code_edit_, widen_utf8(code).c_str());
}

void MainWindow::on_busy_changed(bool) { sync_enabled(); }

void MainWindow::on_status_changed(const std::wstring& text) { SetWindowTextW(status_label_, text.c_str()); }

void MainWindow::on_error(const std::wstring& text) {
    MessageBoxW(hwnd_, text.c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK MainWindow::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    MainWindow* self;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        // CreateWindowExW ещё не вернулся, поэтому hwnd_ пока не присвоен, а WM_CREATE
        // (build_controls) уже использует его как родителя для контролов.
        if (self) self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    switch (msg) {
        case WM_CREATE:
            if (self) {
                HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                self->build_controls(inst);
            }
            return 0;
        case WM_COMMAND:
            if (self) self->on_command(wparam, lparam);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

}  // namespace song2notes