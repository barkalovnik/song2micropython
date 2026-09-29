// view.h - VIEW из MVC.
// [Pattern: MVC | Role: View] отображает Model и превращает действия
// пользователя в сигналы-намерения (Passive View: сама View не вызывает
// сервис и не содержит бизнес-логики - этим занимается Controller).
// [Pattern: Observer | Role: Observer] подписывается на сигналы Model.
#pragma once
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../core/domain.h"
#include "model.h"
#include "signal.h"

namespace song2notes {

class MainWindow {
public:
    // Сигналы-намерения: их слушает Controller (View -> Controller).
    Signal<std::wstring> open_requested;
    Signal<ConversionSettings> settings_edited;
    Signal<> convert_requested;
    Signal<std::wstring> save_requested;

    bool create(HINSTANCE instance);
    void show(int cmd_show);
    void bind_model(ConverterModel& model);

private:
    HWND hwnd_ = nullptr;
    ConverterModel* model_ = nullptr;
    bool applying_settings_ = false;  // подавляет реэмиссию settings_edited при обновлении из модели

    // Дочерние элементы управления.
    HWND file_label_ = nullptr;
    HWND open_btn_ = nullptr;
    HWND track_combo_ = nullptr;
    HWND start_edit_ = nullptr;
    HWND end_edit_ = nullptr;
    HWND speed_edit_ = nullptr;
    HWND transpose_edit_ = nullptr;
    HWND min_note_edit_ = nullptr;
    HWND max_notes_edit_ = nullptr;
    HWND pin_edit_ = nullptr;
    HWND format_combo_ = nullptr;
    HWND code_edit_ = nullptr;
    HWND convert_btn_ = nullptr;
    HWND save_btn_ = nullptr;
    HWND status_label_ = nullptr;

    void build_controls(HINSTANCE instance);
    void layout(int client_width, int client_height);

    void on_command(WPARAM wparam, LPARAM lparam);
    void choose_file();
    void choose_save();
    void emit_settings();
    ConversionSettings collect_settings() const;
    void apply_settings(const ConversionSettings& settings);
    void sync_enabled();

    // Реакции на изменения модели (Observer).
    void on_source_changed(const std::wstring& path);
    void on_tracks_changed(const std::vector<TrackInfo>& tracks);
    void on_result_changed(bool has_result);
    void on_code_changed(const std::string& code);
    void on_busy_changed(bool busy);
    void on_status_changed(const std::wstring& text);
    void on_error(const std::wstring& text);

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

}  // namespace song2notes
