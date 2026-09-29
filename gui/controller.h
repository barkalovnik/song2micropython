// controller.h - CONTROLLER из MVC.
// [Pattern: MVC | Role: Controller] принимает намерения пользователя от View,
// вызывает сервис (Facade) и обновляет Model.
// [Pattern: Dependency Injection] модель, сервис и диспетчер передаются в конструктор.
// [Pattern: Worker Object] convert() запускает std::thread, результат
// возвращается в UI-поток через UiDispatcher.
#pragma once
#include <thread>

#include "../core/service.h"
#include "model.h"
#include "ui_dispatcher.h"

namespace song2notes {

class ConverterController {
public:
    ConverterController(ConverterModel& model, ConversionService& service, UiDispatcher& ui);
    ~ConverterController();

    void open_file(const std::wstring& path);
    void edit_settings(const ConversionSettings& settings);
    void convert();
    void save(const std::wstring& path);

private:
    ConverterModel& model_;
    ConversionService& service_;
    UiDispatcher& ui_;
    std::thread worker_;

    void join_worker();
    void render();
};

}  // namespace song2notes
