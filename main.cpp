#include <iostream>
#include <algorithm>
#include <SDL2/SDL.h>
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
#include "Renderer.h"
using namespace std;

double scalingFactor = 0.1;
int thickness = 2;

const int SCREEN_WIDTH  = 1400;
const int SCREEN_HEIGHT = 1320;
const int CANVAS_WIDTH  = 10000;
const int CANVAS_HEIGHT = 10000;
const int SCROLL_SPEED  = 20;

// TODO: replace with per-glyph hmtx.getAdvanceWidth() / hhea.ascender once hmtx is parsed
constexpr int NOMINAL_ADVANCE_UNITS     = 600;
constexpr int NOMINAL_LINE_HEIGHT_UNITS = 1320;

void handleEvent(const SDL_Event& evt, bool& quit, int& viewportX, int& viewportY, // NOLINT(bugprone-easily-swappable-parameters)
                 bool& canvasDirty, int& advanceWidth, int& advanceHeight) {
    if (evt.type == SDL_QUIT) {
        quit = true;
    } else if (evt.type == SDL_KEYDOWN) {
        switch (evt.key.keysym.sym) {
            case SDLK_UP:
                viewportY -= SCROLL_SPEED;
                viewportY = std::max(viewportY, 0);
                break;
            case SDLK_DOWN:
                viewportY += SCROLL_SPEED;
                viewportY = std::min(viewportY, CANVAS_HEIGHT - SCREEN_HEIGHT);
                break;
            case SDLK_LEFT:
                viewportX -= SCROLL_SPEED;
                viewportX = std::max(viewportX, 0);
                break;
            case SDLK_RIGHT:
                viewportX += SCROLL_SPEED;
                viewportX = std::min(viewportX, CANVAS_WIDTH - SCREEN_WIDTH);
                break;
            case SDLK_PLUS:
            case SDLK_EQUALS:
                scalingFactor += 0.01;
                advanceWidth  = static_cast<int>(NOMINAL_ADVANCE_UNITS     * scalingFactor);
                advanceHeight = static_cast<int>(NOMINAL_LINE_HEIGHT_UNITS  * scalingFactor);
                canvasDirty = true;
                break;
            case SDLK_MINUS:
                scalingFactor -= 0.01;
                scalingFactor = std::max(scalingFactor, 0.01);
                advanceWidth  = static_cast<int>(NOMINAL_ADVANCE_UNITS     * scalingFactor);
                advanceHeight = static_cast<int>(NOMINAL_LINE_HEIGHT_UNITS  * scalingFactor);
                canvasDirty = true;
                break;
            default:
                break;
        }
    } else if (evt.type == SDL_MOUSEWHEEL) {
        viewportX += evt.wheel.x * SCROLL_SPEED;
        viewportY -= evt.wheel.y * SCROLL_SPEED;
        viewportX = std::max(viewportX, 0);
        viewportX = std::min(viewportX, CANVAS_WIDTH - SCREEN_WIDTH);
        viewportY = std::max(viewportY, 0);
        viewportY = std::min(viewportY, CANVAS_HEIGHT - SCREEN_HEIGHT);
    }
}

void renderFrame(SDL_Renderer* renderer, SDL_Texture* canvasTexture,
                 const vector<Glyph>& glyphs, int viewportX, int viewportY,
                 bool& canvasDirty, int advanceWidth, int advanceHeight) {
    if (canvasDirty) {
        SDL_SetRenderTarget(renderer, canvasTexture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        int currentXOffset = 0;
        int currentYOffset = 100;

        for (size_t i = 0; i < glyphs.size(); ++i) {
            if (currentXOffset >= CANVAS_WIDTH - advanceWidth) {
                currentXOffset = 0;
                currentYOffset += advanceHeight;
            }
            try {
                drawSimpleGlyph(renderer, glyphs[i], currentXOffset, currentYOffset,
                                       scalingFactor, SCREEN_HEIGHT, thickness);
            } catch (const std::exception& err) {
                cerr << "Error drawing glyph " << i << ": " << err.what() << '\n';
            }
            currentXOffset += advanceWidth;
        }

        SDL_SetRenderTarget(renderer, nullptr);
        canvasDirty = false;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Rect srcRect = {viewportX, viewportY, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_Rect dstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, canvasTexture, &srcRect, &dstRect);
}

int main() {
    string fileName = "src/fonts/JetBrainsMono-Bold.ttf";
    ifstream file(fileName, ios::binary);

    if (!file.is_open()) {
        cerr << "File: " << fileName << " Cannot be opened" << '\n';
        return 1;
    }

    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);

    if (fileSize < static_cast<std::streamoff>(sizeof(uint32_t))) {
        cerr << "File is too small to read a uint32_t value." << '\n';
        return 1;
    }

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    TTFFile ttfFile = TTFFile::parse(buffer);

    vector<Glyph> glyphs;
    try {
        const string textToRender = "The unanimous Declaration of the thirteen united States of America";
        glyphs = ttfFile.parseGlyphs(buffer, textToRender);
    } catch (const std::exception& err) {
        cerr << "Error parsing glyphs: " << err.what() << '\n';
        return 1;
    }

    Uint32 startTime = SDL_GetTicks();
    int frameCount = 0;

    SDL_Window*   window        = initializeWindow("text_renderer", SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Renderer* renderer      = initializeRenderer(window);
    SDL_Texture*  canvasTexture = initializeTexture(renderer, window, CANVAS_WIDTH, CANVAS_HEIGHT);

    int viewportX = 0;
    int viewportY = 0;

    int advanceWidth  = static_cast<int>(NOMINAL_ADVANCE_UNITS     * scalingFactor);
    int advanceHeight = static_cast<int>(NOMINAL_LINE_HEIGHT_UNITS  * scalingFactor);
    bool canvasDirty  = true;
    bool quit         = false;
    SDL_Event evt;

    while (!quit) {
        while (SDL_PollEvent(&evt) != 0) {
            handleEvent(evt, quit, viewportX, viewportY, canvasDirty, advanceWidth, advanceHeight);
        }

        renderFrame(renderer, canvasTexture, glyphs, viewportX, viewportY,
                    canvasDirty, advanceWidth, advanceHeight);

        frameCount++;
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime >= 1000) {
            float fps = static_cast<float>(frameCount) / (static_cast<float>(elapsedTime) / 1000.0F);
            cout << "FPS: " << fps << '\n';
            frameCount = 0;
            startTime = SDL_GetTicks();
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(canvasTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
