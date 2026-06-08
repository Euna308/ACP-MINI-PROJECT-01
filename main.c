#include <ncursesw/curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphics.h"

#define MAX_HISTORY 50

typedef struct {
    ShapeObject objects[MAX_OBJECTS];
    int object_count;
    int next_id;
} EditorState;

static EditorState undo_stack[MAX_HISTORY];
static int undo_count = 0;

static EditorState redo_stack[MAX_HISTORY];
static int redo_count = 0;

void save_to_undo(ShapeObject *objects, int object_count, int next_id) {
    if (undo_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            undo_stack[i] = undo_stack[i + 1];
        }
        undo_count = MAX_HISTORY - 1;
    }
    undo_stack[undo_count].object_count = object_count;
    undo_stack[undo_count].next_id = next_id;
    memcpy(undo_stack[undo_count].objects, objects, sizeof(ShapeObject) * object_count);
    undo_count++;
    redo_count = 0;
}

void perform_undo(ShapeObject *objects, int *object_count, int *next_id, char *status_msg) {
    if (undo_count == 0) {
        snprintf(status_msg, 128, "Nothing to undo.");
        return;
    }
    if (redo_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            redo_stack[i] = redo_stack[i + 1];
        }
        redo_count = MAX_HISTORY - 1;
    }
    redo_stack[redo_count].object_count = *object_count;
    redo_stack[redo_count].next_id = *next_id;
    memcpy(redo_stack[redo_count].objects, objects, sizeof(ShapeObject) * (*object_count));
    redo_count++;
    
    undo_count--;
    *object_count = undo_stack[undo_count].object_count;
    *next_id = undo_stack[undo_count].next_id;
    memcpy(objects, undo_stack[undo_count].objects, sizeof(ShapeObject) * (*object_count));
    
    snprintf(status_msg, 128, "Undo successful.");
}

void perform_redo(ShapeObject *objects, int *object_count, int *next_id, char *status_msg) {
    if (redo_count == 0) {
        snprintf(status_msg, 128, "Nothing to redo.");
        return;
    }
    if (undo_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            undo_stack[i] = undo_stack[i + 1];
        }
        undo_count = MAX_HISTORY - 1;
    }
    undo_stack[undo_count].object_count = *object_count;
    undo_stack[undo_count].next_id = *next_id;
    memcpy(undo_stack[undo_count].objects, objects, sizeof(ShapeObject) * (*object_count));
    undo_count++;
    
    redo_count--;
    *object_count = redo_stack[redo_count].object_count;
    *next_id = redo_stack[redo_count].next_id;
    memcpy(objects, redo_stack[redo_count].objects, sizeof(ShapeObject) * (*object_count));
    
    snprintf(status_msg, 128, "Redo successful.");
}

// Redraw header window with title and status flash messages
void draw_header(WINDOW *win, const char *status_msg) {
    wclear(win);
    
    // Bold cyan title
    wattron(win, A_BOLD | COLOR_PAIR(4));
    mvwprintw(win, 0, 1, "2D Vector Graphics Editor (C & ncurses)");
    wattroff(win, A_BOLD | COLOR_PAIR(4));
    
    // Status text in yellow
    if (status_msg && status_msg[0] != '\0') {
        wattron(win, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(win, 0, 48, "Status: %s", status_msg);
        wattroff(win, COLOR_PAIR(5) | A_BOLD);
    }
    wrefresh(win);
}

// Redraw canvas viewer with border and grid coordinates labels
void draw_canvas_win(WINDOW *win, char canvas[HEIGHT][WIDTH]) {
    wclear(win);
    box(win, 0, 0);
    
    // Window header tag
    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 0, 2, " CANVAS VIEW (60x20) ");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);
    
    // Column Index Labels (Tens)
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 1, 5, " ");
    for (int x = 0; x < WIDTH; x++) {
        if (x % 10 == 0) {
            wprintw(win, "%d", x / 10);
        } else {
            wprintw(win, " ");
        }
    }
    
    // Column Index Labels (Units)
    mvwprintw(win, 2, 5, " ");
    for (int x = 0; x < WIDTH; x++) {
        wprintw(win, "%d", x % 10);
    }
    wattroff(win, COLOR_PAIR(2));
    
    // Draw canvas rows with index labels
    for (int y = 0; y < HEIGHT; y++) {
        wattron(win, COLOR_PAIR(2));
        mvwprintw(win, y + 3, 1, "%2d │", y);
        wattroff(win, COLOR_PAIR(2));
        
        for (int x = 0; x < WIDTH; x++) {
            char c = canvas[y][x];
            if (c == '*') {
                wattron(win, COLOR_PAIR(1) | A_BOLD);
                mvwaddch(win, y + 3, x + 5, '*');
                wattroff(win, COLOR_PAIR(1) | A_BOLD);
            } else {
                wattron(win, COLOR_PAIR(3));
                mvwaddch(win, y + 3, x + 5, '_');
                wattroff(win, COLOR_PAIR(3));
            }
        }
        
        wattron(win, COLOR_PAIR(2));
        mvwaddch(win, y + 3, WIDTH + 5, '|');
        wattroff(win, COLOR_PAIR(2));
    }
    wrefresh(win);
}

// Redraw sidebar window showing list of active vector shapes
void draw_sidebar_win(WINDOW *win, ShapeObject *objects, int count) {
    wclear(win);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 0, 2, " ACTIVE VECTOR SHAPES ");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);
    
    if (count == 0) {
        wattron(win, COLOR_PAIR(5));
        mvwprintw(win, 2, 2, "No shapes added yet.");
        wattroff(win, COLOR_PAIR(5));
    } else {
        wattron(win, A_UNDERLINE | COLOR_PAIR(2));
        mvwprintw(win, 1, 2, "ID  Shape     Parameters");
        wattroff(win, A_UNDERLINE | COLOR_PAIR(2));
        
        // Dynamic scrolling helper (displays up to 20 shapes)
        int max_rows = 20;
        int start_idx = 0;
        if (count > max_rows) {
            start_idx = count - max_rows;
        }
        
        for (int i = start_idx; i < count; i++) {
            ShapeObject obj = objects[i];
            int print_y = 2 + (i - start_idx);
            
            wattron(win, A_BOLD | COLOR_PAIR(1));
            mvwprintw(win, print_y, 2, "%2d", obj.id);
            wattroff(win, A_BOLD | COLOR_PAIR(1));
            
            switch (obj.type) {
                case SHAPE_LINE:
                    mvwprintw(win, print_y, 6, "Line  (%d,%d)->(%d,%d)", 
                              obj.params.line.x1, obj.params.line.y1, 
                              obj.params.line.x2, obj.params.line.y2);
                    break;
                case SHAPE_RECTANGLE:
                    mvwprintw(win, print_y, 6, "Rect  (%d,%d) %dx%d", 
                              obj.params.rect.x, obj.params.rect.y, 
                              obj.params.rect.w, obj.params.rect.h);
                    break;
                case SHAPE_CIRCLE:
                    mvwprintw(win, print_y, 6, "Circ  (%d,%d) r=%d", 
                              obj.params.circle.cx, obj.params.circle.cy, 
                              obj.params.circle.r);
                    break;
                case SHAPE_TRIANGLE:
                    mvwprintw(win, print_y, 6, "Tri   (%d,%d),(%d,%d)...", 
                              obj.params.triangle.x1, obj.params.triangle.y1, 
                              obj.params.triangle.x2, obj.params.triangle.y2);
                    break;
            }
        }
    }
    wrefresh(win);
}

// Redraw control options bar at bottom
void draw_control_win(WINDOW *win) {
    wclear(win);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 0, 2, " OPTIONS MENU ");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);
    
    mvwprintw(win, 1, 2, "1. Add Shape    2. Delete Shape    3. Modify Shape    4. Clear Canvas    5. Undo    6. Redo    7. Exit");
    
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 3, 2, "Prompt: ");
    wattroff(win, COLOR_PAIR(2));
    wrefresh(win);
}

// Safe, length-limited input parsing wrapper for ncurses text line
int curses_read_int(WINDOW *win, const char *prompt, int min_val, int max_val, int *cancelled) {
    char buf[128] = "";
    *cancelled = 0;
    
    while (1) {
        // Clear prompt area inside border
        wmove(win, 3, 2);
        wclrtoeol(win);
        wprintw(win, "│ ");
        
        // Re-align right box border if cleared
        mvwaddch(win, 3, 109, '|');
        
        // Display prompt
        wmove(win, 3, 2);
        wattron(win, COLOR_PAIR(2) | A_BOLD);
        wprintw(win, "%s", prompt);
        wattroff(win, COLOR_PAIR(2) | A_BOLD);
        wrefresh(win);
        
        // Open cursor input
        echo();
        curs_set(1);
        wgetnstr(win, buf, sizeof(buf) - 1);
        noecho();
        curs_set(0);
        
        // Empty submission cancels current action
        if (strlen(buf) == 0) {
            *cancelled = 1;
            return 0;
        }
        
        // Validate parse
        char *endptr;
        long parsed = strtol(buf, &endptr, 10);
        if (endptr == buf || *endptr != '\0') {
            wmove(win, 4, 2);
            wclrtoeol(win);
            wattron(win, COLOR_PAIR(5) | A_BOLD);
            wprintw(win, "Error: Invalid number format. Press any key to retry...");
            wattroff(win, COLOR_PAIR(5) | A_BOLD);
            mvwaddch(win, 4, 109, '|');
            wrefresh(win);
            wgetch(win);
            wmove(win, 4, 2);
            wclrtoeol(win);
            mvwaddch(win, 4, 109, '|');
            continue;
        }
        if (parsed < min_val || parsed > max_val) {
            wmove(win, 4, 2);
            wclrtoeol(win);
            wattron(win, COLOR_PAIR(5) | A_BOLD);
            wprintw(win, "Error: Value out of range (%d to %d). Press any key...", min_val, max_val);
            wattroff(win, COLOR_PAIR(5) | A_BOLD);
            mvwaddch(win, 4, 109, '|');
            wrefresh(win);
            wgetch(win);
            wmove(win, 4, 2);
            wclrtoeol(win);
            mvwaddch(win, 4, 109, '|');
            continue;
        }
        return (int)parsed;
    }
}

int main() {
    // Start curses mode
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Register color palettes
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  // * (green draw)
        init_pair(2, COLOR_WHITE, COLOR_BLACK);  // Grid borders
        init_pair(3, COLOR_BLUE, COLOR_BLACK);   // _ (blue background grid)
        init_pair(4, COLOR_CYAN, COLOR_BLACK);   // Headers
        init_pair(5, COLOR_YELLOW, COLOR_BLACK); // Error warnings
    }
    
    // Verify window bounds
    int min_cols = 110;
    int min_rows = 31;
    if (COLS < min_cols || LINES < min_rows) {
        endwin();
        fprintf(stderr, "Error: Terminal window is too small.\n");
        fprintf(stderr, "Minimum size required: %d columns x %d rows.\n", min_cols, min_rows);
        fprintf(stderr, "Current size: %d columns x %d rows.\n", COLS, LINES);
        fprintf(stderr, "Please resize your terminal window and run again.\n");
        return 1;
    }
    
    // Initialize panels layout
    WINDOW *header_win = newwin(1, 110, 0, 0);
    WINDOW *canvas_win = newwin(24, 67, 1, 0);
    WINDOW *sidebar_win = newwin(24, 43, 1, 67);
    WINDOW *control_win = newwin(6, 110, 25, 0);
    
    char canvas[HEIGHT][WIDTH];
    ShapeObject objects[MAX_OBJECTS];
    int object_count = 0;
    int next_id = 1;
    
    char status_msg[128] = "Welcome! Canvas initialized empty.";
    int cancelled = 0;

    while (1) {
        render_canvas(canvas, objects, object_count);
        
        draw_header(header_win, status_msg);
        draw_canvas_win(canvas_win, canvas);
        draw_sidebar_win(sidebar_win, objects, object_count);
        draw_control_win(control_win);
        
        int choice = curses_read_int(control_win, "Enter Option (1-7, empty cancels): ", 1, 7, &cancelled);
        if (cancelled) {
            snprintf(status_msg, sizeof(status_msg), "Action cancelled.");
            continue;
        }
        
        if (choice == 7) {
            break; // Exit
        }
        
        switch (choice) {
            case 1: { // Add Shape
                wmove(control_win, 1, 2);
                wclrtoeol(control_win);
                wprintw(control_win, "Add: 1. Line   2. Rectangle   3. Circle   4. Triangle   5. Cancel");
                mvwaddch(control_win, 1, 109, '|');
                wrefresh(control_win);
                
                int shape_choice = curses_read_int(control_win, "Select Shape (1-5): ", 1, 5, &cancelled);
                if (cancelled || shape_choice == 5) {
                    snprintf(status_msg, sizeof(status_msg), "Addition cancelled.");
                    break;
                }
                
                if (object_count >= MAX_OBJECTS) {
                    snprintf(status_msg, sizeof(status_msg), "Error: Shape limit reached (%d)!", MAX_OBJECTS);
                    break;
                }
                
                ShapeObject new_obj;
                new_obj.id = next_id;
                int ok = 1;
                
                if (shape_choice == 1) {
                    new_obj.type = SHAPE_LINE;
                    new_obj.params.line.x1 = curses_read_int(control_win, "Start X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    if (ok) {
                        new_obj.params.line.y1 = curses_read_int(control_win, "Start Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.line.x2 = curses_read_int(control_win, "End X (0-59): ", 0, WIDTH - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.line.y2 = curses_read_int(control_win, "End Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) snprintf(status_msg, sizeof(status_msg), "Line ID %d added.", new_obj.id);
                } else if (shape_choice == 2) {
                    new_obj.type = SHAPE_RECTANGLE;
                    new_obj.params.rect.x = curses_read_int(control_win, "Top-Left X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    if (ok) {
                        new_obj.params.rect.y = curses_read_int(control_win, "Top-Left Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.rect.w = curses_read_int(control_win, "Width (1-60): ", 1, WIDTH, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.rect.h = curses_read_int(control_win, "Height (1-20): ", 1, HEIGHT, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) snprintf(status_msg, sizeof(status_msg), "Rectangle ID %d added.", new_obj.id);
                } else if (shape_choice == 3) {
                    new_obj.type = SHAPE_CIRCLE;
                    new_obj.params.circle.cx = curses_read_int(control_win, "Center X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    if (ok) {
                        new_obj.params.circle.cy = curses_read_int(control_win, "Center Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.circle.r = curses_read_int(control_win, "Radius (0-30): ", 0, 30, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) snprintf(status_msg, sizeof(status_msg), "Circle ID %d added.", new_obj.id);
                } else if (shape_choice == 4) {
                    new_obj.type = SHAPE_TRIANGLE;
                    new_obj.params.triangle.x1 = curses_read_int(control_win, "Vertex 1 X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    if (ok) {
                        new_obj.params.triangle.y1 = curses_read_int(control_win, "Vertex 1 Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.triangle.x2 = curses_read_int(control_win, "Vertex 2 X (0-59): ", 0, WIDTH - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.triangle.y2 = curses_read_int(control_win, "Vertex 2 Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.triangle.x3 = curses_read_int(control_win, "Vertex 3 X (0-59): ", 0, WIDTH - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        new_obj.params.triangle.y3 = curses_read_int(control_win, "Vertex 3 Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) snprintf(status_msg, sizeof(status_msg), "Triangle ID %d added.", new_obj.id);
                }
                
                if (ok) {
                    save_to_undo(objects, object_count, next_id);
                    objects[object_count++] = new_obj;
                    next_id++;
                } else {
                    snprintf(status_msg, sizeof(status_msg), "Shape creation aborted.");
                }
                break;
            }
            case 2: { // Delete Shape
                if (object_count == 0) {
                    snprintf(status_msg, sizeof(status_msg), "No shapes to delete.");
                    break;
                }
                
                int del_id = curses_read_int(control_win, "Enter Shape ID to delete: ", 1, 9999, &cancelled);
                if (cancelled) {
                    snprintf(status_msg, sizeof(status_msg), "Delete cancelled.");
                    break;
                }
                
                int found_idx = -1;
                for (int i = 0; i < object_count; i++) {
                    if (objects[i].id == del_id) {
                        found_idx = i;
                        break;
                    }
                }
                
                if (found_idx != -1) {
                    save_to_undo(objects, object_count, next_id);
                    for (int j = found_idx; j < object_count - 1; j++) {
                        objects[j] = objects[j+1];
                    }
                    object_count--;
                    snprintf(status_msg, sizeof(status_msg), "Shape ID %d deleted.", del_id);
                } else {
                    snprintf(status_msg, sizeof(status_msg), "Error: Shape ID %d not found.", del_id);
                }
                break;
            }
            case 3: { // Modify Shape
                if (object_count == 0) {
                    snprintf(status_msg, sizeof(status_msg), "No shapes to modify.");
                    break;
                }
                
                int mod_id = curses_read_int(control_win, "Enter Shape ID to modify: ", 1, 9999, &cancelled);
                if (cancelled) {
                    snprintf(status_msg, sizeof(status_msg), "Modify cancelled.");
                    break;
                }
                
                int idx = -1;
                for (int i = 0; i < object_count; i++) {
                    if (objects[i].id == mod_id) {
                        idx = i;
                        break;
                    }
                }
                
                if (idx == -1) {
                    snprintf(status_msg, sizeof(status_msg), "Error: Shape ID %d not found.", mod_id);
                    break;
                }
                
                ShapeObject *obj = &objects[idx];
                int ok = 1;
                
                if (obj->type == SHAPE_LINE) {
                    int x1 = curses_read_int(control_win, "New Start X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    int y1 = 0, x2 = 0, y2 = 0;
                    if (ok) {
                        y1 = curses_read_int(control_win, "New Start Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        x2 = curses_read_int(control_win, "New End X (0-59): ", 0, WIDTH - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        y2 = curses_read_int(control_win, "New End Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        save_to_undo(objects, object_count, next_id);
                        obj->params.line.x1 = x1;
                        obj->params.line.y1 = y1;
                        obj->params.line.x2 = x2;
                        obj->params.line.y2 = y2;
                        snprintf(status_msg, sizeof(status_msg), "Line ID %d updated.", mod_id);
                    }
                } else if (obj->type == SHAPE_RECTANGLE) {
                    int x = curses_read_int(control_win, "New Top-Left X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    int y = 0, w = 0, h = 0;
                    if (ok) {
                        y = curses_read_int(control_win, "New Top-Left Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        w = curses_read_int(control_win, "New Width (1-60): ", 1, WIDTH, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        h = curses_read_int(control_win, "New Height (1-20): ", 1, HEIGHT, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        save_to_undo(objects, object_count, next_id);
                        obj->params.rect.x = x;
                        obj->params.rect.y = y;
                        obj->params.rect.w = w;
                        obj->params.rect.h = h;
                        snprintf(status_msg, sizeof(status_msg), "Rectangle ID %d updated.", mod_id);
                    }
                } else if (obj->type == SHAPE_CIRCLE) {
                    int cx = curses_read_int(control_win, "New Center X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    int cy = 0, r = 0;
                    if (ok) {
                        cy = curses_read_int(control_win, "New Center Y (0-19): ", 0, HEIGHT - 1, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        r = curses_read_int(control_win, "New Radius (0-30): ", 0, 30, &cancelled);
                        if (cancelled) ok = 0;
                    }
                    if (ok) {
                        save_to_undo(objects, object_count, next_id);
                        obj->params.circle.cx = cx;
                        obj->params.circle.cy = cy;
                        obj->params.circle.r = r;
                        snprintf(status_msg, sizeof(status_msg), "Circle ID %d updated.", mod_id);
                    }
                } else if (obj->type == SHAPE_TRIANGLE) {
                    int x1 = curses_read_int(control_win, "New Vertex 1 X (0-59): ", 0, WIDTH - 1, &cancelled);
                    if (cancelled) ok = 0;
                    int y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
                    if (ok) { y1 = curses_read_int(control_win, "New Vertex 1 Y (0-19): ", 0, HEIGHT - 1, &cancelled); if (cancelled) ok = 0; }
                    if (ok) { x2 = curses_read_int(control_win, "New Vertex 2 X (0-59): ", 0, WIDTH - 1, &cancelled); if (cancelled) ok = 0; }
                    if (ok) { y2 = curses_read_int(control_win, "New Vertex 2 Y (0-19): ", 0, HEIGHT - 1, &cancelled); if (cancelled) ok = 0; }
                    if (ok) { x3 = curses_read_int(control_win, "New Vertex 3 X (0-59): ", 0, WIDTH - 1, &cancelled); if (cancelled) ok = 0; }
                    if (ok) { y3 = curses_read_int(control_win, "New Vertex 3 Y (0-19): ", 0, HEIGHT - 1, &cancelled); if (cancelled) ok = 0; }
                    if (ok) {
                        save_to_undo(objects, object_count, next_id);
                        obj->params.triangle.x1 = x1; obj->params.triangle.y1 = y1;
                        obj->params.triangle.x2 = x2; obj->params.triangle.y2 = y2;
                        obj->params.triangle.x3 = x3; obj->params.triangle.y3 = y3;
                        snprintf(status_msg, sizeof(status_msg), "Triangle ID %d updated.", mod_id);
                    }
                }
                
                if (!ok) {
                    snprintf(status_msg, sizeof(status_msg), "Modification aborted.");
                }
                break;
            }
            case 4: { // Clear Canvas
                if (object_count == 0) {
                    snprintf(status_msg, sizeof(status_msg), "Canvas already empty.");
                } else {
                    save_to_undo(objects, object_count, next_id);
                    object_count = 0;
                    snprintf(status_msg, sizeof(status_msg), "Canvas cleared.");
                }
                break;
            }
            case 5: { // Undo
                perform_undo(objects, &object_count, &next_id, status_msg);
                break;
            }
            case 6: { // Redo
                perform_redo(objects, &object_count, &next_id, status_msg);
                break;
            }
        }
    }
    
    // Clean up resources and restore terminal
    delwin(header_win);
    delwin(canvas_win);
    delwin(sidebar_win);
    delwin(control_win);
    endwin();
    
    return 0;
}
