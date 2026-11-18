#include <cassert>
#include <filesystem>

int main(int argc, char** argv) {
    assert (argc > 0);
    std::filesystem::path exePath = std::filesystem::canonical (argv[0]).parent_path().parent_path();
    std::filesystem::current_path (exePath);
}