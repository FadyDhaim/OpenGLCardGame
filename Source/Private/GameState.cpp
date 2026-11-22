#include "GameState.hpp"

#include <algorithm>

GameState::GameState(std::uint32_t randomSeed)
    : randomEngine_ (randomSeed) {
    for (std::size_t index = 0; index < CardCount; ++index) {
        remainingCards_[index] = static_cast <int> (index);
    }
}

void GameState::update(float deltaTime) {
    const float safeDeltaTime = std::max (deltaTime, 0.0F);
    elapsedTime_ += static_cast <double> (safeDeltaTime);

    if (cameraTransitionProgress_ < 1.0F) {
        cameraTransitionProgress_ = std::min (cameraTransitionProgress_ + safeDeltaTime / CameraTransitionDuration, 1.0F);
        const float easedProgress = easeInOut (cameraTransitionProgress_);
        cameraBlend_ = cameraTransitionFrom_ + (cameraTransitionTo_ - cameraTransitionFrom_) * easedProgress;
    }
}

bool GameState::playRandomCard() {
    if (remainingCardCount_ == 0) {
        return false;
    }

    std::uniform_int_distribution <std::size_t> selection (0, remainingCardCount_ - 1);
    const std::size_t selectedPosition = selection (randomEngine_);
    const int selectedCard = remainingCards_[selectedPosition];
    remainingCards_[selectedPosition] = remainingCards_[remainingCardCount_ - 1];
    --remainingCardCount_;

    CardState& card = cards_[static_cast <std::size_t> (selectedCard)];
    card.played = true;
    card.playSlot = static_cast <int> (nextPlaySlot_);
    card.startTime = elapsedTime_;
    ++nextPlaySlot_;
    return true;
}

void GameState::setCameraMode(CameraMode mode) {
    if (cameraMode_ == mode) {
        return;
    }

    cameraTransitionFrom_ = cameraBlend_;
    cameraTransitionTo_ = mode == CameraMode::PlayerPerspective ? 1.0F : 0.0F;
    cameraTransitionProgress_ = 0.0F;
    cameraMode_ = mode;
}

CameraMode GameState::cameraMode() const {
    return cameraMode_;
}

float GameState::cameraBlend() const {
    return cameraBlend_;
}

std::size_t GameState::remainingCardCount() const {
    return remainingCardCount_;
}

std::vector <CardVisual> GameState::cardVisuals() const {
    std::vector <CardVisual> visuals;
    visuals.reserve (CardCount);

    for (std::size_t index = 0; index < CardCount; ++index) {
        const CardState& card = cards_[index];
        float x = slotX (index);
        float rowCenterZ = DeckRowZ;
        float lift = 0.0F;

        if (card.played) {
            const float progress = glm::clamp (
                static_cast <float> ((elapsedTime_ - card.startTime) / CardFlightDuration),
                0.0F,
                1.0F
            );
            const float easedProgress = easeInOut (progress);
            const float destinationX = slotX (static_cast <std::size_t> (card.playSlot));
            x += (destinationX - x) * easedProgress;
            rowCenterZ += (PlayRowZ - rowCenterZ) * easedProgress;
            lift = glm::sin (easedProgress * std::numbers::pi_v <float>) * 0.12F;
        }

        const float pitchDegrees = (1.0F - cameraBlend_) * 90.0F;
        const float bottomEdgeOffset = CardHeight * 0.5F * glm::sin (glm::radians (pitchDegrees));
        visuals.push_back (
            {
                .textureIndex = static_cast <int> (index),
                .playSlot = card.playSlot,
                .played = card.played,
                .bottomAnchor = { x, TableTopY + 0.035F + lift, rowCenterZ + bottomEdgeOffset },
                .pitchDegrees = pitchDegrees
            }
        );
    }

    return visuals;
}

CameraPose GameState::cameraPose() const {
    constexpr glm::vec3 topDownPosition { 0.0F, 3.48F, 0.0F };
    constexpr glm::vec3 playerPosition { 0.0F, 2.18F, 6.85F };
    constexpr glm::vec3 topDownTarget { 0.0F, 0.88F, 0.0F };
    constexpr glm::vec3 playerTarget { 0.0F, 1.30F, -0.25F };

    return {
        .position = glm::mix (topDownPosition, playerPosition, cameraBlend_),
        .target = glm::mix (topDownTarget, playerTarget, cameraBlend_),
        .fieldOfViewDegrees = 82.0F - cameraBlend_ * 24.0F
    };
}

float GameState::slotX(std::size_t slot) {
    constexpr auto middle = (static_cast <float> (CardCount) - 1) / 2;
    return (static_cast <float> (slot) - middle) * CardSpacing;
}

float GameState::easeInOut(float value) {
    const float clampedValue = std::clamp (value, 0.0F, 1.0F);
    return clampedValue * clampedValue * (3.0F - 2.0F * clampedValue);
}
