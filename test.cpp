#include <iostream>
#include <SDL2/SDL.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <cstring> // For memcpy
#include <cmath>

#include "TTFFile.h"
#include "Helpers.h"
#include "GlyphRenderer.h"
#include "SDLInitializer.h"
using namespace std;

double scalingFactor = 0.5;
int thickness = 2;

const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 1000;


int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    if (!file) {
        cerr << "Failed to open TTF file.\n";
        return 1;
    }

    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    if (fileSize <= 0) {
        cerr << "Empty or unreadable TTF file.\n";
        return 1;
    }
    file.seekg(0, ios::beg);

    vector<char> buffer(static_cast<size_t>(fileSize));
    file.read(buffer.data(), fileSize);
    if (!file) {
        cerr << "Failed to read TTF file.\n";
        return 1;
    }
    file.close();

    TTFFile ttfFile = TTFFile::parse(buffer);

    string textToRender = "G";

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
    SDL_Texture* canvasTexture = intializeTexture(renderer, window, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Optional: ensure scaling matches logical pixels regardless of window resize
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Variables for scrolling (apply when copying texture to window)
    int viewportX = 0;
    int viewportY = 0;
    const int SCROLL_SPEED = 20;

    GlyphRenderer::RenderMode mode = GlyphRenderer::TRIANGULATION;
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        // ---- Handle events ----
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_LEFT:  viewportX -= SCROLL_SPEED; break;
                    case SDLK_RIGHT: viewportX += SCROLL_SPEED; break;
                    case SDLK_UP:    viewportY -= SCROLL_SPEED; break;
                    case SDLK_DOWN:  viewportY += SCROLL_SPEED; break;
                    case SDLK_a:     mode = GlyphRenderer::TRIANGULATION; break;
                    case SDLK_d:     mode = GlyphRenderer::DEBUG_LINES; break;
                    case SDLK_p:     mode = GlyphRenderer::POINTS; break;
                    case SDLK_i:     mode = GlyphRenderer::INSIDE_OFF_CURVE; break;
                    case SDLK_o:     mode = GlyphRenderer::OUTSIDE_OFF_CURVE; break;
                    default: break;
                }
            }
        }

        // ---- Draw to the offscreen canvas texture ----
        SDL_SetRenderTarget(renderer, canvasTexture);

        // Clear with white background for the canvas
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw the glyphs in black onto the canvas
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        // puts("----------------------------------\n");
        for (size_t i = 0; i < glyphs.size(); ++i) {
            try {
              GlyphRenderer::renderGlyph(renderer, glyphs[i], mode, 0.5);
            } catch (const std::exception& e) {
                cerr << "Error drawing glyph " << i << ": " << e.what() << endl;
            }
        }

        // ---- Now render the canvas texture to the window ----
        SDL_SetRenderTarget(renderer, nullptr);

        // Clear the window with a neutral color (here: light gray)
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderClear(renderer);

        // Source: whole canvas; Destination: shifted by viewport for simple panning
        SDL_Rect src{0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_Rect dst{-viewportX, -viewportY, SCREEN_WIDTH, SCREEN_HEIGHT};

        SDL_RenderCopy(renderer, canvasTexture, &src, &dst);

        // ---- Present frame ----
        SDL_RenderPresent(renderer);

        // ---- FPS (optional) ----
        frameCount++;
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime >= 1000) {
            float fps = frameCount / (elapsedTime / 1000.0f);
            cout << "FPS: " << fps << endl;
            frameCount = 0;
            startTime = SDL_GetTicks();
        }
    }

    // Clean up
    SDL_DestroyTexture(canvasTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
