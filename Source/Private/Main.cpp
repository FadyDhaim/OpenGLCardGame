#include <cassert>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "Application.hpp"
#include "GameState.hpp"

int main(int argc, char** argv) {
    assert(argc > 0);

    try {
        const std::filesystem::path executablePath = std::filesystem::canonical(argv[0]);
        std::filesystem::current_path(executablePath.parent_path().parent_path());

        if (argc > 1 && std::string_view(argv[1]) == "--self-test") {
            return runGameStateTests();
        }

        Application application;
        return application.run();
    } catch (const std::exception& exception) {
        std::cerr << "OpenGLCardGame: " << exception.what() << '\n';
        return 1;
    }
}