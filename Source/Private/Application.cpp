#include "Application.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Scene.hpp"

Application::Application()
    : projectRoot_ (std::filesystem::current_path()) {
    glfwSetErrorCallback (
        [] (int, const char* description) {
            std::cerr << "GLFW: " << description << '\n';
        }
    );
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error ("GLFW initialization failed");
    }

    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint (GLFW_SRGB_CAPABLE, GLFW_TRUE);
    window_ = glfwCreateWindow (1440, 900, "Card Table", nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error ("Unable to create an OpenGL 4.6 window");
    }

    glfwMakeContextCurrent (window_);
    const auto loadFunction = [] (const char* name) -> void* {
        return reinterpret_cast <void*> (glfwGetProcAddress (name));
    };
    if (gladLoadGLLoader (loadFunction) == 0 || !GLAD_GL_VERSION_4_6) {
        glfwDestroyWindow (window_);
        window_ = nullptr;
        glfwTerminate();
        throw std::runtime_error ("Unable to load OpenGL 4.6 functions");
    }

    glfwSetWindowUserPointer (window_, this);
    glfwSetKeyCallback (window_, handleKey);
    glfwSwapInterval (1);
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);
    glEnable (GL_FRAMEBUFFER_SRGB);
}

Application::~Application() {
    if (window_ != nullptr) {
        glfwDestroyWindow (window_);
    }
    glfwTerminate();
}

int Application::run() {
    Scene scene;
    scene.initialize (projectRoot_);
    double previousFrameTime = glfwGetTime();

    while (glfwWindowShouldClose (window_) == GLFW_FALSE) {
        const double currentFrameTime = glfwGetTime();
        const double deltaTime = std::min (currentFrameTime - previousFrameTime, 0.05);
        previousFrameTime = currentFrameTime;

        gameState_.update (static_cast <float> (deltaTime));
        glfwPollEvents();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize (window_, &framebufferWidth, &framebufferHeight);
        if (framebufferWidth > 0 && framebufferHeight > 0) {
            glViewport (0, 0, framebufferWidth, framebufferHeight);
            glClearColor (0.055F, 0.045F, 0.07F, 1.0F);
            glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene.render (gameState_, framebufferWidth, framebufferHeight);
            glfwSwapBuffers (window_);
        } else {
            glfwWaitEventsTimeout (0.05);
        }
    }
    return 0;
}

void Application::handleKey(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) {
        return;
    }

    auto& gameState = static_cast <Application*> (glfwGetWindowUserPointer (window))->gameState_;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose (window, GLFW_TRUE);
            break;
        case GLFW_KEY_G: gameState.playRandomCard();
            break;
        case GLFW_KEY_1: gameState.setCameraMode (CameraMode::TopDown);
            break;
        case GLFW_KEY_2: gameState.setCameraMode (CameraMode::PlayerPerspective);
            break;
        default: break;
    }
}
