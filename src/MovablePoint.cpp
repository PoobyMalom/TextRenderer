#include "MovablePoint.h"
#include <SDL2/SDL.h>

MovablePoint::MovablePoint(float x, float y) // NOLINT(bugprone-easily-swappable-parameters, readability-identifier-length)
    : x(x), y(y), dragging(false), offsetX(0), offsetY(0) {}

float MovablePoint::getX() const {
    return x;
}

float MovablePoint::getY() const {
    return y;
}

void MovablePoint::setPosition(float x, float y) { // NOLINT(bugprone-easily-swappable-parameters, readability-identifier-length)
    this->x = x;
    this->y = y;
}

void MovablePoint::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX;
        int mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        // Check if the mouse is inside the point
        if (mouseX >= int(x) && mouseX <= (int)x + 10 && mouseY >= (int)y && mouseY <= (int)y + 10) {
            dragging = true;
            offsetX = mouseX - (int)x;
            offsetY = mouseY - (int)y;
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        dragging = false;
    } else if (event.type == SDL_MOUSEMOTION) {
        if (dragging) {
            int mouseX;
            int mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            x = (float)(mouseX - offsetX);
            y = (float)(mouseY - offsetY);
        }
    }
}

void MovablePoint::move() {
    // Placeholder for any movement logic
}
