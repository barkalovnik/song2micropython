// model.h - MODEL из MVC.
// [Pattern: MVC | Role: Model] хранит состояние приложения и ничего не знает
// ни о View, ни о Controller.
// [Pattern: Observer | Role: Subject] любое изменение рассылается через Signal.
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "../core/domain.h"
#include "signal.h"

namespace song2notes {

class ConverterModel {
public:
    Signal<std::wstring> source_changed;             // путь к файлу ("" если закрыт)
    Signal<std::vector<TrackInfo>> tracks_changed;
    Signal<ConversionSettings> settings_changed;
    Signal<bool> has_result_changed;                  // появился/исчез результат
    Signal<std::string> code_changed;                 // текст результата (UTF-8)
    Signal<bool> busy_changed;
    Signal<std::wstring> status_changed;
    Signal<std::wstring> error_occurred;

    const std::wstring& source() const { return source_; }
    const std::vector<TrackInfo>& tracks() const { return tracks_; }
    const ConversionSettings& settings() const { return settings_; }
    const ConversionResult* result() const { return result_.has_value() ? &*result_ : nullptr; }
    const std::string& code() const { return code_; }
    bool busy() const { return busy_; }

    void set_source(std::wstring path) {
        source_ = std::move(path);
        source_changed.emit(source_);
    }
    void set_tracks(std::vector<TrackInfo> tracks) {
        tracks_ = std::move(tracks);
        tracks_changed.emit(tracks_);
    }
    void set_settings(ConversionSettings s) {
        settings_ = s;
        settings_changed.emit(settings_);
    }
    void set_result(std::optional<ConversionResult> r) {
        result_ = std::move(r);
        has_result_changed.emit(result_.has_value());
    }
    void set_code(std::string c) {
        code_ = std::move(c);
        code_changed.emit(code_);
    }
    void set_busy(bool b) {
        if (b == busy_) return;
        busy_ = b;
        busy_changed.emit(busy_);
    }
    void set_status(std::wstring s) { status_changed.emit(s); }
    void report_error(std::wstring s) { error_occurred.emit(s); }

private:
    std::wstring source_;
    std::vector<TrackInfo> tracks_;
    ConversionSettings settings_;
    std::optional<ConversionResult> result_;
    std::string code_;
    bool busy_ = false;
};

}  // namespace song2notes
