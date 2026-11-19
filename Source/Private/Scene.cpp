#include "Scene.hpp"

#include <array>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float RoomWidth = 10.0F;
constexpr float RoomHeight = 4.2F;
constexpr float RoomDepth = 10.0F;
constexpr float TableTop = GameState::TableTopY;

glm::mat4 translatedScaled(const glm::vec3& position, const glm::vec3& scale) {
    return glm::translate(glm::mat4(1.0F), position) * glm::scale(glm::mat4(1.0F), scale);
}

Material solid(const glm::vec3& color, float emissive = 0.0F) {
    Material material;
    material.color = color;
    material.emissive = emissive;
    return material;
}
}

void Scene::initialize(const std::filesystem::path& projectRoot) {
    renderer_.initialize(projectRoot);
    const std::filesystem::path assets = projectRoot / "Assets/Textures";
    wallpaperTexture_ = renderer_.loadTexture(assets / "Wallpapers/Royal2K/Diffuse.jpg", true);
    floorDiffuseTexture_ = renderer_.loadTexture(assets / "Floors/Wooden/HinokiPlanks2K/Diffuse.png", true);
    floorNormalTexture_ = renderer_.loadTexture(assets / "Floors/Wooden/HinokiPlanks2K/Normal.png", false);
    floorPackedTexture_ = renderer_.loadTexture(assets / "Floors/Wooden/HinokiPlanks2K/AO-Rough-Metal.png", false);
    posterTexture_ = renderer_.loadTexture(assets / "Posters/TornArtPoster.png", true);

    constexpr std::array cardFiles{
            "DarkMagician.png",
            "BlueEyesWhiteDragon.png",
            "RedEyesBlackDragon.png",
            "DarkMagicianGirl.png",
            "BusterBlader.png"
    };
    for (std::size_t index = 0; index < cardFiles.size(); ++index) {
        cardTextures_[index] = renderer_.loadTexture(assets / "Cards" / cardFiles[index], true);
    }
}

void Scene::render(const GameState& gameState, int framebufferWidth, int framebufferHeight) {
    const CameraPose camera = gameState.cameraPose();
    const glm::vec3 direction = glm::normalize(camera.target - camera.position);
    const glm::vec3 up = std::abs(glm::dot(direction, glm::vec3(0.0F, 1.0F, 0.0F))) > 0.98F
                                 ? glm::vec3(0.0F, 0.0F, -1.0F)
                                 : glm::vec3(0.0F, 1.0F, 0.0F);
    const glm::mat4 view = glm::lookAt(camera.position, camera.target, up);
    const glm::mat4 projection = glm::perspective(glm::radians(camera.fieldOfViewDegrees),
                                                   static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
                                                   0.08F,
                                                   60.0F);
    renderer_.beginFrame(projection * view, camera.position);

    drawRoom();
    drawTable();
    drawProps();
    drawCards(gameState);
}

glm::mat4 Scene::planeTransform(const glm::vec3& position,
                                const glm::vec3& dimensions,
                                float rotationXDegrees,
                                float rotationYDegrees) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0F), position);
    transform = glm::rotate(transform, glm::radians(rotationYDegrees), {0.0F, 1.0F, 0.0F});
    transform = glm::rotate(transform, glm::radians(rotationXDegrees), {1.0F, 0.0F, 0.0F});
    return glm::scale(transform, dimensions);
}

glm::mat4 Scene::boxTransform(const glm::vec3& position, const glm::vec3& dimensions) {
    return translatedScaled(position, dimensions);
}

void Scene::drawRoom() {
    Material wallpaper;
    wallpaper.diffuseTexture = wallpaperTexture_;
    wallpaper.uvScale = {2.0F, 1.25F};
    renderer_.draw(Primitive::Plane,
                   planeTransform({0.0F, 0.012F, 0.0F}, {RoomWidth, RoomDepth, 1.0F}, -90.0F),
                   Material{.diffuseTexture = floorDiffuseTexture_,
                            .normalTexture = floorNormalTexture_,
                            .packedTexture = floorPackedTexture_,
                            .uvScale = {3.0F, 3.0F}});

    renderer_.draw(Primitive::Plane, planeTransform({0.0F, RoomHeight * 0.5F, -RoomDepth * 0.5F},
                                                     {RoomWidth, RoomHeight, 1.0F}), wallpaper);
    renderer_.draw(Primitive::Plane, planeTransform({0.0F, RoomHeight * 0.5F, RoomDepth * 0.5F},
                                                     {RoomWidth, RoomHeight, 1.0F}, 0.0F, 180.0F), wallpaper);
    renderer_.draw(Primitive::Plane, planeTransform({-RoomWidth * 0.5F, RoomHeight * 0.5F, 0.0F},
                                                     {RoomDepth, RoomHeight, 1.0F}, 0.0F, 90.0F), wallpaper);
    renderer_.draw(Primitive::Plane, planeTransform({RoomWidth * 0.5F, RoomHeight * 0.5F, 0.0F},
                                                     {RoomDepth, RoomHeight, 1.0F}, 0.0F, -90.0F), wallpaper);
    renderer_.draw(Primitive::Plane, planeTransform({0.0F, RoomHeight, 0.0F},
                                                     {RoomWidth, RoomDepth, 1.0F}, 90.0F), wallpaper);

}

void Scene::drawTable() {
    renderer_.draw(Primitive::Cube, boxTransform({0.0F, 0.89F, 0.0F}, {5.25F, 0.24F, 4.35F}),
                   solid({0.30F, 0.15F, 0.075F}));
    renderer_.draw(Primitive::Cube, boxTransform({0.0F, 0.745F, 0.0F}, {4.65F, 0.12F, 3.72F}),
                   solid({0.085F, 0.20F, 0.18F}));

    for (const float x : {-2.25F, 2.25F}) {
        for (const float z : {-1.72F, 1.72F}) {
            renderer_.draw(Primitive::Cube, boxTransform({x, 0.40F, z}, {0.25F, 0.80F, 0.25F}),
                           solid({0.23F, 0.115F, 0.055F}));
        }
    }
    renderer_.draw(Primitive::Cube, boxTransform({0.0F, 0.56F, -1.72F}, {4.55F, 0.20F, 0.22F}),
                   solid({0.20F, 0.095F, 0.045F}));
    renderer_.draw(Primitive::Cube, boxTransform({0.0F, 0.56F, 1.72F}, {4.55F, 0.20F, 0.22F}),
                   solid({0.20F, 0.095F, 0.045F}));

    const auto row = [this](float z, const glm::vec3& color) {
        renderer_.draw(Primitive::Plane,
                       planeTransform({0.0F, TableTop + 0.005F, z}, {4.45F, 0.91F, 1.0F}, -90.0F),
                       solid(color));
    };
    row(GameState::DeckRowZ, {0.12F, 0.29F, 0.24F});
    row(GameState::PlayRowZ, {0.16F, 0.32F, 0.27F});
    row(GameState::OpponentRowZ, {0.12F, 0.29F, 0.24F});
    for (const float z : {-0.595F, 0.595F}) {
        renderer_.draw(Primitive::Cube, boxTransform({0.0F, TableTop + 0.012F, z}, {4.55F, 0.012F, 0.025F}),
                       solid({0.79F, 0.58F, 0.28F}));
    }
    for (const float rowZ : {GameState::DeckRowZ, GameState::PlayRowZ, GameState::OpponentRowZ}) {
        for (std::size_t slot = 0; slot < GameState::CardCount; ++slot) {
            renderer_.draw(Primitive::Plane,
                           planeTransform({GameState::slotX(slot), TableTop + 0.014F, rowZ},
                                          {GameState::CardWidth + 0.05F, GameState::CardHeight + 0.05F, 1.0F}, -90.0F),
                           solid({0.075F, 0.21F, 0.18F}));
        }
    }
}

void Scene::drawProps() {
    const float posterZ = -RoomDepth * 0.5F + 0.030F;
    constexpr float posterWidth = 2.45F;
    constexpr float posterHeight = 1.45F;
    constexpr float posterBorder = 0.075F;
    const float posterBorderZ = posterZ - 0.015F;
    const Material posterFrame = solid({0.22F, 0.13F, 0.10F});
    for (const float x : {-(posterWidth + posterBorder) * 0.5F, (posterWidth + posterBorder) * 0.5F}) {
        renderer_.draw(Primitive::Plane,
                       planeTransform({x, 2.85F, posterBorderZ}, {posterBorder, posterHeight, 1.0F}),
                       posterFrame);
    }
    for (const float y : {2.85F - (posterHeight + posterBorder) * 0.5F,
                          2.85F + (posterHeight + posterBorder) * 0.5F}) {
        renderer_.draw(Primitive::Plane,
                       planeTransform({0.0F, y, posterBorderZ}, {posterWidth + 2.0F * posterBorder, posterBorder, 1.0F}),
                       posterFrame);
    }
    Material poster;
    poster.diffuseTexture = posterTexture_;
    poster.emissive = 0.10F;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    renderer_.draw(Primitive::Plane,
                   planeTransform({0.0F, 2.85F, posterZ}, {posterWidth, posterHeight, 1.0F}),
                   poster);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    renderer_.draw(Primitive::Cylinder, translatedScaled({-4.12F, 0.43F, -0.10F}, {0.72F, 0.86F, 0.72F}),
                   solid({0.57F, 0.23F, 0.14F}));
    renderer_.draw(Primitive::Cylinder, translatedScaled({-4.12F, 0.90F, -0.10F}, {0.61F, 0.10F, 0.61F}),
                   solid({0.20F, 0.12F, 0.075F}));
    renderer_.draw(Primitive::Cylinder, translatedScaled({-4.12F, 1.40F, -0.10F}, {0.09F, 1.05F, 0.09F}),
                   solid({0.18F, 0.34F, 0.13F}));
    for (int leaf = 0; leaf < 5; ++leaf) {
        const float angle = static_cast<float>(leaf) * 1.25663706F;
        const glm::vec3 leafPosition{-4.12F + std::cos(angle) * 0.28F, 1.62F + (leaf % 2) * 0.14F,
                                     -0.10F + std::sin(angle) * 0.28F};
        glm::mat4 leafTransform = glm::translate(glm::mat4(1.0F), leafPosition);
        leafTransform = glm::rotate(leafTransform, angle, {0.0F, 1.0F, 0.0F});
        leafTransform = glm::rotate(leafTransform, glm::radians(-28.0F), {0.0F, 0.0F, 1.0F});
        renderer_.draw(Primitive::Cone, glm::scale(leafTransform, {0.36F, 0.92F, 0.36F}),
                       solid({0.16F + 0.025F * leaf, 0.37F, 0.17F}));
    }

    renderer_.draw(Primitive::Cube, boxTransform({4.38F, 1.00F, -0.45F}, {0.26F, 2.00F, 1.45F}),
                   solid({0.31F, 0.17F, 0.10F}));
    renderer_.draw(Primitive::Cube, boxTransform({3.88F, 0.76F, -0.45F}, {0.92F, 0.10F, 1.48F}),
                   solid({0.36F, 0.20F, 0.12F}));
    renderer_.draw(Primitive::Cube, boxTransform({3.88F, 1.50F, -0.45F}, {0.92F, 0.10F, 1.48F}),
                   solid({0.36F, 0.20F, 0.12F}));
    const std::array<glm::vec3, 5> bookColors{{
            {0.58F, 0.16F, 0.13F}, {0.12F, 0.29F, 0.44F}, {0.73F, 0.47F, 0.16F},
            {0.30F, 0.40F, 0.23F}, {0.48F, 0.25F, 0.38F}
    }};
    for (std::size_t book = 0; book < bookColors.size(); ++book) {
        const float width = book % 2 == 0 ? 0.22F : 0.26F;
        const float height = 0.55F + static_cast<float>(book % 3) * 0.10F;
        renderer_.draw(Primitive::Cube,
                       boxTransform({3.58F + static_cast<float>(book) * 0.20F, 0.81F + height * 0.5F, -0.45F},
                                    {width, height, 0.72F}),
                       solid(bookColors[book]));
    }

    renderer_.draw(Primitive::Cylinder, translatedScaled({0.0F, 3.91F, 0.0F}, {0.035F, 0.50F, 0.035F}),
                   solid({0.20F, 0.17F, 0.13F}));
    renderer_.draw(Primitive::Cone, translatedScaled({0.0F, 3.61F, 0.0F}, {0.54F, 0.34F, 0.54F}),
                   solid({0.28F, 0.23F, 0.15F}));
    renderer_.draw(Primitive::Sphere, translatedScaled({0.0F, 3.50F, 0.0F}, {0.18F, 0.18F, 0.18F}),
                   solid({1.0F, 0.79F, 0.43F}, 5.0F));
}

void Scene::drawCards(const GameState& gameState) {
    for (const CardVisual& card : gameState.cardVisuals()) {
        const glm::vec3 anchor = card.bottomAnchor;
        glm::mat4 model = glm::translate(glm::mat4(1.0F), anchor);
        model = glm::rotate(model, glm::radians(-card.pitchDegrees), {1.0F, 0.0F, 0.0F});
        model = glm::translate(model, {0.0F, GameState::CardHeight * 0.5F, 0.0F});
        model = glm::scale(model, {GameState::CardWidth, GameState::CardHeight, 1.0F});
        Material cardMaterial;
        cardMaterial.diffuseTexture = cardTextures_[static_cast<std::size_t>(card.textureIndex)];
        cardMaterial.uvScale = {1.0F, 1.0F};
        cardMaterial.ambientFloor = 0.62F;
        cardMaterial.directLightScale = 0.22F;
        renderer_.draw(Primitive::Plane, model, cardMaterial);
    }
}