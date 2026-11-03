#pragma once

#include "SceneDocument.h"
#include <array>
#include <memory>
#include <string_view>
#include <vector>

namespace pre {
struct RuntimeInput {
    static constexpr std::array<std::string_view, 31> keys = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
        "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
        "left", "right", "up", "down", "space"
    };
    std::array<bool, keys.size()> down{};
};

class SceneRuntime {
public:
    SceneRuntime();
    ~SceneRuntime();
    void play(const SceneDocument& source);
    void stop();
    void tick(float delta_seconds, const RuntimeInput& input);
    const SceneDocument* scene() const { return scene_ ? &*scene_ : nullptr; }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    struct Script;
    std::optional<SceneDocument> scene_;
    std::vector<std::unique_ptr<Script>> scripts_;
    std::vector<std::string> errors_;
};
}
