// csv_exporter.h - экспорт в таблицу CSV (для отладки и других языков/плат).
// [Pattern: Template Method | Role: ConcreteClass]
#pragma once
#include "exporter.h"

namespace song2notes {

class CsvExporter : public Exporter {
public:
    std::string key() const override { return "csv"; }
    std::string label() const override { return "Таблица CSV (.csv)"; }
    std::string file_suffix() const override { return ".csv"; }

protected:
    std::string header(const std::vector<NoteEvent>& events, const ConversionSettings& settings,
                        const std::string& source_name) const override;
    std::string body(const std::vector<NoteEvent>& events, const ConversionSettings& settings) const override;
};

}  // namespace song2notes
