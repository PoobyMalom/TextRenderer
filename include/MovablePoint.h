#ifndef MOVABLEPOINT_H
#define MOVABLEPOINT_H

#include <SDL.h>

class MovablePoint {
public:
    MovablePoint(float x = 0.0f, float y = 0.0f);

    float getX() const;
    float getY() const;
    void setPosition(float x, float y);

    void handleEvent(SDL_Event& e);
    void move();

private:
    float x, y;
    bool dragging;
    int offsetX, offsetY;
};

#endif // MOVABLEPOINT_H
