#include "controller.h"

#include "../core/errors.h"
#include "../core/io_utils.h"
#include "../core/text_convert.h"

namespace song2notes {

ConverterController::ConverterController(ConverterModel& model, ConversionService& service, UiDispatcher& ui)
    : model_(model), service_(service), ui_(ui) {}

ConverterController::~ConverterController() { join_worker(); }

void ConverterController::join_worker() {
    if (worker_.joinable()) worker_.join();
}

void ConverterController::open_file(const std::wstring& path) {
    try {
        std::vector<TrackInfo> tracks = service_.inspect(path);
        model_.set_source(path);
        model_.set_result(std::nullopt);
        model_.set_code("");
        ConversionSettings s = model_.settings();
        s.track.reset();
        model_.set_settings(s);
        model_.set_tracks(std::move(tracks));
        model_.set_status(L"Открыт файл: " + filename_only(path));
    } catch (const ConversionError& e) {
        model_.report_error(widen_utf8(e.what()));
    }
}

void ConverterController::edit_settings(const ConversionSettings& settings) {
    ConversionSettings old = model_.settings();
    model_.set_settings(settings);
    // Смена формата или пина не требует повторного анализа файла - только перерисовки текста.
    if (model_.result() != nullptr &&
        (old.exporter_key != settings.exporter_key || old.pin != settings.pin)) {
        render();
    }
}

void ConverterController::convert() {
    if (model_.source().empty() || model_.busy()) return;
    join_worker();  // на случай, если предыдущий воркер ещё не был дождан
    model_.set_busy(true);
    model_.set_status(L"Конвертация...");

    std::wstring path = model_.source();
    ConversionSettings settings = model_.settings();

    worker_ = std::thread([this, path, settings]() {
        try {
            ConversionResult result = service_.convert(path, settings, [this](const std::wstring& msg) {
                ui_.post([this, msg]() { model_.set_status(msg); });
            });
            ui_.post([this, result]() {
                model_.set_result(result);
                render();
                std::wstring note = result.truncated ? L" (обрезано по лимиту нот)" : L"";
                model_.set_status(L"Готово: " + std::to_wstring(result.events.size()) + L" событий, ~" +
                                   std::to_wstring(result.total_ms() / 1000.0) + L" с" + note);
                model_.set_busy(false);
            });
        } catch (const ConversionError& e) {
            std::wstring msg = widen_utf8(e.what());
            ui_.post([this, msg]() {
                model_.set_status(L"Ошибка");
                model_.report_error(msg);
                model_.set_busy(false);
            });
        }
    });
}

void ConverterController::save(const std::wstring& path) {
    try {
        write_file_bytes(path, model_.code());
        model_.set_status(L"Сохранено: " + path);
    } catch (const ConversionError& e) {
        model_.report_error(widen_utf8(e.what()));
    }
}

void ConverterController::render() {
    const ConversionResult* result = model_.result();
    if (result == nullptr || model_.source().empty()) return;
    try {
        std::string name = narrow_utf8(filename_only(model_.source()));
        std::string code = service_.render(result->events, model_.settings(), name);
        model_.set_code(std::move(code));
    } catch (const ConversionError& e) {
        model_.report_error(widen_utf8(e.what()));
    }
}

}  // namespace song2notes
