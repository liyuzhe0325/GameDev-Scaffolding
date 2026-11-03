#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>

namespace pre {
class Localization {
public:
    explicit Localization(const std::filesystem::path& directory);
    const char* text(const char* key, bool chinese) const;
    bool has_chinese() const { return !chinese_.empty(); }

private:
    nlohmann::json english_;
    nlohmann::json chinese_;
};
}
