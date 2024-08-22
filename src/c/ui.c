#include <ui.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

struct UICallbackTable {
    void (*ui_draw)(UIElement ui_element, int window_w, int window_h);
    void (*ui_mouse_down)(UIElement ui_element, int button, int x, int y);
    void (*ui_mouse_up)(UIElement ui_element, int button, int x, int y);
    void (*ui_mouse_moved)(UIElement ui_element, int x, int y);
};

struct UIElement {
    enum UIType type;
    int min_w, min_h, max_w, max_h;
    double x, y, w, h;
    struct UIElement* parent;
    struct UIElement** children;
    int child_count;
    struct UIStyleSheet style;
    const struct UICallbackTable* callback;
};

static void dimensions(UIElement ui_element, int window_w, int window_h,
                       int* x, int* y, int* w, int* h) {
    *x = ui_element->x * window_w;
    *y = ui_element->y * window_h;
    *w = CLAMP(ui_element->min_w,
               ui_element->max_w,
               ui_element->w * window_w);
    *h = CLAMP(ui_element->min_h,
               ui_element->max_h,
               ui_element->h * window_h);
    if (*w < 0) {
        *x += *w;       
        *w = -*w;
    }
    if (*h < 0) {
        *y += *h;
        *h = -*h;
    }
}

static void basic_draw(UIElement ui_element, int window_w, int window_h) {
    if (ui_element->style.border_strengh > 0) {
        int t = ui_element->style.border_strengh;
        int x, y, w, h;
        dimensions(ui_element, window_w, window_h, &x, &y, &w, &h);
        glColor4ubv(ui_element->style.border_color.rgba);
        glRectd(x,
                y,
                x + w,
                y + t);
        glRectd(x,
                y + h,
                x + w,
                y + h - t);
        glRectd(x,
                y + t,
                x + t,
                y + h - t);
        glRectd(x + w,
                y + t,
                x + w - t,
                y + h - t);
        glColor4ubv(ui_element->style.background_color.rgba);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glRecti(x + t, y + t, x + w - t, y + h - t);
    }
}

const struct UICallbackTable canvas_table = {
    .ui_draw = basic_draw,
    .ui_mouse_down = NULL,
    .ui_mouse_up = NULL,
    .ui_mouse_moved = NULL
};

UIElement ui_canvas() {
    UIElement out = malloc(sizeof(struct UIElement));
    out->type = UI_CANVAS;

    out->parent = NULL;
    out->child_count = 0;
    out->children = NULL;

    out->x = 0;
    out->y = 0;
    out->w = 0;
    out->h = 0;
    out->min_w = 0;
    out->max_w = INT_MAX;   
    out->min_h = 0;
    out->max_h = INT_MAX;

    out->style.background_color = color32(0x20, 0x20, 0x20, 0x80);
    out->style.border_color = color32(0x20, 0x20, 0x20, 0xff);
    out->style.border_strengh = 2;
    out->style.color = color32(0x20, 0x20, 0x20, 0xff);

    out->callback = &canvas_table;

    return out;
}

void ui_draw(UIElement ui_element, int window_w, int window_h) {
    bool scissors = glIsEnabled(GL_SCISSOR_TEST);
    int scissors_box[4];
    if (!scissors)
        glEnable(GL_SCISSOR_TEST);
    else
        glGetIntegerv(GL_SCISSOR_BOX, scissors_box);
    int x, y, w, h;
    dimensions(ui_element, window_w, window_h, &x, &y, &w, &h);
    glScissor(x + MIN(w, 0), y + MIN(h, 0), ABS(w), ABS(h));
    if (ui_element->callback->ui_draw)
        ui_element->callback->ui_draw(ui_element, window_w, window_h);
    if (!scissors) {
        glDisable(GL_SCISSOR_TEST);
        glScissor(scissors_box[0], scissors_box[1], scissors_box[2], scissors_box[3]);
    }
}

void ui_mouse_down(UIElement ui_element, int button, int x, int y) {
    if (x < ui_element->x || x > ui_element->x + ui_element->w ||
        y < ui_element->y || y > ui_element->y + ui_element->h)
        return;
    if (ui_element->callback->ui_mouse_down)
        ui_element->callback->ui_mouse_down(ui_element, button, x, y);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_mouse_down(ui_element->children[i], button, x, y);
}

void ui_mouse_up(UIElement ui_element, int button, int x, int y) {
    if (x < ui_element->x || x > ui_element->x + ui_element->w ||
        y < ui_element->y || y > ui_element->y + ui_element->h)
        return;
    if (ui_element->callback->ui_mouse_up)
        ui_element->callback->ui_mouse_up(ui_element, button, x, y);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_mouse_up(ui_element->children[i], button, x, y);
}

void ui_mouse_moved(UIElement ui_element, int x, int y) {
    if (ui_element->callback->ui_mouse_moved)
        ui_element->callback->ui_mouse_moved(ui_element, x, y);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_mouse_moved(ui_element->children[i], x, y);
}

static int* find_param_i(UIElement ui_element, int param) {
    switch (param) {
    case UI_CHILD_COUNT:
        return &ui_element->child_count;
    case UI_MIN_WIDTH:
        return &ui_element->min_w;
    case UI_MAX_WIDTH:
        return &ui_element->max_w;
    case UI_MIN_HEIGHT:
        return &ui_element->min_h;
    case UI_MAX_HEIGHT:
        return &ui_element->max_h;
    default:
        return NULL;
    }
}

void ui_set_i(UIElement ui_element, int param, int val) {
    int* ptr = find_param_i(ui_element, param);
    if (ptr)
        *ptr = val;
}

int ui_get_i(UIElement ui_element, int param) {
    int* ptr = find_param_i(ui_element, param);
    return ptr ? *ptr : 0;
}

static double* find_param_d(UIElement ui_element, int param) {
    switch (param) {
    case UI_X:
        return &ui_element->x;
    case UI_Y:
        return &ui_element->y;
    case UI_WIDTH:
        return &ui_element->w;
    case UI_HEIGHT:
        return &ui_element->h;
    default:
        return NULL;
    }
}

void ui_set_d(UIElement ui_element, int param, double val) {
    double* ptr = find_param_d(ui_element, param);
    if (ptr)
        *ptr = val;
    else
        printf("[UI][WARNING] trying to set invalid parameter set with type double\n");
}

double ui_get_d(UIElement ui_element, int param) {
    double* ptr = find_param_d(ui_element, param);
    if (ptr)
        return *ptr;
    else
        printf("[UI][WARNING] trying to set invalid parameter set with type double\n");
    return 0;
}

UIStyleSheet ui_access_stylesheet(UIElement ui_element) {
    return &ui_element->style;
}