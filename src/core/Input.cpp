#include "Input.h"
#include <GLFW/glfw3.h>
#include <algorithm>

std::array<bool, Key::_Count>         Input::s_keysCur   = {};
std::array<bool, Key::_Count>         Input::s_keysPrev  = {};
std::array<bool, MouseButton::_Count> Input::s_mouseCur  = {};
std::array<bool, MouseButton::_Count> Input::s_mousePrev = {};
glm::vec2 Input::s_mousePos    = {0.f, 0.f};
glm::vec2 Input::s_mousePrev_  = {0.f, 0.f};
float     Input::s_scrollDelta = 0.f;
float     Input::s_scrollAccum = 0.f;

void Input::init(GLFWwindow* window) {
    glfwSetKeyCallback        (window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback  (window, cursorPosCallback);
    glfwSetScrollCallback     (window, scrollCallback);
}

void Input::update() {
    s_keysPrev   = s_keysCur;
    s_mousePrev  = s_mouseCur;
    s_mousePrev_ = s_mousePos;
    s_scrollDelta = s_scrollAccum;
    s_scrollAccum = 0.f;
}

bool Input::keyDown    (int key) { return key>=0&&key<Key::_Count && s_keysCur[key]; }
bool Input::keyPressed (int key) { return key>=0&&key<Key::_Count && s_keysCur[key] && !s_keysPrev[key]; }
bool Input::keyReleased(int key) { return key>=0&&key<Key::_Count && !s_keysCur[key] && s_keysPrev[key]; }

bool Input::mouseDown    (int b) { return b>=0&&b<MouseButton::_Count && s_mouseCur[b]; }
bool Input::mousePressed (int b) { return b>=0&&b<MouseButton::_Count && s_mouseCur[b]&&!s_mousePrev[b]; }
bool Input::mouseReleased(int b) { return b>=0&&b<MouseButton::_Count && !s_mouseCur[b]&&s_mousePrev[b]; }

glm::vec2 Input::mousePos()   { return s_mousePos; }
glm::vec2 Input::mouseDelta() { return s_mousePos - s_mousePrev_; }
float     Input::scrollDelta(){ return s_scrollDelta; }

// --- Callbacks -----------------------------------------------------------
void Input::keyCallback(GLFWwindow*, int key, int /*scan*/, int action, int /*mods*/) {
    if (key >= 0 && key < Key::_Count) {
        if      (action == GLFW_PRESS)   s_keysCur[key] = true;
        else if (action == GLFW_RELEASE) s_keysCur[key] = false;
    }
}
void Input::mouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/) {
    if (button >= 0 && button < MouseButton::_Count) {
        s_mouseCur[button] = (action == GLFW_PRESS);
    }
}
void Input::cursorPosCallback(GLFWwindow*, double x, double y) {
    s_mousePos = { static_cast<float>(x), static_cast<float>(y) };
}
void Input::scrollCallback(GLFWwindow*, double /*x*/, double y) {
    s_scrollAccum += static_cast<float>(y);
}
