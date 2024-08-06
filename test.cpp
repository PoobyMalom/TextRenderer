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
#include "SDLInitializer.h"
using namespace std;

double scalingFactor = 0.1;
int thickness = 2;

const int SCREEN_WIDTH = 1400;
const int SCREEN_HEIGHT = 1320;

// Large canvas dimension constants
const int CANVAS_WIDTH = 10000;
const int CANVAS_HEIGHT = 10000;

int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);

    if (fileSize < sizeof(uint32_t)) {
        cerr << "File is too small to read a uint32_t value." << endl;
        return 1;
    }

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);

    TTFFile ttfFile = TTFFile::parse(buffer);

    string textToRender =  "Café prices increased by 5% due to inflation; isn't that surprising?"; 

    vector<Glyph> glyphs;
    try {
        glyphs = ttfFile.parseGlyphs(buffer, textToRender);
    } catch (const std::exception& e) {
        cerr << "Error parsing glyphs: " << e.what() << endl;
        return 1;
    }

    Uint32 startTime = SDL_GetTicks();
    int frameCount = 0;

    SDL_Window* window = initializeWindow("text_renderer", SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Renderer* renderer = initializeRenderer(window);
    SDL_Texture* canvasTexture = intializeTexture(renderer, window, CANVAS_WIDTH, CANVAS_HEIGHT);

    // Variables for scrolling
    int viewportX = 0;
    int viewportY = 0;
    const int SCROLL_SPEED = 20;

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        const int ADVANCEWIDTH = 600 * scalingFactor;
        const int ADVANCEHEIGHT = 1320 * scalingFactor;
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
                    case SDLK_PLUS:
                    case SDLK_EQUALS:  // For the '=' key, typically on the same key as '+'
                        scalingFactor += 0.01;
                        break;
                    case SDLK_MINUS:
                        scalingFactor -= 0.01;
                        if (scalingFactor < 0.01) scalingFactor = 0.01;  // Prevent negative or zero scale
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

        // Clear the canvas texture
        SDL_SetRenderTarget(renderer, canvasTexture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Clear with white
        SDL_RenderClear(renderer);

        // Draw the glyphs
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set draw color for glyphs

        int currentXOffset = 0;
        int currentYOffset = 100;

        for (size_t i = 0; i < glyphs.size(); ++i) {
            if (currentXOffset >= CANVAS_WIDTH - ADVANCEWIDTH) {
                currentXOffset = 0;
                currentYOffset += ADVANCEHEIGHT;
            }

            try {
                Glyph::drawSimpleGlyph(renderer, glyphs[i], currentXOffset, currentYOffset, scalingFactor, SCREEN_HEIGHT, thickness);
            } catch (const std::exception& e) {
                cerr << "Error drawing glyph " << i << ": " << e.what() << endl;
            }

            currentXOffset += ADVANCEWIDTH;
        }

        // Reset the render target to the default window
        SDL_SetRenderTarget(renderer, nullptr);

        // Clear the renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Define the source rectangle (viewport) from the canvas texture
        SDL_Rect srcRect = {viewportX, viewportY, SCREEN_WIDTH, SCREEN_HEIGHT};

        // Define the destination rectangle on the window
        SDL_Rect dstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

        // Render the visible part of the canvas texture to the window
        SDL_RenderCopy(renderer, canvasTexture, &srcRect, &dstRect);

        frameCount++;
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime >= 1000) {
            float fps = frameCount / (elapsedTime / 1000.0f);
            //cout << "FPS: " << fps << endl;
            //uncomment for fps debug on console
            frameCount = 0;
            startTime = SDL_GetTicks();
        }

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
