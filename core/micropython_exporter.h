// micropython_exporter.h - экспорт в готовый скрипт MicroPython для Pico.
// [Pattern: Template Method | Role: ConcreteClass]
#pragma once
#include "exporter.h"

namespace song2notes {

class MicroPythonExporter : public Exporter {
public:
    std::string key() const override { return "micropython"; }
    std::string label() const override { return "Скрипт MicroPython (.py)"; }
    std::string file_suffix() const override { return ".py"; }

protected:
    std::string header(const std::vector<NoteEvent>& events, const ConversionSettings& settings,
                        const std::string& source_name) const override;
    std::string body(const std::vector<NoteEvent>& events, const ConversionSettings& settings) const override;
    std::string footer(const std::vector<NoteEvent>& events, const ConversionSettings& settings) const override;
};

}  // namespace song2notes
