// signal.h - минимальная реализация паттерна Observer в духе Qt-сигналов,
// без внешних зависимостей. Subject (например, Model) объявляет поля Signal<...>,
// Observer подписывается через connect().
#pragma once
#include <functional>
#include <vector>

namespace song2notes {

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    void connect(Slot slot) { slots_.push_back(std::move(slot)); }

    void emit(Args... args) const {
        for (const auto& slot : slots_) slot(args...);
    }

private:
    std::vector<Slot> slots_;
};

}  // namespace song2notes
