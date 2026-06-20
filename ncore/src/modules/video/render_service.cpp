#include <ncore/modules/video/render_service.h>

#include <cfloat>
#include <cmath>
#include <numbers>

namespace ncore {

void IRenderService::draw_circle(float x, float y, float radius, Color color, bool filled, bool edge) {
    set_draw_color(color);

    if (filled) {
        // Approximate filled circle using many points
        for (int w = -radius; w <= radius; w++) {
            for (int h = -radius; h <= radius; h++) {
                if (w * w + h * h <= radius * radius) {
                    draw_point(x + w, y + h);
                }
            }
        }

        if (edge) {
            set_draw_color(Color(255, 255, 255, 150));
            for (int i = 0; i < 360; i++) {
                float angle = i * std::numbers::pi_v<float> / 180.0f;
                int xc = x + (radius + 2) * std::cos(angle);
                int ys = x + (radius + 2) * std::sin(angle);
                draw_point(xc, ys);
            }
        }
    } else {
        // Draw an outlined circle using the midpoint circle algorithm
        for (int i = 0; i < 360; i++) {
            float angle = i * std::numbers::pi_v<float> / 180.0f;
            int xc = x + radius * std::cos(angle);
            int ys = y + radius * std::sin(angle);
            draw_point(xc, ys);
        }
    }
}

void IRenderService::draw_convex_polygon_filled(const Vec2 *vertices, int count, const Color &color) {
    set_draw_color(color);

    float minY = vertices[0].y, maxY = vertices[0].y;
    for (int i = 1; i < count; i++) {
        if (vertices[i].y < minY)
            minY = vertices[i].y;
        if (vertices[i].y > maxY)
            maxY = vertices[i].y;
    }

    int yMin = static_cast<int>(minY);
    int yMax = static_cast<int>(maxY);

    for (int y = yMin; y <= yMax; y++) {
        float rowStart = FLT_MAX, rowEnd = -FLT_MAX;
        float yf = static_cast<float>(y);

        for (int i = 0, j = count - 1; i < count; j = i++) {
            float y1 = vertices[j].y, y2 = vertices[i].y;
            if ((y1 <= yf && y2 > yf) || (y2 <= yf && y1 > yf)) {
                float t = (yf - y1) / (y2 - y1);
                float x = vertices[j].x + t * (vertices[i].x - vertices[j].x);
                if (x < rowStart)
                    rowStart = x;
                if (x > rowEnd)
                    rowEnd = x;
            }
        }

        if (rowEnd > rowStart) {
            draw_line(Vec4{rowStart, yf, rowEnd, yf});
        }
    }
}

} // namespace ncore
