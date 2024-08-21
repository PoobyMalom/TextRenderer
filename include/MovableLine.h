#ifndef MOVABLELINE_H
#define MOVABLELINE_H

#include <SDL.h>

class MovableLine {
public:
    // Updated constructor to accept xOffset and yOffset
    MovableLine(SDL_Point start, SDL_Point end, int xOffset, int yOffset);

    void handleEvent(SDL_Event& e);
    void draw(SDL_Renderer* renderer) const;

    int getX1() const { return start.x + xOffset; }
    int getY1() const { return start.y + yOffset; }
    int getX2() const { return end.x + xOffset; }
    int getY2() const { return end.y + yOffset; }

private:
    SDL_Point start;
    SDL_Point end;
    int xOffset;  // Horizontal offset for positioning
    int yOffset;  // Vertical offset for positioning
    bool dragging = false;
    bool draggingStart = false;
    SDL_Point dragOffset;

    bool isMouseOnLine(int mouseX, int mouseY) const;
    bool isMouseNearStartPoint(int mouseX, int mouseY) const;
};

#endif // MOVABLELINE_H
