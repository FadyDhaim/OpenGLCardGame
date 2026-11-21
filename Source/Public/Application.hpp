#pragma once

#include <filesystem>

#include "GameState.hpp"

struct GLFWwindow;

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    static void handleKey(GLFWwindow* window, int key, int scanCode, int action, int modifiers);

    GLFWwindow* window_ = nullptr;
    std::filesystem::path projectRoot_;
    GameState gameState_;
};
