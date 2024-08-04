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
#include "Helpers.h"
using namespace std;

const double SCALINGFACTOR = 0.08;

const int ADVANCEWIDTH = 600 * SCALINGFACTOR;
const int ADVANCEHEIGHT = 1320 * SCALINGFACTOR;

const int SCREEN_WIDTH = 1400;
const int SCREEN_HEIGHT = 1320;

// Large canvas dimension constants
const int CANVAS_WIDTH = 10000;
const int CANVAS_HEIGHT = 10000;

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

int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    cout << "File size: " << fileSize << " bytes" << endl;
    file.seekg(0, ios::beg);

    if (fileSize < sizeof(uint32_t)) {
        cerr << "File is too small to read a uint32_t value." << endl;
        return 1;
    }

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);

    TTFFile ttfFile = TTFFile::parse(buffer);

    uint16_t platformID = 0;
    uint16_t encodingID = 4;

    string alph1 = 
        "The quick brown fox jumps over the lazy dog. 1234567890!@#$%^&*()_+-=[]{}|;':\",./<>?"
        "\"Hello, World!\" exclaimed the programmer, as he debugged the code at 3:00 AM."
        "Café prices increased by 5% due to inflation; isn't that surprising?"
        "Über-cool musicians play jazz-fusion on their saxophones and xylophones."
        "Visiting the Louvre in Paris, we admired Da Vinci's Mona Lisa—such an iconic piece!"
        "El Niño affects the weather patterns globally; it's a fascinating phenomenon."
        "Smörgåsbord is a type of Scandinavian meal served buffet-style, with various hot and cold dishes.";

    string alph = "i";

    vector<Glyph> glyphs;
    try {
        glyphs = ttfFile.parseGlyphs(buffer, platformID, encodingID, alph);
    } catch (const std::exception& e) {
        cerr << "Error parsing glyphs: " << e.what() << endl;
        return 1;
    }

    cout << "number of glyphs: " << glyphs.size() << endl;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        cerr << "SDL initialization failed: " << SDL_GetError() << endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
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
        cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
        return -1;
    }

    // Create renderer for window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
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
        cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << endl;
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

    for (size_t i = 0; i < glyphs.size(); ++i) {
        if (currentXOffset >= CANVAS_WIDTH - ADVANCEWIDTH) {
            currentXOffset = 0;
            currentYOffset += ADVANCEHEIGHT;
        }

        try {
            if (glyphs[i].getNumberOfContours() >= 0) {
                Glyph::drawSimpleGlyph(renderer, glyphs[i], currentXOffset, currentYOffset, SCALINGFACTOR, SCREEN_HEIGHT);
            } else {
                cout << "glyph " << i << " has " << glyphs[i].getNumberOfContours() << " contours" << endl;
                cout << "glyph " << i << " is a compound glyph" << endl;
            }
        } catch (const std::exception& e) {
            cerr << "Error drawing glyph " << i << ": " << e.what() << endl;
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
