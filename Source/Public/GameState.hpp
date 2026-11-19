#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>

enum class CameraMode {
    TopDown,
    PlayerPerspective
};

struct CardVisual {
    int textureIndex;
    int playSlot;
    bool played;
    glm::vec3 bottomAnchor;
    float pitchDegrees;
};

struct CameraPose {
    glm::vec3 position;
    glm::vec3 target;
    float fieldOfViewDegrees;
};

class GameState {
public:
    static constexpr std::size_t CardCount = 5;
    static constexpr float TableTopY = 1.01F;
    static constexpr float CardWidth = 0.62F;
    static constexpr float CardHeight = 0.91F;
    static constexpr float CardSpacing = 0.86F;
    static constexpr float CardFlightDuration = 0.28F;
    static constexpr float DeckRowZ = 1.19F;
    static constexpr float PlayRowZ = 0.0F;
    static constexpr float OpponentRowZ = -1.19F;

    explicit GameState(std::uint32_t randomSeed = std::random_device{}());

    void update(float deltaTime);
    bool playRandomCard();
    void setCameraMode(CameraMode mode);

    [[nodiscard]] CameraMode cameraMode() const;
    [[nodiscard]] float cameraBlend() const;
    [[nodiscard]] std::size_t remainingCardCount() const;
    [[nodiscard]] std::vector<CardVisual> cardVisuals() const;
    [[nodiscard]] CameraPose cameraPose() const;
    [[nodiscard]] static float slotX(std::size_t slot);

private:
    struct CardState {
        bool played = false;
        int playSlot = -1;
        double startTime = 0.0;
    };

    [[nodiscard]] static float easeInOut(float value);

    std::array<CardState, CardCount> cards_{};
    std::array<int, CardCount> remainingCards_{};
    std::size_t remainingCardCount_ = CardCount;
    std::size_t nextPlaySlot_ = 0;
    std::mt19937 randomEngine_;
    double elapsedTime_ = 0.0;
    CameraMode cameraMode_ = CameraMode::TopDown;
    float cameraBlend_ = 0.0F;
    float cameraTransitionFrom_ = 0.0F;
    float cameraTransitionTo_ = 0.0F;
    float cameraTransitionProgress_ = 1.0F;
};

int runGameStateTests();
