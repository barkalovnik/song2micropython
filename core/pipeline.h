// pipeline.h - конвейер обработки сегментов (обрезка фрагмента, отсечение
// шума, подгонка диапазона).
// [Pattern: Pipeline / Pipes-and-Filters] Pipeline последовательно применяет шаги.
// [Pattern: Strategy] каждый ProcessingStep взаимозаменяем.
#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "domain.h"

namespace song2notes {

// [Pattern: Pipeline | Role: Filter]
class ProcessingStep {
public:
    virtual ~ProcessingStep() = default;
    virtual std::vector<NoteSegment> process(const std::vector<NoteSegment>& segments) const = 0;
};

// [Pattern: Pipeline | Role: ConcreteFilter] Вырезает [start, end) и сдвигает к нулю.
class CropStep : public ProcessingStep {
public:
    CropStep(double start, std::optional<double> end) : start_(start), end_(end) {}
    std::vector<NoteSegment> process(const std::vector<NoteSegment>& segments) const override;

private:
    double start_;
    std::optional<double> end_;
};

// [Pattern: Pipeline | Role: ConcreteFilter] Отбрасывает слишком короткие ноты (шум).
class MinDurationStep : public ProcessingStep {
public:
    explicit MinDurationStep(int min_ms) : min_seconds_(min_ms / 1000.0) {}
    std::vector<NoteSegment> process(const std::vector<NoteSegment>& segments) const override;

private:
    double min_seconds_;
};

// [Pattern: Pipeline | Role: ConcreteFilter] Транспонирует и «сворачивает» ноты
// по октавам в диапазон, где пищалка звучит нормально.
class FitRangeStep : public ProcessingStep {
public:
    explicit FitRangeStep(std::optional<int> transpose) : transpose_(transpose) {}
    std::vector<NoteSegment> process(const std::vector<NoteSegment>& segments) const override;

private:
    std::optional<int> transpose_;
    static int fold(int pitch);
    static int best_shift(const std::vector<NoteSegment>& segments);
};

// [Pattern: Pipeline | Role: Pipeline]
class Pipeline {
public:
    explicit Pipeline(std::vector<std::unique_ptr<ProcessingStep>> steps) : steps_(std::move(steps)) {}
    std::vector<NoteSegment> run(std::vector<NoteSegment> segments) const {
        for (const auto& step : steps_) segments = step->process(segments);
        return segments;
    }

private:
    std::vector<std::unique_ptr<ProcessingStep>> steps_;
};

// [Pattern: Factory Function] Собирает конвейер под заданные настройки.
Pipeline build_pipeline(const ConversionSettings& settings);

}  // namespace song2notes
