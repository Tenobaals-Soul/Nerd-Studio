#include <ui.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#define GET_EXTENTION_DATA(ui_element, t) ({assert(ui_element->type == t); get_extention_data(ui_element);})

struct UICallbackTable {
    void (*ui_draw)(UIElement ui_element, int window_w, int window_h);
    void (*ui_resize)(UIElement ui_element, int window_w, int window_h);
    void (*ui_mouse_down)(UIElement ui_element, int button, int x, int y);
    void (*ui_mouse_up)(UIElement ui_element, int button, int x, int y);
    void (*ui_mouse_moved)(UIElement ui_element, int x, int y);
    void (*ui_free)(UIElement ui_element);
};

struct UIElement {
    enum UIType type;
    const struct UICallbackTable* callback;
    int min_w, min_h, max_w, max_h;
    double x, y, w, h;
    struct UIElement* parent;
    struct UIElement** children;
    int child_count;
    struct UIStyleSheet style;
    int _x, _y, _w, _h;
};

struct UIResizer {
    UIElement connected_item1;
    UIElement connected_item2;
    enum ui_direction direction;
    double side_ration;
    void (*set_cursor)(void* user_data, enum ui_direction);
    void* user_data;
    bool currently_grabbed;
    int window_w;
    int window_h;
};

struct UIButton {
    void (*on_click)(void* user_data);
    void* user_data;
    bool click_started;
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
    ui_element->_x = *x;
    ui_element->_y = *y;
    ui_element->_w = *w;
    ui_element->_h = *h;
}

static bool point_inside(UIElement ui_element, int x, int y) {
    return x == CLAMP(ui_element->_x, ui_element->_x + ui_element->_w, x) &&
           y == CLAMP(ui_element->_y, ui_element->_y + ui_element->_h, y);
}

static void init_ui_element(UIElement init, int window_w, int window_h) {
    init->type = UI_NO_TYPE;

    init->parent = NULL;
    init->child_count = 0;
    init->children = NULL;

    init->x = 0;
    init->y = 0;
    init->w = 0;
    init->h = 0;
    init->min_w = 0;
    init->max_w = INT_MAX;   
    init->min_h = 0;
    init->max_h = INT_MAX;

    init->style.background_color = color32(0x20, 0x20, 0x20, 0x80);
    init->style.border_color = color32(0x20, 0x20, 0x20, 0xff);
    init->style.border_strengh = 2;
    init->style.color = color32(0xff, 0xff, 0xff, 0x80);
    
    init->callback = NULL;

    int x, y, w, h;
    dimensions(init, window_w, window_h, &x, &y, &w, &h);
}

static void* get_extention_data(UIElement ui_element) {
    return ui_element + 1;
}

static void basic_draw(UIElement ui_element, int window_w, int window_h) {
    if (ui_element->style.border_strengh > 0) {
        int t = ui_element->style.border_strengh;
        int x, y, w, h;
        dimensions(ui_element, window_w, window_h, &x, &y, &w, &h);
        glColor4ubv(ui_element->style.border_color.rgba);
        glRecti(x,      y,
                x + w,  y + t);
        glRecti(x,      y + h,
                x + w,  y + h - t);
        glRecti(x,      y + t,
                x + t,  y + h - t);
        glRecti(x + w,  y + t,
                x+w-t,  y + h - t);
        glColor4ubv(ui_element->style.background_color.rgba);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glRecti(x + t, y + t,
                x+w-t, y + h - t);
    }
}

const struct UICallbackTable canvas_table = {
    .ui_draw = basic_draw,
    .ui_resize = NULL,
    .ui_mouse_down = NULL,
    .ui_mouse_up = NULL,
    .ui_mouse_moved = NULL
};

UIElement ui_canvas(int window_w, int window_h) {
    UIElement out = malloc(sizeof(struct UIElement));
    init_ui_element(out, window_w, window_h);
    out->type = UI_CANVAS;
    out->callback = &canvas_table;
    return out;
}

static void resizer_draw(UIElement ui_element, int window_w, int window_h) {
    struct UIResizer* res = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    basic_draw(ui_element, window_w, window_h);
    glColor4ubv(ui_element->style.color.rgba);
    int x, y, w, h, t;
    dimensions(ui_element, window_w, window_h, &x, &y, &w, &h);
    if (res->direction == HORIZONTAL) {
        t = w * res->side_ration;
        for (int i = -2; i <= 2; i+=2)
            glRecti(x + (w - t) / 2, y + (h - t) / 2 + i * t,
                    x + (w + t) / 2, y + (h + t) / 2 + i * t);
    }
    else {
        t = h * res->side_ration;
        for (int i = -2; i <= 2; i+=2)
            glRecti(x + (h - t) / 2 + i * t,  y + (w - t) / 2,
                    x + (h + t) / 2 + i * t,  y + (w + t) / 2);
    }
}

static void position_resizer(UIElement ui_element) {
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    if (resizer->connected_item1 != NULL) {
        int x, y, w, h;
        dimensions(resizer->connected_item1, resizer->window_w, resizer->window_h, &x, &y, &w, &h);
        if (resizer->direction == HORIZONTAL)
            ui_element->x = (x + MIN(w, 0)) / (double) resizer->window_w;
        else
            ui_element->y = (y + MIN(w, 0)) / (double) resizer->window_h;
    }
    else if (resizer->connected_item2 != NULL) {
        int x, y, w, h;
        dimensions(resizer->connected_item2, resizer->window_w, resizer->window_h, &x, &y, &w, &h);
        if (resizer->direction == HORIZONTAL)
            ui_element->x = (x + MAX(w, 0)) / (double) resizer->window_w;
        else
            ui_element->y = (y + MAX(w, 0)) / (double) resizer->window_h;
    }
}

static void resizer_resize(UIElement ui_element, int window_w, int window_h) {
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    resizer->window_w = window_w;
    resizer->window_h = window_h;
    position_resizer(ui_element);
}

static void resizer_mouse_down(UIElement ui_element, int button, int x, int y) {
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    if (button != 1)
        return;
    if (point_inside(ui_element, x, y))
        resizer->currently_grabbed = true;
}

static void resizer_mouse_up(UIElement ui_element, int button, int x, int y) {
    (void) x; (void) y;
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    if (button != 1)
        return;
    resizer->currently_grabbed = false;
}

static void resizer_mouse_moved(UIElement ui_element, int x, int y) {
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    if (point_inside(ui_element, x, y) || resizer->currently_grabbed) {
        if (resizer->set_cursor)
            resizer->set_cursor(resizer->user_data, resizer->direction);
    }
    if (resizer->currently_grabbed) {
        if (resizer->direction == HORIZONTAL) {
            double nx = x / (double) resizer->window_w;
            if (resizer->connected_item1) {
                if (resizer->connected_item1->w >= 0) {
                    resizer->connected_item1->w = nx - resizer->connected_item1->x;
                    resizer->connected_item1->x = nx;
                }
                else {
                    resizer->connected_item1->w = nx - resizer->connected_item1->x;
                }
            }
            if (resizer->connected_item2) {
                if (resizer->connected_item2->w >= 0) {
                    resizer->connected_item2->w = nx - resizer->connected_item2->x;
                }
                else {
                    resizer->connected_item2->w = nx - resizer->connected_item2->x;
                    resizer->connected_item2->x = nx;
                }
            }
        }
        else {
            double ny = y / (double) resizer->window_h;
            if (resizer->connected_item1) {
                if (resizer->connected_item1->h >= 0) {
                    resizer->connected_item1->h = ny - resizer->connected_item1->y;
                    resizer->connected_item1->y = ny;
                }
                else {
                    resizer->connected_item1->h = ny - resizer->connected_item1->y;
                }
            }
            if (resizer->connected_item2) {
                if (resizer->connected_item2->h >= 0) {
                    resizer->connected_item2->h = ny - resizer->connected_item2->y;
                }
                else {
                    resizer->connected_item2->h = ny - resizer->connected_item2->y;
                    resizer->connected_item2->y = ny;
                }
            }
        }
    }
    position_resizer(ui_element);
}

const struct UICallbackTable resizer_table = {
    .ui_draw = resizer_draw,
    .ui_resize = resizer_resize,
    .ui_mouse_down = resizer_mouse_down,
    .ui_mouse_up = resizer_mouse_up,
    .ui_mouse_moved = resizer_mouse_moved
};

UIElement ui_resizer(int window_w, int window_h, enum ui_direction direction,
                     UIElement item1, UIElement item2, double side) {
    UIElement out = malloc(sizeof(struct UIElement) + sizeof(struct UIResizer));
    init_ui_element(out, window_w, window_h);
    out->type = UI_RESIZER;
    out->callback = &resizer_table;
    struct UIResizer* resizer = get_extention_data(out);
    resizer->connected_item1 = item1;
    resizer->connected_item2 = item2;
    resizer->direction = direction;
    resizer->side_ration = side;
    resizer->window_w = window_w;
    resizer->window_h = window_h;
    resizer->currently_grabbed = false;

    if (item1 != NULL) {
        int x, y, w, h;
        dimensions(item1, window_w, window_h, &x, &y, &w, &h);
        if (resizer->direction == HORIZONTAL)
            out->x = (x + MIN(w, 0)) / (double) window_w;
        else
            out->y = (y + MIN(w, 0)) / (double) window_h;
    }
    else if (item2 != NULL) {
        int x, y, w, h;
        dimensions(item2, window_w, window_h, &x, &y, &w, &h);
        if (resizer->direction == HORIZONTAL)
            out->x = (x + MAX(w, 0)) / (double) window_w;
        else
            out->y = (y + MAX(w, 0)) / (double) window_h;
    }
    return out;
}

static void button_mouse_down(UIElement ui_element, int mbutton, int x, int y) {
    if (mbutton != 1)
        return;
    if (point_inside(ui_element, x, y)) {
        struct UIButton* button = get_extention_data(ui_element);
        button->click_started = true;
    }
}

static void button_mouse_up(UIElement ui_element, int mbutton, int x, int y) {
    if (mbutton != 1)
        return;
    struct UIButton* button = get_extention_data(ui_element);
    if (button->click_started && point_inside(ui_element, x, y)) {
        button->on_click(button->user_data);
    }
    button->click_started = false;
}

const struct UICallbackTable button_table = {
    .ui_draw = basic_draw,
    .ui_resize = NULL,
    .ui_mouse_down = button_mouse_down,
    .ui_mouse_up = button_mouse_up,
    .ui_mouse_moved = NULL
};

UIElement ui_button(int window_w, int window_h, void (*on_click)(void*), void* user_data) {
    UIElement out = malloc(sizeof(struct UIElement) + sizeof(struct UIButton));
    init_ui_element(out, window_w, window_h);
    out->type = UI_BUTTON;
    out->callback = &button_table;
    struct UIButton* button = get_extention_data(out);
    button->on_click = on_click;
    button->user_data = user_data;
    button->click_started = false;
    return out;
}

void ui_resizer_set_curser_func(UIElement ui_element, void (*curser_func)
                                (void* user_data, enum ui_direction),
                                void* user_data) {
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    resizer->set_cursor = curser_func;
    resizer->user_data = user_data;
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
    glScissor(x, y, w, h);
    if (ui_element->callback->ui_draw)
        ui_element->callback->ui_draw(ui_element, window_w, window_h);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_draw(ui_element->children[i], window_w, window_h);
    if (!scissors) {
        glDisable(GL_SCISSOR_TEST);
        glScissor(scissors_box[0], scissors_box[1], scissors_box[2], scissors_box[3]);
    }
}

void ui_resize(UIElement ui_element, int window_w, int window_h) {
    if (ui_element->callback->ui_resize)
        ui_element->callback->ui_resize(ui_element, window_w, window_h);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_resize(ui_element->children[i], window_w, window_h);
}

void ui_mouse_down(UIElement ui_element, int button, int x, int y) {
    if (ui_element->callback->ui_mouse_down)
        ui_element->callback->ui_mouse_down(ui_element, button, x, y);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_mouse_down(ui_element->children[i], button, x, y);
}

void ui_mouse_up(UIElement ui_element, int button, int x, int y) {
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

void ui_set_parent(UIElement ui_element, UIElement parent) {
    if (ui_element->parent) {
        UIElement old_parent = ui_element->parent;
        bool move = false;
        for (int i = 0; i < ui_element->parent->child_count + 1; i++) {
            if (move)
                old_parent->children[i-1] = old_parent->children[i];
            else
                move = ui_element->parent->children[i] == ui_element;
        }
        old_parent->child_count--;
    }
    if (parent) {
        parent->children = realloc(parent->children, sizeof(UIElement) * (parent->child_count + 1));
        parent->children[parent->child_count] = ui_element;
        parent->child_count++;
    }
    ui_element->parent = parent;
}

void ui_free(UIElement ui_element) {
    if (ui_element->callback->ui_free)
        ui_element->callback->ui_free(ui_element);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_free(ui_element->children[i]);
    free(ui_element);
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