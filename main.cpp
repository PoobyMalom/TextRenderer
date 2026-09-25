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
#include "Metrics.h"
#include "GlyphTable.h"
#include "TTFFile.h"
#include "Helpers.h"
#include "SDLInitializer.h"
#include "Renderer.h"
using namespace std;

int thickness = 2;

const float INITIAL_FONT_SIZE = 228.0;
const int SCREEN_WIDTH  = 1920;
const int SCREEN_HEIGHT = 1080;
const int CANVAS_WIDTH  = 10000;
const int CANVAS_HEIGHT = 10000;
const int SCROLL_SPEED  = 20;

void handleEvent(const SDL_Event& evt, FontTransform& ftrans, bool& quit, int& viewportX, int& viewportY, // NOLINT(bugprone-easily-swappable-parameters)
                 bool& canvasDirty, int& advanceHeight, int& initialYOffset, int lineHeightUnits, int ascentUnits) {
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
                ftrans.pixelsPerEm += 1;
                cout << "Font Size: " << ftrans.pixelsPerEm << "\n";
                advanceHeight  = static_cast<int>(ftrans.toPixels(lineHeightUnits));
                initialYOffset = static_cast<int>(ftrans.toPixels(ascentUnits));
                canvasDirty = true;
                break;
            case SDLK_MINUS:
                ftrans.pixelsPerEm -= 1;
                ftrans.pixelsPerEm = std::max(ftrans.pixelsPerEm, 1.0f);
                cout << "Font Size: " << ftrans.pixelsPerEm << "\n";
                advanceHeight  = static_cast<int>(ftrans.toPixels(lineHeightUnits));
                initialYOffset = static_cast<int>(ftrans.toPixels(ascentUnits));
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

void renderFrame(SDL_Renderer* renderer, SDL_Texture* canvasTexture, FontTransform& ftrans,
                 const vector<Glyph>& glyphs, int viewportX, int viewportY,
                 bool& canvasDirty, int initialYOffset, int advanceHeight) {
    if (canvasDirty) {
        SDL_SetRenderTarget(renderer, canvasTexture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        int currentXOffset = 0;
        int currentYOffset = initialYOffset;

        for (size_t i = 0; i < glyphs.size(); ++i) {
            if (currentXOffset + static_cast<int>(ftrans.toPixels(glyphs[i].getAdvanceWidth())) >= CANVAS_WIDTH) {
                currentXOffset = 0;
                currentYOffset += advanceHeight;
            }
            try {
                drawSimpleGlyph(renderer, glyphs[i], ftrans, currentXOffset, currentYOffset, thickness);
            } catch (const std::exception& err) {
                cerr << "Error drawing glyph " << i << ": " << err.what() << '\n';
            }
            currentXOffset += ftrans.toPixels(glyphs[i].getAdvanceWidth());
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./main <font.ttf>\n";
        return 1;
    }
    string fileName = argv[1];
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
    FontTransform ftrans = {INITIAL_FONT_SIZE, ttfFile.getHeadTable().unitsPerEm};

    int ascentUnits     = ttfFile.getMetricsTable().getHheaTable().ascent;
    int lineHeightUnits = ascentUnits - ttfFile.getMetricsTable().getHheaTable().descent + ttfFile.getMetricsTable().getHheaTable().lineGap;
    int advanceHeight   = static_cast<int>(ftrans.toPixels(lineHeightUnits));
    int initialYOffset  = static_cast<int>(ftrans.toPixels(ascentUnits));


    vector<Glyph> glyphs;
    try {
        const string textToRender = "The unanimous Declaration of the thirteen united States of America and the lazy dog that jumped over that fox or something + - ,.<> hello (*& @#^ !@#%%# {}|}{|})";
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

    bool canvasDirty  = true;
    bool quit         = false;
    SDL_Event evt;

    while (!quit) {
        while (SDL_PollEvent(&evt) != 0) {
            handleEvent(evt, ftrans, quit, viewportX, viewportY, canvasDirty, advanceHeight, initialYOffset, lineHeightUnits, ascentUnits);
        }

        renderFrame(renderer, canvasTexture, ftrans, glyphs, viewportX, viewportY,
                    canvasDirty, initialYOffset, advanceHeight);

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
