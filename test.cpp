#include <iostream>
#include <SDL.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <cstring> // For memcpy

#include "TTFTable.h"
#include "TTFHeader.h"
#include "HeadTable.h"
#include "MaxpTable.h"
#include "CmapTable.h"
#include "LocaTable.h"
#include "GlyphTable.h"
#include "TTFFile.h"
using namespace std;

const double SCALINGFACTOR = 1;

const int ADVANCEWIDTH = 600 * SCALINGFACTOR;
const int ADVANCEHEIGHT = 1320 * SCALINGFACTOR;

const int SCREEN_WIDTH = 1400;
const int SCREEN_HEIGHT = 1320;

// Large canvas dimension constants
const int CANVAS_WIDTH = 10000;
const int CANVAS_HEIGHT = 10000;

//thanks chatGPT
void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w*w + h*h <= radius*radius) {
                SDL_RenderDrawPoint(renderer, centerX + w, centerY + h);
            }
        }
    }
}

uint32_t convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint16_t convertEndian16(uint16_t value) {
    return ((value >> 8) & 0x00FF) |
           ((value << 8)  & 0xFF00);
}

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2) {
    // Check if the control point is collinear with the start and end points
    auto isCollinear = [](const SDL_Point& p0, const SDL_Point& p1, const SDL_Point& p2) {
        return std::abs((p2.y - p0.y) * (p1.x - p0.x) - (p1.y - p0.y) * (p2.x - p0.x)) < 1e-5;
    };

    if (isCollinear(point1, controlPoint, point2)) {
        // Draw a straight line if the points are collinear
        SDL_RenderDrawLine(renderer, point1.x, point1.y, point2.x, point2.y);
        return;
    }

    int numPoints = 100; // Increase the number of points for a smoother curve
    float t = 0.0;
    float step = 1.0f / numPoints;
    std::vector<SDL_Point> points;

    for (int i = 0; i <= numPoints; i++) {
        float x = ((1 - t) * (1 - t)) * point1.x + 2 * (1 - t) * t * controlPoint.x + (t * t) * point2.x;
        float y = ((1 - t) * (1 - t)) * point1.y + 2 * (1 - t) * t * controlPoint.y + (t * t) * point2.y;
        SDL_Point point = { static_cast<int>(x), static_cast<int>(y) };
        points.push_back(point);
        t += step;
    }

    // Line simplification: remove consecutive points that are too close to each other
    std::vector<SDL_Point> simplifiedPoints;
    const int distanceThreshold = 1; // Distance threshold for simplification
    simplifiedPoints.push_back(points.front());

    for (size_t i = 1; i < points.size(); ++i) {
        if (std::abs(points[i].x - simplifiedPoints.back().x) > distanceThreshold ||
            std::abs(points[i].y - simplifiedPoints.back().y) > distanceThreshold) {
            simplifiedPoints.push_back(points[i]);
        }
    }

    // Draw the simplified points
    for (size_t i = 1; i < simplifiedPoints.size(); ++i) {
        SDL_RenderDrawLine(renderer, simplifiedPoints[i - 1].x, simplifiedPoints[i - 1].y, simplifiedPoints[i].x, simplifiedPoints[i].y);
    }
}


int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    cout << "File size: " << fileSize << " bytes" << endl;
    file.seekg(0, ios::beg);

    if (sizeof(file) < sizeof(uint32_t)) {
        cerr << "File is too small to read a uint32_t value." << endl;
        return 1;
    }

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);

    TTFFile ttfFile = TTFFile::parse(buffer);

    uint16_t platformID = 0;
    uint16_t encodingID = 4;

    string alph = "B";
    cout << static_cast<int>(alph[0]) << endl;

    vector<Glyph> glyphs = ttfFile.parseGlyphs(buffer, platformID, encodingID, alph);


//-------------------------------------------------------------------------

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        cerr << "SDL initialization failed: " << SDL_GetError() << endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("Scrollable SDL Window",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create renderer for window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Create a texture to represent the large canvas
    SDL_Texture* canvasTexture = SDL_CreateTexture(renderer,
                                                   SDL_PIXELFORMAT_RGBA8888,
                                                   SDL_TEXTUREACCESS_TARGET,
                                                   CANVAS_WIDTH,
                                                   CANVAS_HEIGHT);
    if (canvasTexture == nullptr) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Set the canvas texture as the target for rendering
    SDL_SetRenderTarget(renderer, canvasTexture);

    // Clear the canvas texture with a color (for example, white)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw something on the canvas (example: a red rectangle)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    int currentXOffset = 0;
    int currentYOffset = 0;
    int radius = 4;

    for (int i = 0; i < alph.size(); ++i) {
        if (currentXOffset >= CANVAS_WIDTH - ADVANCEWIDTH) {
            currentXOffset = 0;
            currentYOffset += ADVANCEHEIGHT;
        }
        Glyph glyph = glyphs[i];
        glyph.addPointsBetween();
        vector<uint16_t> endpoints = glyph.getEndPtsOfContours();

        int currentContour = 0;
        int contourStartIndex = 0;

        cout << "glyph: " << alph[i] << endl;
        for (uint16_t endpoint : endpoints) {
            cout << "endpoints: " << endpoint << endl;
        }

        vector<int16_t> xCoordinates = glyph.getXCoordinates();
        vector<int16_t> yCoordinates = glyph.getYCoordinates();

        vector<uint8_t> flags = glyph.getFlags();
        for (int j = 0; j < xCoordinates.size(); ++j) {
            uint8_t flag = flags[j];
            if (j > endpoints[currentContour]) {
                contourStartIndex = endpoints[currentContour] + 1;
                ++currentContour;
            }
            if (flag & 1) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                drawCircle(renderer, xCoordinates[j], SCREEN_HEIGHT - yCoordinates[j], radius);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                drawCircle(renderer, xCoordinates[j], SCREEN_HEIGHT - yCoordinates[j], radius);
            }
            if (j == endpoints[currentContour]) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                drawCircle(renderer, xCoordinates[j], SCREEN_HEIGHT - yCoordinates[j], 2);
            }
            cout << "point: " << j << " in contour: " << currentContour << " | x: " << xCoordinates[j] << ", y: " << yCoordinates[j] << endl;


            //if (flag & 1) {
            //    SDL_Point point1 = 
            //}

            if (flag & 1) { // If the current point is an on-curve point
                SDL_Point point1 = { static_cast<int>(xCoordinates[j] * SCALINGFACTOR + currentXOffset), static_cast<int>(SCREEN_HEIGHT - yCoordinates[j] * SCALINGFACTOR + currentYOffset)};
                SDL_Point controlPoint;
                SDL_Point point2;
                if (j !=  endpoints[currentContour] - 1) {
                    cout << j << endl;
                    controlPoint.x = static_cast<int>((xCoordinates[j + 1] + currentXOffset) * SCALINGFACTOR);
                    controlPoint.y = static_cast<int>(SCREEN_HEIGHT - ((yCoordinates[j + 1] + currentYOffset) * SCALINGFACTOR));
                    point2.x = static_cast<int>((xCoordinates[j + 2] + currentXOffset) * SCALINGFACTOR);
                    point2.y = static_cast<int>(SCREEN_HEIGHT - ((yCoordinates[j + 2] + currentYOffset) * SCALINGFACTOR));
                } else {
                    controlPoint.x = static_cast<int>((xCoordinates[j + 1] + currentXOffset) * SCALINGFACTOR);
                    controlPoint.y = static_cast<int>(SCREEN_HEIGHT - ((yCoordinates[j + 1] + currentYOffset) * SCALINGFACTOR));
                    point2.x = static_cast<int>((xCoordinates[contourStartIndex] + currentXOffset) * SCALINGFACTOR);
                    point2.y = static_cast<int>(SCREEN_HEIGHT - ((yCoordinates[contourStartIndex] + currentYOffset) * SCALINGFACTOR));
                }
                cout << "contour " << currentContour << " start index: " << contourStartIndex << endl;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

                DrawBezier(renderer, point1, controlPoint, point2);
            }
        }
        currentXOffset += ADVANCEWIDTH;
    }
    // Reset the render target to the default window
    SDL_SetRenderTarget(renderer, nullptr);

    // Variables for scrolling
    int viewportX = 0;
    int viewportY = 0;
    const int SCROLL_SPEED = 10;

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        // Handle events on the queue
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                // Adjust the viewport position based on arrow key input
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        viewportY -= SCROLL_SPEED;
                        if (viewportY < 0) viewportY = 0;
                        break;
                    case SDLK_DOWN:
                        viewportY += SCROLL_SPEED;
                        if (viewportY > CANVAS_HEIGHT - SCREEN_HEIGHT) viewportY = CANVAS_HEIGHT - SCREEN_HEIGHT;
                        break;
                    case SDLK_LEFT:
                        viewportX -= SCROLL_SPEED;
                        if (viewportX < 0) viewportX = 0;
                        break;
                    case SDLK_RIGHT:
                        viewportX += SCROLL_SPEED;
                        if (viewportX > CANVAS_WIDTH - SCREEN_WIDTH) viewportX = CANVAS_WIDTH - SCREEN_WIDTH;
                        break;
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                // Adjust the viewport position based on mouse wheel input
                viewportX += e.wheel.x * SCROLL_SPEED;
                viewportY -= e.wheel.y * SCROLL_SPEED; // Typically, wheel.y is positive when scrolling up

                if (viewportX < 0) viewportX = 0;
                if (viewportX > CANVAS_WIDTH - SCREEN_WIDTH) viewportX = CANVAS_WIDTH - SCREEN_WIDTH;
                if (viewportY < 0) viewportY = 0;
                if (viewportY > CANVAS_HEIGHT - SCREEN_HEIGHT) viewportY = CANVAS_HEIGHT - SCREEN_HEIGHT;
            }
        }

        // Clear the renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Define the source rectangle (viewport) from the canvas texture
        SDL_Rect srcRect = {viewportX, viewportY, SCREEN_WIDTH, SCREEN_HEIGHT};

        // Define the destination rectangle on the window
        SDL_Rect dstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

        // Render the visible part of the canvas texture to the window
        SDL_RenderCopy(renderer, canvasTexture, &srcRect, &dstRect);

        // Update the screen
        SDL_RenderPresent(renderer);
    }


    // Clean up
    SDL_DestroyTexture(canvasTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    file.close();

    return 0;
}