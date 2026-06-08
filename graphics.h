#ifndef GRAPHICS_H
#define GRAPHICS_H

#define WIDTH 60
#define HEIGHT 20
#define MAX_OBJECTS 100

typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

typedef struct {
    int x1, y1;
    int x2, y2;
} LineParams;

typedef struct {
    int x, y;
    int w, h;
} RectParams;

typedef struct {
    int cx, cy;
    int r;
} CircleParams;

typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriangleParams;

typedef struct {
    int id;
    ShapeType type;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriangleParams triangle;
    } params;
} ShapeObject;

// Canvas operations
void clear_canvas(char canvas[HEIGHT][WIDTH]);
void render_canvas(char canvas[HEIGHT][WIDTH], ShapeObject *objects, int count);

// Low-level drawing algorithms
void draw_line(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2, char color);
void draw_rectangle(char canvas[HEIGHT][WIDTH], int x, int y, int w, int h, char color);
void draw_circle(char canvas[HEIGHT][WIDTH], int cx, int cy, int r, char color);
void draw_triangle(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2, int x3, int y3, char color);

#endif // GRAPHICS_H
