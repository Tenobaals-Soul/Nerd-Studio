#ifndef UI_H
#define UI_H
#include <types.h>

enum UIType {
    UI_NO_TYPE, UI_CANVAS
};

typedef struct UIStyleSheet {
    color32 color;
    color32 background_color;
    color32 border_color;
    int border_strengh;
}* UIStyleSheet;

typedef struct UIElement* UIElement;

UIElement ui_canvas();
void ui_draw(UIElement ui_element, int window_w, int window_h);
void ui_mouse_down(UIElement ui_element, int button, int x, int y);
void ui_mouse_up(UIElement ui_element, int button, int x, int y);
void ui_mouse_moved(UIElement ui_element, int x, int y);

enum UI_CONSTS {
    UI_WIDTH,
    UI_HEIGHT,
    UI_X,
    UI_Y,
    UI_MIN_WIDTH,
    UI_MAX_WIDTH,
    UI_MIN_HEIGHT,
    UI_MAX_HEIGHT,
    UI_CHILD_COUNT
};

void ui_set_i(UIElement ui_element, int param, int val);
int ui_get_i(UIElement ui_element, int param);
void ui_set_d(UIElement ui_element, int param, double val);
double ui_get_d(UIElement ui_element, int param);

UIStyleSheet ui_access_stylesheet(UIElement ui_element);

#endif