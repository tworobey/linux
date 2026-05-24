#pragma once

namespace geom {

struct Point2D {
    double x = 0.0;
    double y = 0.0;

    Point2D() = default;
    Point2D(double x, double y) : x(x), y(y) {}

    Point2D operator+(const Point2D& other) const {
        return {x + other.x, y + other.y};
    }

    Point2D operator-(const Point2D& other) const {
        return {x - other.x, y - other.y};
    }

    Point2D operator*(double scalar) const {
        return {x * scalar, y * scalar};
    }
};

struct Vec2D {
    double x = 0.0;
    double y = 0.0;

    Vec2D() = default;
    Vec2D(double x, double y) : x(x), y(y) {}
};

} // namespace geom
