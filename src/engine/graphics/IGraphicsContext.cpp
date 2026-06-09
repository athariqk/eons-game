#include "IGraphicsContext.h"

namespace Aeon {

void IGraphicsContext::DrawCircle(float x, float y, float radius, Color color, bool filled, bool edge) {
    SetDrawColor(color);

    if (filled) {
        // Approximate filled circle using many points
        for (int w = -radius; w <= radius; w++) {
            for (int h = -radius; h <= radius; h++) {
                if (w * w + h * h <= radius * radius) {
                    DrawPoint(x + w, y + h);
                }
            }
        }

        if (edge) {
            SetDrawColor(Color(255, 255, 255, 150));
            for (int i = 0; i < 360; i++) {
                float angle = i * M_PI / 180.0f;
                int xc = x + (radius + 2) * cos(angle);
                int ys = x + (radius + 2) * sin(angle);
                DrawPoint(xc, ys);
            }
        }
    } else {
        // Draw an outlined circle using the midpoint circle algorithm
        for (int i = 0; i < 360; i++) {
            float angle = i * M_PI / 180.0f;
            int xc = x + radius * cos(angle);
            int ys = y + radius * sin(angle);
            DrawPoint(xc, ys);
        }
    }
}

void IGraphicsContext::FillConvexPolygon(const Vector2D *vertices, int count, const Color &color) {
    SetDrawColor(color);

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
            DrawLine(Rect{rowStart, yf, rowEnd, yf});
        }
    }
}

} // namespace Aeon
