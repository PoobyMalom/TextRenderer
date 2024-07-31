#include "MovablePoint.h"
#include <SDL.h>

MovablePoint::MovablePoint(float x, float y)
    : x(x), y(y), dragging(false), offsetX(0), offsetY(0) {}

float MovablePoint::getX() const {
    return x;
}

float MovablePoint::getY() const {
    return y;
}

void MovablePoint::setPosition(float x, float y) {
    this->x = x;
    this->y = y;
}

void MovablePoint::handleEvent(SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        // Check if the mouse is inside the point
        if (mouseX >= x && mouseX <= x + 10 && mouseY >= y && mouseY <= y + 10) {
            dragging = true;
            offsetX = mouseX - x;
            offsetY = mouseY - y;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        dragging = false;
    } else if (e.type == SDL_MOUSEMOTION) {
        if (dragging) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            x = mouseX - offsetX;
            y = mouseY - offsetY;
        }
    }
}

void MovablePoint::move() {
    // Placeholder for any movement logic
}
