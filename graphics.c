#include "graphics.h"
#include <stdlib.h>
#include <math.h>

// Initialize canvas with background character
void clear_canvas(char canvas[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x] = '_';
        }
    }
}

// Plot 8-way symmetric points for Bresenham's circle
static void plot_circle_points(char canvas[HEIGHT][WIDTH], int cx, int cy, int x, int y, char color) {
    int points[8][2] = {
        {cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y},
        {cx + y, cy + x}, {cx - y, cy + x}, {cx + y, cy - x}, {cx - y, cy - x}
    };
    for (int i = 0; i < 8; i++) {
        int px = points[i][0];
        int py = points[i][1];
        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
            canvas[py][px] = color;
        }
    }
}

// Midpoint (Bresenham's) Circle Algorithm
void draw_circle(char canvas[HEIGHT][WIDTH], int cx, int cy, int r, char color) {
    if (r < 0) return;
    if (r == 0) {
        if (cx >= 0 && cx < WIDTH && cy >= 0 && cy < HEIGHT) {
            canvas[cy][cx] = color;
        }
        return;
    }
    
    int x = 0;
    int y = r;
    int d = 1 - r;
    plot_circle_points(canvas, cx, cy, x, y, color);
    
    while (x < y) {
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
        plot_circle_points(canvas, cx, cy, x, y, color);
    }
}

// Bresenham's Line Algorithm (all octants)
void draw_line(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2, char color) {
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT) {
            canvas[y1][x1] = color;
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// Rectangle Drawing (plotting boundaries)
void draw_rectangle(char canvas[HEIGHT][WIDTH], int x, int y, int w, int h, char color) {
    if (w <= 0 || h <= 0) return;
    
    // Draw top and bottom horizontal lines
    for (int i = 0; i < w; i++) {
        int px = x + i;
        if (px >= 0 && px < WIDTH) {
            if (y >= 0 && y < HEIGHT) canvas[y][px] = color;
            if (y + h - 1 >= 0 && y + h - 1 < HEIGHT) canvas[y + h - 1][px] = color;
        }
    }
    
    // Draw left and right vertical lines
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py >= 0 && py < HEIGHT) {
            if (x >= 0 && x < WIDTH) canvas[py][x] = color;
            if (x + w - 1 >= 0 && x + w - 1 < WIDTH) canvas[py][x + w - 1] = color;
        }
    }
}

// Triangle Drawing (three lines)
void draw_triangle(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2, int x3, int y3, char color) {
    draw_line(canvas, x1, y1, x2, y2, color);
    draw_line(canvas, x2, y2, x3, y3, color);
    draw_line(canvas, x3, y3, x1, y1, color);
}

// Regenerate/Redraw the canvas based on vector shapes list
void render_canvas(char canvas[HEIGHT][WIDTH], ShapeObject *objects, int count) {
    clear_canvas(canvas);
    for (int i = 0; i < count; i++) {
        ShapeObject obj = objects[i];
        switch (obj.type) {
            case SHAPE_LINE:
                draw_line(canvas, obj.params.line.x1, obj.params.line.y1, obj.params.line.x2, obj.params.line.y2, '*');
                break;
            case SHAPE_RECTANGLE:
                draw_rectangle(canvas, obj.params.rect.x, obj.params.rect.y, obj.params.rect.w, obj.params.rect.h, '*');
                break;
            case SHAPE_CIRCLE:
                draw_circle(canvas, obj.params.circle.cx, obj.params.circle.cy, obj.params.circle.r, '*');
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(canvas, obj.params.triangle.x1, obj.params.triangle.y1, obj.params.triangle.x2, obj.params.triangle.y2, obj.params.triangle.x3, obj.params.triangle.y3, '*');
                break;
        }
    }
}
