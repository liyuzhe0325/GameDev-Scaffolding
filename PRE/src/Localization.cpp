#include "Localization.h"

#include <fstream>
#include <utility>

namespace pre {
namespace {
nlohmann::json read_catalog(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) return nlohmann::json::object();
    auto catalog = nlohmann::json::parse(input, nullptr, false);
    return catalog.is_object() ? std::move(catalog) : nlohmann::json::object();
}

const char* lookup(const nlohmann::json& catalog, const char* key) {
    const auto entry = catalog.find(key);
    if (entry == catalog.end()) return nullptr;
    const auto* value = entry->get_ptr<const std::string*>();
    return value && !value->empty() ? value->c_str() : nullptr;
}
}

Localization::Localization(const std::filesystem::path& directory)
    : english_(read_catalog(directory / "en.json")),
      chinese_(read_catalog(directory / "zh-CN.json")) {}

const char* Localization::text(const char* key, bool chinese) const {
    if (chinese) {
        if (const char* translated = lookup(chinese_, key)) return translated;
    }
    if (const char* source = lookup(english_, key)) return source;
    return key;
}
}
