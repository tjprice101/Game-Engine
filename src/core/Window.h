#pragma once
#include <string>
#include <functional>

struct GLFWwindow;

struct WindowConfig {
    int         width      = 1280;
    int         height     = 720;
    std::string title      = "Game Engine";
    bool        vsync      = true;
    bool        resizable  = true;
    bool        fullscreen = false;
};

class Window {
public:
    explicit Window(const WindowConfig& cfg = {});
    ~Window();

    // Non-copyable
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    bool     shouldClose()  const;
    void     swapBuffers()  const;
    void     pollEvents()   const;
    void     close();

    int      width()        const { return m_width; }
    int      height()       const { return m_height; }
    float    aspectRatio()  const;
    GLFWwindow* handle()    const { return m_handle; }

    void setResizeCallback(std::function<void(int,int)> cb) { m_resizeCb = std::move(cb); }

private:
    static void framebufferSizeCallback(GLFWwindow* w, int width, int height);

    GLFWwindow* m_handle = nullptr;
    int         m_width  = 0;
    int         m_height = 0;
    std::function<void(int,int)> m_resizeCb;
};
