#pragma once
#include <array>
#include <glm/glm.hpp>

struct GLFWwindow;

// Keys mirror GLFW key codes so callers can use Input::Key::A etc.
namespace Key {
    enum : int {
        Space = 32, Apostrophe = 39, Comma = 44, Minus = 45, Period = 46,
        Slash = 47,
        Num0=48,Num1,Num2,Num3,Num4,Num5,Num6,Num7,Num8,Num9,
        Semicolon=59, Equal=61,
        A=65,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
        Escape=256, Enter=257, Tab=258, Backspace=259,
        Right=262, Left=263, Down=264, Up=265,
        LeftShift=340, LeftCtrl=341, LeftAlt=342,
        RightShift=344, RightCtrl=345, RightAlt=346,
        F1=290,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,
        _Count = 512
    };
}

namespace MouseButton {
    enum : int { Left=0, Right=1, Middle=2, _Count=8 };
}

class Input {
public:
    static void init(GLFWwindow* window);

    // Call once per frame to advance state
    static void update();

    static bool keyDown(int key);          // Held this frame
    static bool keyPressed(int key);       // Just pressed
    static bool keyReleased(int key);      // Just released

    static bool mouseDown(int btn);
    static bool mousePressed(int btn);
    static bool mouseReleased(int btn);

    static glm::vec2 mousePos();           // Screen coords
    static glm::vec2 mouseDelta();         // Screen delta since last frame
    static float     scrollDelta();        // Scroll wheel delta

private:
    static void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow*, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow*, double x, double y);
    static void scrollCallback(GLFWwindow*, double xoff, double yoff);

    static std::array<bool, Key::_Count>         s_keysCur;
    static std::array<bool, Key::_Count>         s_keysPrev;
    static std::array<bool, MouseButton::_Count> s_mouseCur;
    static std::array<bool, MouseButton::_Count> s_mousePrev;
    static glm::vec2 s_mousePos;
    static glm::vec2 s_mousePrev_;
    static float     s_scrollDelta;
    static float     s_scrollAccum;
};
