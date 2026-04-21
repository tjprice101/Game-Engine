#include "Window.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

Window::Window(const WindowConfig& cfg) {
    if (!glfwInit())
        throw std::runtime_error("glfwInit() failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, cfg.resizable ? GLFW_TRUE : GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWmonitor* monitor = cfg.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_handle = glfwCreateWindow(cfg.width, cfg.height, cfg.title.c_str(), monitor, nullptr);
    if (!m_handle) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow() failed");
    }

    m_width  = cfg.width;
    m_height = cfg.height;

    glfwMakeContextCurrent(m_handle);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_handle);
        glfwTerminate();
        throw std::runtime_error("gladLoadGL() failed");
    }

    glfwSwapInterval(cfg.vsync ? 1 : 0);

    // Store pointer to this for callbacks
    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, framebufferSizeCallback);

    glViewport(0, 0, m_width, m_height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Window::~Window() {
    if (m_handle) glfwDestroyWindow(m_handle);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_handle); }
void Window::swapBuffers()  const { glfwSwapBuffers(m_handle); }
void Window::pollEvents()   const { glfwPollEvents(); }
void Window::close()              { glfwSetWindowShouldClose(m_handle, GLFW_TRUE); }

float Window::aspectRatio() const {
    return m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
}

void Window::framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self) {
        self->m_width  = width;
        self->m_height = height;
        if (self->m_resizeCb) self->m_resizeCb(width, height);
    }
}
