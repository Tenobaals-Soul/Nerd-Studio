#include <ui.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#if defined(_MSC_VER)
#define strtok_r strtok_s
#endif


#define GET_EXTENTION_DATA(ui_element, t) ({assert(ui_element->type == t); get_extention_data(ui_element);})

int window_width;
int window_height;

struct UICallbackTable {
    void (*ui_draw)(UIElement ui_element);
    void (*ui_resize)(UIElement ui_element, int window_w, int window_h);
    void (*ui_mouse_down)(UIElement ui_element, int button, int x, int y);
    void (*ui_mouse_up)(UIElement ui_element, int button, int x, int y);
    void (*ui_mouse_moved)(UIElement ui_element, int x, int y);
    void (*ui_free)(UIElement ui_element);
};

struct UIElement {
    enum UIType type;
    const struct UICallbackTable* callback;
    struct UITransform transform;
    struct UIElement* parent;
    struct UIElement** children;
    int child_count;
    struct UIStyleSheet style;
    int _x, _y, _w, _h;
};

struct UIResizer {
    UIElement connected_item1;
    UIElement connected_item2;
    enum UIDirection direction;
    double side_ration;
    void (*set_cursor)(void* user_data, enum UIDirection);
    void* user_data;
    bool currently_grabbed;
};

struct UIButton {
    void (*on_click)(void* user_data);
    void* user_data;
    bool click_started;
};

static void dimensions(UITransform transform, int window_w, int window_h,
                                   int* x, int* y, int* w, int* h) {
    *x = transform->x * window_w + transform->off_x;
    *y = transform->y * window_h + transform->off_y;
    *w = CLAMP(transform->min_w,
               transform->max_w,
               transform->w * window_w);
    *h = CLAMP(transform->min_h,
               transform->max_h,
               transform->h * window_h);
    if (*w < 0) {
        *x += *w;       
        *w = -*w;
    }
    if (*h < 0) {
        *y += *h;
        *h = -*h;
    }
}

static void recalculate_dimensions(UIElement ui_element, int window_w, int window_h) {
    int x, y, w, h;
    dimensions(&ui_element->transform, window_w, window_h, &x, &y, &w, &h);
    ui_element->_x = x;
    ui_element->_y = y;
    ui_element->_w = w;
    ui_element->_h = h;
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

    init->transform.x = 0;
    init->transform.y = 0;
    init->transform.w = 0;
    init->transform.h = 0;
    init->transform.min_w = 0;
    init->transform.max_w = INT_MAX;   
    init->transform.min_h = 0;
    init->transform.max_h = INT_MAX;
    init->transform.off_x = 0;
    init->transform.off_y = 0;

    init->style.background_color = color32(0x20, 0x20, 0x20, 0x80);
    init->style.border_color = color32(0x20, 0x20, 0x20, 0xff);
    init->style.border_strengh = 2;
    init->style.color = color32(0xff, 0xff, 0xff, 0x80);

    window_width = window_w;
    window_height = window_h;
    
    init->callback = NULL;

    recalculate_dimensions(init, window_w, window_h);
}

static void* get_extention_data(UIElement ui_element) {
    return ui_element + 1;
}

static void basic_draw(UIElement ui_element) {
    if (ui_element->style.border_strengh > 0) {
        int t = ui_element->style.border_strengh;
        int x = ui_element->_x;
        int y = ui_element->_y;
        int w = ui_element->_w;
        int h = ui_element->_h;
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

static void resizer_draw(UIElement ui_element) {
    struct UIResizer* res = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    basic_draw(ui_element);
    glColor4ubv(ui_element->style.color.rgba);
    int x = ui_element->_x;
    int y = ui_element->_y;
    int w = ui_element->_w;
    int h = ui_element->_h;
    int t;
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
        if (resizer->direction == HORIZONTAL)
            ui_element->transform.x = (resizer->connected_item1->_x + MAX(resizer->connected_item1->_w, 0)) / (double) window_width;
        else
            ui_element->transform.y = (resizer->connected_item1->_y + MAX(resizer->connected_item1->_h, 0)) / (double) window_height;
    }
    else if (resizer->connected_item2 != NULL) {
        if (resizer->direction == HORIZONTAL)
            ui_element->transform.x = (resizer->connected_item2->_x + MIN(resizer->connected_item2->_w, 0)) / (double) window_width;
        else
            ui_element->transform.y = (resizer->connected_item2->_y + MIN(resizer->connected_item2->_h, 0)) / (double) window_height;
    }
}

static void resizer_resize(UIElement ui_element, int window_w, int window_h) {
    (void) window_w; (void) window_h;
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
            double nx = x / (double) window_width;
            if (resizer->connected_item1) {
                resizer->connected_item1->transform.w = nx - resizer->connected_item1->transform.x;
                if (resizer->connected_item1->transform.w < 0)
                    resizer->connected_item1->transform.x = nx;
            }
            if (resizer->connected_item2) {
                resizer->connected_item2->transform.w = nx - resizer->connected_item2->transform.x;
                if (resizer->connected_item2->transform.w >= 0)
                    resizer->connected_item2->transform.x = nx;
            }
        }
        else {
            double ny = y / (double) window_height;
            if (resizer->connected_item1) {
                resizer->connected_item1->transform.h = ny - resizer->connected_item1->transform.x;
                if (resizer->connected_item1->transform.h < 0)
                    resizer->connected_item1->transform.y = ny;
            }
            if (resizer->connected_item2) {
                resizer->connected_item2->transform.h = ny - resizer->connected_item2->transform.x;
                if (resizer->connected_item2->transform.h >= 0)
                    resizer->connected_item2->transform.y = ny;
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

UIElement ui_resizer(int window_w, int window_h, enum UIDirection direction,
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
    resizer->currently_grabbed = false;

    if (item1 != NULL) {
        int x = item1->_x;
        int y = item1->_y;
        int w = item1->_w;
        int h = item1->_h;
        if (resizer->direction == HORIZONTAL)
            out->transform.x = (x + MIN(w, 0)) / (double) window_w;
        else
            out->transform.y = (y + MIN(h, 0)) / (double) window_h;
    }
    else if (item2 != NULL) {
        int x = item2->_x;
        int y = item2->_y;
        int w = item2->_w;
        int h = item2->_h;
        if (resizer->direction == HORIZONTAL)
            out->transform.x = (x + MAX(w, 0)) / (double) window_w;
        else
            out->transform.y = (y + MAX(h, 0)) / (double) window_h;
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
                                (void* user_data, enum UIDirection),
                                void* user_data) {
    struct UIResizer* resizer = GET_EXTENTION_DATA(ui_element, UI_RESIZER);
    resizer->set_cursor = curser_func;
    resizer->user_data = user_data;
}

void ui_draw(UIElement ui_element) {
    recalculate_dimensions(ui_element,window_width, window_height); // TODO: find a clean solution for this
    bool scissors = glIsEnabled(GL_SCISSOR_TEST);
    int scissors_box[4];
    if (scissors)
        glGetIntegerv(GL_SCISSOR_BOX, scissors_box);
    else
        glEnable(GL_SCISSOR_TEST);
    glScissor(ui_element->_x, ui_element->_y, ui_element->_w, ui_element->_h);
    if (ui_element->callback->ui_draw)
        ui_element->callback->ui_draw(ui_element);
    for (int i = 0; i < ui_element->child_count; i++)
        ui_draw(ui_element->children[i]);
    if (scissors)
        glScissor(scissors_box[0], scissors_box[1], scissors_box[2], scissors_box[3]);
    else
        glDisable(GL_SCISSOR_TEST);
}

void ui_resize(UIElement ui_element, int window_w, int window_h) {
    window_width = window_w;
    window_height = window_h;
    recalculate_dimensions(ui_element, window_w, window_h);
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
        return &ui_element->transform.min_w;
    case UI_MAX_WIDTH:
        return &ui_element->transform.max_w;
    case UI_MIN_HEIGHT:
        return &ui_element->transform.min_h;
    case UI_MAX_HEIGHT:
        return &ui_element->transform.max_h;
    case UI_OFFSET_X:
        return &ui_element->transform.off_x;
    case UI_OFFSET_Y:
        return &ui_element->transform.off_y;
    default:
        return NULL;
    }
}

void ui_set_i(UIElement ui_element, int param, int val) {
    int* ptr = find_param_i(ui_element, param);
    if (ptr)
        *ptr = val;
    else
        printf("[UI][WARNING] trying to set invalid parameter set with type int\n");
}

int ui_get_i(UIElement ui_element, int param) {
    int* ptr = find_param_i(ui_element, param);
    if (ptr)
        return *ptr;
    else
        printf("[UI][WARNING] trying to get invalid parameter set with type int\n");
    return 0;
}

static double* find_param_d(UIElement ui_element, int param) {
    switch (param) {
    case UI_X:
        return &ui_element->transform.x;
    case UI_Y:
        return &ui_element->transform.y;
    case UI_WIDTH:
        return &ui_element->transform.w;
    case UI_HEIGHT:
        return &ui_element->transform.h;
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
        printf("[UI][WARNING] trying to get invalid parameter set with type double\n");
    return 0;
}

UIStyleSheet ui_access_stylesheet(UIElement ui_element) {
    return &ui_element->style;
}

static void parse_param_as_int(int* out, const char* valstr) {
    int val;
    if (sscanf(valstr, "%d", &val) != 1) {
        printf("[UI][WARNING] invalid value \"%s\"\n", valstr);
        return;
    }
    *out = val;
}

static void parse_param_as_double(double* out, const char* valstr) {
    double val;
    if (sscanf(valstr, "%lf", &val) != 1) {
        printf("[UI][WARNING] invalid value \"%s\"\n", valstr);
        return;
    }
    *out = val;
}

static void parse_param_as_color(color32* out, const char* valstr) {
    char val[8] = "000000FF";
    if (sscanf(valstr, "%8c", val) != 1 || sscanf(valstr, "%6c", val) != 1) {
        printf("[UI][WARNING] invalid value \"%s\"\n", valstr);
        return;
    }
    for (int i = 0; i < 4; i++) {
        int p = i << 1;
        out->rgba[i] = (val[p] << 4) + val[p+1];
    }
}

static void parse_single_style(UIElement ui_element, const char* style) {
    char key[256], val[256];
    if (sscanf(style, " %255[a-zA-Z0-9_] = %255s ", key, val) != 2) {
        printf("[UI][WARNING] invalid style format \"%s\"\n", style);
        return;
    }
    if (strcmp(key, "x") == 0)
        parse_param_as_double(&ui_element->transform.x, val);
    else if (strcmp(key, "y") == 0)
        parse_param_as_double(&ui_element->transform.y, val);
    else if (strcmp(key, "w") == 0)
        parse_param_as_double(&ui_element->transform.w, val);
    else if (strcmp(key, "h") == 0)
        parse_param_as_double(&ui_element->transform.h, val);
    else if (strcmp(key, "min_w") == 0)
        parse_param_as_int(&ui_element->transform.min_w, val);
    else if (strcmp(key, "max_w") == 0)
        parse_param_as_int(&ui_element->transform.max_w, val);
    else if (strcmp(key, "min_h") == 0)
        parse_param_as_int(&ui_element->transform.min_h, val);
    else if (strcmp(key, "max_h") == 0)
        parse_param_as_int(&ui_element->transform.max_h, val);
    else if (strcmp(key, "off_x") == 0)
        parse_param_as_int(&ui_element->transform.off_x, val);
    else if (strcmp(key, "off_y") == 0)
        parse_param_as_int(&ui_element->transform.off_y, val);
    else if (strcmp(key, "color") == 0)
        parse_param_as_color(&ui_element->style.color, val);
    else if (strcmp(key, "background_color") == 0)
        parse_param_as_color(&ui_element->style.background_color, val);
    else if (strcmp(key, "border_color") == 0)
        parse_param_as_color(&ui_element->style.border_color, val);
    else if (strcmp(key, "border_strengh") == 0)
        parse_param_as_int(&ui_element->style.border_strengh, val);
    else
        printf("[UI][WARNING] invalid style name \"%s\"\n", key);
}

void ui_parse_style(UIElement ui_element, const char* style) {
    char* copy = malloc(strlen(style) + 1);
    strcpy(copy, style);
    char* strtok_r_state;
    char* current = strtok_r(copy, ";", &strtok_r_state);
    do  {
        parse_single_style(ui_element, current);
    } while((current = strtok_r(NULL, ";", &strtok_r_state)));
    free(copy);
}