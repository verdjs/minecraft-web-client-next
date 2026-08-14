#include "Window.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <stdexcept>

namespace mc {

Window::Window(int width, int height, const std::string& title)
    : width(width), height(height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        throw std::runtime_error(std::string("Failed to initialize SDL2: ") + SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    handle = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!handle) {
        SDL_Quit();
        throw std::runtime_error(std::string("Failed to create SDL2 window: ") + SDL_GetError());
    }

    glContext = SDL_GL_CreateContext(handle);
    if (!glContext) {
        SDL_DestroyWindow(handle);
        SDL_Quit();
        throw std::runtime_error(std::string("Failed to create OpenGL context: ") + SDL_GetError());
    }

    SDL_GL_SetSwapInterval(1); // Enable VSync for rock solid 60/120 FPS

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD OpenGL loader");
    }

    int drawableW, drawableH;
    SDL_GL_GetDrawableSize(handle, &drawableW, &drawableH);
    this->width = drawableW;
    this->height = drawableH;
    glViewport(0, 0, drawableW, drawableH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Window::~Window() {
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
    }
    if (handle) {
        SDL_DestroyWindow(handle);
    }
    SDL_Quit();
}

void Window::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            closeRequested = true;
        } else if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                int dw, dh;
                SDL_GL_GetDrawableSize(handle, &dw, &dh);
                width = dw;
                height = dh;
                glViewport(0, 0, dw, dh);
                if (onResize) onResize(dw, dh);
            }
        } else if (e.type == SDL_MOUSEMOTION) {
            if (cursorLocked && onMouseMove) {
                onMouseMove(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
            }
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            if (onKey) {
                int action = (e.type == SDL_KEYDOWN) ? 1 : 0;
                onKey(e.key.keysym.sym, e.key.keysym.scancode, action, e.key.keysym.mod);
            }
        } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            if (onMouseButton) {
                int action = (e.type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
                onMouseButton(e.button.button, action, 0);
            }
        }
    }
}

void Window::swapBuffers() {
    SDL_GL_SwapWindow(handle);
}

void Window::setCursorLocked(bool locked) {
    cursorLocked = locked;
    SDL_SetRelativeMouseMode(locked ? SDL_TRUE : SDL_FALSE);
}

} // namespace mc
