#pragma once
#include <string_view>
namespace pre {
inline bool is_script_reference(std::string_view ref) {
    if (!ref.starts_with("scripts/") || !ref.ends_with(".lua") || ref.size() <= 12 || ref.size() > 120)
        return false;
    for (const char c : ref.substr(8, ref.size() - 12)) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }
    return true;
}
}