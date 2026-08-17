#pragma once
#include <cmath>

namespace drayven {
struct Vec2 { float x{0}, y{0}; };
struct Vec3 { float x{0}, y{0}, z{0}; };
struct Vec4 { float x{0}, y{0}, z{0}, w{0}; };
struct Color { float r{1}, g{1}, b{1}, a{1}; };
struct Transform {
    Vec3 position{};
    Vec3 rotation{};
    Vec3 scale{1,1,1};
};
}
