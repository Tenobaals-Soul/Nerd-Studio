#ifndef UI_H
#define UI_H
#include <types.h>

enum UIType {
    UI_NO_TYPE, UI_CANVAS, UI_RESIZER, UI_BUTTON
};

typedef struct UIStyleSheet {
    color32 color;
    color32 background_color;
    color32 border_color;
    int border_strengh;
}* UIStyleSheet;

typedef struct UIElement* UIElement;

enum ui_direction {
    HORIZONTAL, VERTICAL
};

enum UI_CONSTS {
    UI_WIDTH,
    UI_HEIGHT,
    UI_X,
    UI_Y,
    UI_OFFSET_X,
    UI_OFFSET_Y,
    UI_MIN_WIDTH,
    UI_MAX_WIDTH,
    UI_MIN_HEIGHT,
    UI_MAX_HEIGHT,
    UI_CHILD_COUNT
};

UIElement ui_canvas(int window_w, int window_h);
UIElement ui_resizer(int window_w, int window_h, enum ui_direction direction,
                     UIElement item1, UIElement item2, double size);
void ui_resizer_set_curser_func(UIElement ui_element, void (*curser_func)
                                (void* user_data, enum ui_direction),
                                void* user_data);
UIElement ui_button(int window_w, int window_h,
                    void (*on_click)(void* user_data), void* user_data);

void ui_free(UIElement ui_element);

void ui_draw(UIElement ui_element, int window_w, int window_h);
void ui_resize(UIElement ui_element, int window_w, int window_h);
void ui_mouse_down(UIElement ui_element, int button, int x, int y);
void ui_mouse_up(UIElement ui_element, int button, int x, int y);
void ui_mouse_moved(UIElement ui_element, int x, int y);

void ui_set_i(UIElement ui_element, int param, int val);
int ui_get_i(UIElement ui_element, int param);
void ui_set_d(UIElement ui_element, int param, double val);
double ui_get_d(UIElement ui_element, int param);
void ui_set_parent(UIElement ui_element, UIElement parent);
void ui_parse_style(UIElement ui_element, const char* style);

UIStyleSheet ui_access_stylesheet(UIElement ui_element);

#endif