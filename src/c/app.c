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
};

static void display_func(struct program_state* program_state) {
    glClearColor(program_state->user_config.background_color.r / 255.0,
                 program_state->user_config.background_color.g / 255.0,
                 program_state->user_config.background_color.b / 255.0,
                 program_state->user_config.background_color.a / 255.0);
    glClear(GL_COLOR_BUFFER_BIT);
    int w, h;
    glfwGetWindowSize(program_state->window, &w, &h);
    ui_draw(program_state->left_ui, w, h);
    ui_draw(program_state->right_ui, w, h);

    glfwSwapBuffers(program_state->window);
}

void close_func(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    // glfwSetWindowShouldClose(window, GLFW_FALSE);
}

void resize_func(GLFWwindow* window, int x, int y) {
    (void) window; (void) x; (void) y;
    glLoadIdentity();
    gluOrtho2D(0, x, 0, y);
    glViewport(0, 0, x, y);
}

static void setup_window(struct program_state* program_state) {
    int w = 640, h = 480;
    program_state->window = glfwCreateWindow(w, h, "Nerd Studio", NULL, NULL);
    if (!program_state->window) {
        glfwTerminate();
        exit(1);
    }
    glfwSetWindowUserPointer(program_state->window, &program_state);
    glfwMakeContextCurrent(program_state->window);
    glfwSetWindowCloseCallback(program_state->window, close_func);
    glfwSetFramebufferSizeCallback(program_state->window, resize_func);

    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glViewport(0, 0, w, h);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_BLEND);
}

static void setup_layout(struct program_state* program_state) {
    int w, h;
    glfwGetWindowSize(program_state->window, &w, &h);
    program_state->left_ui = ui_canvas();
    ui_set_d(program_state->left_ui, UI_X, 0);
    ui_set_d(program_state->left_ui, UI_Y, 0);
    ui_set_d(program_state->left_ui, UI_WIDTH, MAX(.2, 200 / w));
    ui_set_d(program_state->left_ui, UI_HEIGHT, 1);
    ui_set_i(program_state->left_ui, UI_MIN_WIDTH, 200);
    ui_set_i(program_state->left_ui, UI_MAX_WIDTH, 600);
    program_state->right_ui = ui_canvas();
    ui_set_d(program_state->right_ui, UI_X, 1);
    ui_set_d(program_state->right_ui, UI_Y, 0);
    ui_set_d(program_state->right_ui, UI_WIDTH, -MAX(.2, 200 / w));
    ui_set_d(program_state->right_ui, UI_HEIGHT, 1);
    ui_set_i(program_state->right_ui, UI_MIN_WIDTH, -600);
    ui_set_i(program_state->right_ui, UI_MAX_WIDTH, -200);
}

static void user_data_init(struct program_state* program_state) {
    program_state->user_config.background_color = color32(0x80, 0x80, 0x80, 0xFF);
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;
    struct program_state program_state;
    user_data_init(&program_state);
    if (!glfwInit())
        exit(1);
    setup_window(&program_state);
    setup_layout(&program_state);

    while (!glfwWindowShouldClose(program_state.window)) {
        display_func(&program_state);
        glfwWaitEventsTimeout(1/30);
    }

    glfwTerminate();
    return 0;
}