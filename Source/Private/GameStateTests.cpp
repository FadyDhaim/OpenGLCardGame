#include "GameState.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error (std::string (message));
        }
    }
}

int runGameStateTests() {
    try {
        GameState state (0xC0FFEEU);
        const auto initialCards = state.cardVisuals();
        const CameraPose initialCamera = state.cameraPose();
        const auto cardCenterZ = [] (const CardVisual& card) {
            return card.bottomAnchor.z - GameState::CardHeight * 0.5F *
                   std::sin (glm::radians (card.pitchDegrees));
        };
        require (initialCards.size() == GameState::CardCount, "initial card count must be five");
        require (state.remainingCardCount() == GameState::CardCount, "all cards must begin in the deck");
        require (
            std::abs (initialCamera.position.x - initialCamera.target.x) < 0.001F &&
            std::abs (initialCamera.position.z - initialCamera.target.z) < 0.001F,
            "the top-down camera must look exactly vertically at the table"
        );
        for (std::size_t index = 0; index < initialCards.size(); ++index) {
            require (!initialCards[index].played, "initial cards must not be in play");
            require (initialCards[index].playSlot == -1, "initial cards must not reserve a play slot");
            require (
                std::abs (
                    initialCards[index].bottomAnchor.x - GameState::slotX (index)
                ) < 0.001F,
                "initial cards must be evenly spaced"
            );
            require (
                std::abs (cardCenterZ (initialCards[index]) - GameState::DeckRowZ) < 0.001F,
                "flat deck cards must be centered in the deck row"
            );
            require (
                std::abs (initialCards[index].pitchDegrees - 90.0F) < 0.001F,
                "top-down mode must lay cards flat toward the camera"
            );
        }
        const float overheadTableSpan = 2.0F * (initialCamera.position.y - GameState::TableTopY) *
                                        std::tan (glm::radians (initialCamera.fieldOfViewDegrees) / 2);
        require (
            overheadTableSpan > GameState::DeckRowZ - GameState::OpponentRowZ + GameState::CardHeight,
            "the overhead camera must include the full card-row depth"
        );

        GameState movingState (19U);
        require (movingState.playRandomCard(), "a card can start moving from the deck");
        movingState.update (GameState::CardFlightDuration * 0.5F);
        const auto halfwayCards = movingState.cardVisuals();
        const auto movingCard = std::find_if (
            halfwayCards.begin(),
            halfwayCards.end(),
            [] (const CardVisual& card) {
                return card.played && card.playSlot == 0;
            }
        );
        require (movingCard != halfwayCards.end(), "the played card must retain its reserved play slot");
        require (
            std::abs (cardCenterZ (*movingCard) - (GameState::DeckRowZ - GameState::PlayRowZ) * 0.5F) < 0.001F,
            "the card's depth center must be halfway through the two rows given a half-complete animation"
        );
        movingState.update (GameState::CardFlightDuration * 0.5F);
        const auto landedCards = movingState.cardVisuals();
        const auto landedCard = std::find_if (
            landedCards.begin(),
            landedCards.end(),
            [] (const CardVisual& card) {
                return card.played && card.playSlot == 0;
            }
        );
        require (
            landedCard != landedCards.end() &&
            std::abs (cardCenterZ (*landedCard) - GameState::PlayRowZ) < 0.001F,
            "the card must reach the play-row center after the quick flight"
        );

        for (std::size_t slot = 0; slot < GameState::CardCount; ++slot) {
            require (state.playRandomCard(), "each remaining card can be played");
            const auto cards = state.cardVisuals();
            int cardsInNewSlot = 0;
            for (const CardVisual& card: cards) {
                if (card.played && card.playSlot == static_cast <int> (slot)) {
                    ++cardsInNewSlot;
                }
            }
            require (cardsInNewSlot == 1, "each press must reserve one unique left-to-right slot");
        }
        require (state.remainingCardCount() == 0, "all five cards must be removed after five plays");
        require (!state.playRandomCard(), "playing with an empty deck must be a no-op");
        state.update (GameState::CardFlightDuration);
        for (const CardVisual& card: state.cardVisuals()) {
            require (card.played, "all 5 cards must have been played");
            const float expectedX = GameState::slotX (card.playSlot);
            require (
                std::abs (card.bottomAnchor.x - expectedX) < 0.001F,
                "completed cards must line up in their reserved play slots"
            );
            require (
                std::abs (cardCenterZ (card) - GameState::PlayRowZ) < 0.001F,
                "completed cards must be centered in the play row"
            );
        }

        GameState cameraState (7U);
        cameraState.setCameraMode (CameraMode::PlayerPerspective);
        cameraState.update (GameState::CameraTransitionDuration / 2);
        const float halfwayBlend = cameraState.cameraBlend();
        require (halfwayBlend > 0.0F && halfwayBlend < 1.0F, "camera movement must ease through intermediate poses");
        const auto transitioningCards = cameraState.cardVisuals();
        require (
            transitioningCards.front().pitchDegrees < 90.0F,
            "cards must rotate toward upright during the perspective transition"
        );
        for (const CardVisual& card: transitioningCards) {
            require (
                std::abs (cardCenterZ (card) - GameState::DeckRowZ) < 0.001F,
                "cards must stay centered in their row while rotating around the bottom edge"
            );
            require (
                std::abs (card.bottomAnchor.y - (GameState::TableTopY + 0.035F)) < 0.001F,
                "the bottom-edge pivot must remain on the table during camera movement"
            );
        }
        cameraState.setCameraMode (CameraMode::TopDown);
        require (
            std::abs (cameraState.cameraBlend() - halfwayBlend) < 0.001F,
            "interrupting a camera transition must preserve the current pose"
        );
        cameraState.update (GameState::CameraTransitionDuration);
        require (
            std::abs (cameraState.cameraBlend()) < 0.001F,
            "camera transition must return to the top-down endpoint"
        );
        for (const CardVisual& card: cameraState.cardVisuals()) {
            require (
                std::abs (card.pitchDegrees - 90.0F) < 0.001F,
                "all card pitches must follow the top-down camera mode"
            );
        }
        cameraState.setCameraMode (CameraMode::PlayerPerspective);
        cameraState.update (GameState::CameraTransitionDuration);
        for (const CardVisual& card: cameraState.cardVisuals()) {
            require (
                std::abs (card.pitchDegrees) < 0.001F,
                "player-perspective mode must leave all cards standing upright"
            );
        }

        std::cout << "GameState tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "GameState tests failed: " << exception.what() << '\n';
        return 1;
    }
}
