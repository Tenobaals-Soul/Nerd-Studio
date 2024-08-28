#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <types.h>
#include <stdlib.h>
#include <ui.h>
#include <stdio.h>

struct program_state {
    GLFWwindow* window;
    struct user_config {
        color32 background_color;
    } user_config;
    UIElement left_ui;
    UIElement right_ui;
    UIElement resizer_left;
    UIElement resizer_right;
    UIElement* toolbox_buttons;
    GLFWcursor* standart_cur;
    GLFWcursor* resize_ew_cur;
    GLFWcursor* resize_ns_cur;
};

static void display_func(struct program_state* program_state) {
    glClearColor(program_state->user_config.background_color.r / 255.0,
                 program_state->user_config.background_color.g / 255.0,
                 program_state->user_config.background_color.b / 255.0,
                 program_state->user_config.background_color.a / 255.0);
    glClear(GL_COLOR_BUFFER_BIT);
    int w, h;
    glfwGetFramebufferSize(program_state->window, &w, &h);
    ui_draw(program_state->left_ui, w, h);
    ui_draw(program_state->right_ui, w, h);

    glfwSwapBuffers(program_state->window);
}

static void close_func(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    // glfwSetWindowShouldClose(window, GLFW_FALSE);
}

static void set_gl_coordinates(int w, int h) {
    gluOrtho2D(0, w, 0, h);
    glViewport(0, 0, w, h);
}

static void resize_func(GLFWwindow* window, int x, int y) {
    struct program_state* program_state = glfwGetWindowUserPointer(window);
    glLoadIdentity();
    set_gl_coordinates(x, y);

    ui_resize(program_state->resizer_left, x, y);
    ui_resize(program_state->resizer_right, x, y);
}

static void move_func(GLFWwindow* window, double x, double y) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    y = h - y;
    struct program_state* program_state = glfwGetWindowUserPointer(window);
    glfwSetCursor(window, program_state->standart_cur);
    ui_mouse_moved(program_state->resizer_left, x, y);
    ui_mouse_moved(program_state->resizer_right, x, y);
}

static void mouse_func(GLFWwindow* window, int button, int action, int mods) {
    (void) mods;
    struct program_state* program_state = glfwGetWindowUserPointer(window);
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    y = h - y;
    if (action == GLFW_PRESS) {
        ui_mouse_down(program_state->left_ui, button + 1, x, y);
        ui_mouse_down(program_state->right_ui, button + 1, x, y);
    }
    else if (action == GLFW_RELEASE) {
        ui_mouse_up(program_state->left_ui, button + 1, x, y);
        ui_mouse_up(program_state->right_ui, button + 1, x, y);
    }
}

static void setup_window(struct program_state* program_state, int w, int h) {
    program_state->window = glfwCreateWindow(w, h, "Nerd Studio", NULL, NULL);
    if (!program_state->window) {
        glfwTerminate();
        exit(1);
    }
    glfwSetWindowUserPointer(program_state->window, program_state);
    glfwMakeContextCurrent(program_state->window);
    glfwSetWindowCloseCallback(program_state->window, close_func);
    glfwSetFramebufferSizeCallback(program_state->window, resize_func);
    glfwSetCursorPosCallback(program_state->window, move_func);
    glfwSetMouseButtonCallback(program_state->window, mouse_func);

    glLoadIdentity();
    set_gl_coordinates(w, h);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_BLEND);
}

void set_cur(void* user_data, enum ui_direction dir) {
    struct program_state* program_state = user_data;
    glfwSetCursor(program_state->window, dir == HORIZONTAL ?
        program_state->resize_ew_cur : program_state->resize_ns_cur);
}

static void setup_layout(struct program_state* program_state, int w, int h) {
    program_state->left_ui = ui_canvas(w, h);
    const char* style_canvas_l = "x=0; y=0; w=0.2; h=1; min_w=200; max_w=600";
    ui_parse_style(program_state->left_ui, style_canvas_l);
    program_state->resizer_left = ui_resizer(w, h, HORIZONTAL,
                                             program_state->left_ui, NULL, 1.5);
    const char* style_resizer_l = "y=0; w=1; h=1; min_w=4; max_w=4; off_x=-2";
    ui_parse_style(program_state->resizer_left, style_resizer_l);
    ui_resizer_set_curser_func(program_state->resizer_left, set_cur, program_state);
    ui_set_parent(program_state->resizer_left, program_state->left_ui);

    program_state->right_ui = ui_canvas(w, h);
    const char* style_canvas_r = "x=1; y=0; w=-0.2; h=1; min_w=-600; max_w=-200";
    ui_parse_style(program_state->right_ui, style_canvas_r);
    program_state->resizer_right = ui_resizer(w, h, HORIZONTAL,
                                              NULL, program_state->right_ui, 1.5);
    const char* style_resizer_r = "y=0; w=-1; h=1; min_w=-4; max_w=-4; off_x=2";
    ui_parse_style(program_state->resizer_right, style_resizer_r);
    ui_resizer_set_curser_func(program_state->resizer_right, set_cur, program_state);
    ui_set_parent(program_state->resizer_right, program_state->right_ui);
    
    program_state->toolbox_buttons = malloc(sizeof(UIElement) * 1);
    program_state->toolbox_buttons[0] = NULL;
}

static void user_data_init(struct program_state* program_state) {
    program_state->user_config.background_color = color32(0x80, 0x80, 0x80, 0xFF);
}

static void user_data_destroy(struct program_state* program_state) {
    for (int i = 0; program_state->toolbox_buttons[i]; i++)
        ui_free(program_state->toolbox_buttons[i]);
    free(program_state->toolbox_buttons);
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;
    struct program_state program_state;
    int w = 640, h = 480;
    user_data_init(&program_state);
    if (!glfwInit())
        exit(1);
    setup_window(&program_state, w, h);
    setup_layout(&program_state, w, h);

    program_state.standart_cur = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    program_state.resize_ew_cur = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    program_state.resize_ns_cur = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);

    while (!glfwWindowShouldClose(program_state.window)) {
        display_func(&program_state);
        glfwWaitEventsTimeout(1/30);
    }

    glfwDestroyCursor(program_state.standart_cur);
    glfwDestroyCursor(program_state.resize_ew_cur);
    glfwDestroyCursor(program_state.resize_ns_cur);
    glfwTerminate();
    user_data_destroy(&program_state);
    return 0;
}