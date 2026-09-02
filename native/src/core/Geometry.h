#pragma once

struct PointF {
    float x;
    float y;
};

struct SizeF {
    float width;
    float height;
};

struct RectF {
    float left;
    float top;
    float right;
    float bottom;

    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
};

struct QuadF {
    PointF p1;
    PointF p2;
    PointF p3;
    PointF p4;
};

struct Matrix3x2F {
    float a, b, c, d, e, f;
};

