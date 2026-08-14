#pragma once
#include <string>
#include <functional>

struct SDL_Window;
typedef void *SDL_GLContext;

namespace mc {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const { return closeRequested; }
    void pollEvents();
    void swapBuffers();

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getAspectRatio() const { return height > 0 ? static_cast<float>(width) / height : 1.0f; }

    void setCursorLocked(bool locked);
    bool isCursorLocked() const { return cursorLocked; }

    SDL_Window* getHandle() const { return handle; }

    std::function<void(int width, int height)> onResize;
    std::function<void(double xpos, double ypos, double dx, double dy)> onMouseMove;
    std::function<void(int key, int scancode, int action, int mods)> onKey;
    std::function<void(int button, int action, int mods)> onMouseButton;

private:
    SDL_Window* handle{nullptr};
    SDL_GLContext glContext{nullptr};
    int width{1280};
    int height{720};
    bool closeRequested{false};
    bool cursorLocked{false};
};

} // namespace mc
