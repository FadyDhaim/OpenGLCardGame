#pragma once

#include <filesystem>
#include <array>

#include "GameState.hpp"
#include "Renderer.hpp"

class Scene {
public:
    void initialize(const std::filesystem::path& projectRoot);
    void render(const GameState& gameState, int framebufferWidth, int framebufferHeight);

private:

    static glm::mat4 planeTransform(
        const glm::vec3& position,
        const glm::vec3& dimensions,
        float rotationXDegrees = 0.0F,
        float rotationYDegrees = 0.0F
    );


    static glm::mat4 boxTransform(const glm::vec3& position, const glm::vec3& dimensions);
    void drawRoom();
    void drawTable();
    void drawProps();
    void drawCards(const GameState& gameState);

    Renderer renderer_;
    GLuint wallpaperTexture_ = 0;
    GLuint floorDiffuseTexture_ = 0;
    GLuint floorNormalTexture_ = 0;
    GLuint floorPackedTexture_ = 0;
    GLuint posterTexture_ = 0;
    std::array <GLuint, GameState::CardCount> cardTextures_ {};
};
