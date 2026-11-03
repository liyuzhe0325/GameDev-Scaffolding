#pragma once

#include <string_view>

namespace pre {
inline constexpr int tile_pixels = 16;
inline constexpr int tile_coordinate_limit = 4096;
inline constexpr int tile_count_limit = 8192;

struct TileCell {
    int x;
    int y;
    bool operator==(const TileCell&) const = default;
};

enum class TileKind { empty, floor, wall };

inline constexpr bool valid_cell(TileCell cell) {
    return cell.x >= -tile_coordinate_limit && cell.x <= tile_coordinate_limit &&
           cell.y >= -tile_coordinate_limit && cell.y <= tile_coordinate_limit;
}

inline constexpr TileKind tile_kind(std::string_view value) {
    if (value == "floor") return TileKind::floor;
    if (value == "wall") return TileKind::wall;
    return TileKind::empty;
}
}
