#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <SDL3/SDL.h>

#include "TTFFile.h"
#include "Metrics.h"
#include "GlyphTable.h"
#include "SDLInitializer.h"
#include "Renderer.h"

using namespace std;

namespace {

const float INITIAL_FONT_SIZE = 2000.0;
const int SCREEN_WIDTH   = 1920;
const int SCREEN_HEIGHT  = 1080;
const int CANVAS_WIDTH   = 10000;
const int CANVAS_HEIGHT  = 10000;
const int SCROLL_SPEED   = 20;
const int GLYPH_THICKNESS = 2;
const int ON_CURVE_DOT_RADIUS = 2;

// Each mode is bound to a number key (1 = first mode, 2 = second, ...).
enum class Mode {
    Outline = 0,   // What main.cpp draws: the Bezier outlines of every glyph
    OnCurvePoints, // Only the on-curve points of every glyph
    OffCurvePoints,
    AllPoints,
    Dashed,
    StraightLines,
    Combined,
    Triangulated,
    CombinedTriangulated,
    Count
};

constexpr int kNumModes = static_cast<int>(Mode::Count);

const char* modeName(Mode mode) {
    switch (mode) {
        case Mode::Outline:
            return "Outline";
        case Mode::OnCurvePoints:
            return "On-curve points";
        default:
            return "";
    }
}

struct ViewState {
    FontTransform ftrans;
    Mode mode = Mode::Outline;
    int viewportX = 0;
    int viewportY = 0;
    int advanceHeight = 0;
    int initialYOffset = 0;
    int lineHeightUnits = 0;
    int ascentUnits = 0;
    bool canvasDirty = true;
    bool quit = false;
};

void announceMode(SDL_Window* window, Mode mode) {
    int modeNumber = static_cast<int>(mode) + 1;
    string title = "Triangulation -- Mode " + to_string(modeNumber) + "/" + to_string(kNumModes) + ": " + modeName(mode);
    SDL_SetWindowTitle(window, title.c_str());
    cout << "Mode " << modeNumber << "/" << kNumModes << " -- " << modeName(mode) << '\n';
}

void recomputeLayout(ViewState& view) {
    view.advanceHeight  = static_cast<int>(view.ftrans.toPixels(view.lineHeightUnits));
    view.initialYOffset = static_cast<int>(view.ftrans.toPixels(view.ascentUnits));
}

void clampViewport(ViewState& view) {
    view.viewportX = std::clamp(view.viewportX, 0, CANVAS_WIDTH - SCREEN_WIDTH);
    view.viewportY = std::clamp(view.viewportY, 0, CANVAS_HEIGHT - SCREEN_HEIGHT);
}

void handleEvent(const SDL_Event& evt, SDL_Window* window, ViewState& view) {
    if (evt.type == SDL_EVENT_QUIT) {
        view.quit = true;
        return;
    }
    if (evt.type == SDL_EVENT_MOUSE_WHEEL) {
        view.viewportX += static_cast<int>(evt.wheel.x * SCROLL_SPEED);
        view.viewportY -= static_cast<int>(evt.wheel.y * SCROLL_SPEED);
        clampViewport(view);
        return;
    }
    if (evt.type != SDL_EVENT_KEY_DOWN) {
        return;
    }

    SDL_Keycode key = evt.key.key;

    if (key == SDLK_ESCAPE) {
        view.quit = true;
        return;
    }

    if (key >= SDLK_1 && key < SDLK_1 + kNumModes) {
        auto requested = static_cast<Mode>(key - SDLK_1);
        if (requested != view.mode) {
            view.mode = requested;
            view.canvasDirty = true;
            announceMode(window, view.mode);
        }
        return;
    }

    switch (key) {
        case SDLK_UP:
            view.viewportY -= SCROLL_SPEED;
            break;
        case SDLK_DOWN:
            view.viewportY += SCROLL_SPEED;
            break;
        case SDLK_LEFT:
            view.viewportX -= SCROLL_SPEED;
            break;
        case SDLK_RIGHT:
            view.viewportX += SCROLL_SPEED;
            break;
        case SDLK_PLUS:
        case SDLK_EQUALS:
            view.ftrans.pixelsPerEm += 1;
            cout << "Font Size: " << view.ftrans.pixelsPerEm << "\n";
            recomputeLayout(view);
            view.canvasDirty = true;
            break;
        case SDLK_MINUS:
            view.ftrans.pixelsPerEm = std::max(view.ftrans.pixelsPerEm - 1, 1.0F);
            cout << "Font Size: " << view.ftrans.pixelsPerEm << "\n";
            recomputeLayout(view);
            view.canvasDirty = true;
            break;
        default:
            break;
    }
    clampViewport(view);
}

// Mode 2: a filled square at every on-curve point of the glyph.
void drawOnCurvePoints(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans, int xOffset, int yOffset) {
    const vector<int16_t>& xs = glyph.getXCoordinates();
    const vector<int16_t>& ys = glyph.getYCoordinates();
    const vector<uint8_t>& flags = glyph.getFlags();

    SDL_SetRenderDrawColor(renderer, 0, 160, 0, 255);
    for (size_t i = 0; i < xs.size(); ++i) {
        if ((flags[i] & 1) == 0) {
            continue;
        }
        float screenX = ftrans.toPixels(xs[i]) + static_cast<float>(xOffset);
        auto screenY = static_cast<float>(ftrans.toScreenY(ys[i], yOffset));
        // cout << "Coord: (" << screenX << ", " << screenY << ")" << "\n";
        SDL_FRect rect = {screenX - ON_CURVE_DOT_RADIUS, screenY - ON_CURVE_DOT_RADIUS,
                          ON_CURVE_DOT_RADIUS * 2.0F, ON_CURVE_DOT_RADIUS * 2.0F};
        SDL_RenderFillRect(renderer, &rect);
    }
}

// Mode 3: a filled square at every off-curve point of the glyph.
void drawOffCurvePoints(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans, int xOffset, int yOffset) {
    const vector<int16_t>& xs = glyph.getXCoordinates();
    const vector<int16_t>& ys = glyph.getYCoordinates();
    const vector<uint8_t>& flags = glyph.getFlags();

    SDL_SetRenderDrawColor(renderer, 160, 0, 0, 255);
    for (size_t i = 0; i < xs.size(); ++i) {
        if ((flags[i] & 1) == 1) {
            continue;
        }
        float screenX = ftrans.toPixels(xs[i]) + static_cast<float>(xOffset);
        auto screenY = static_cast<float>(ftrans.toScreenY(ys[i], yOffset));
        SDL_FRect rect = {screenX - ON_CURVE_DOT_RADIUS, screenY - ON_CURVE_DOT_RADIUS,
                          ON_CURVE_DOT_RADIUS * 2.0F, ON_CURVE_DOT_RADIUS * 2.0F};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void drawGlyph(SDL_Renderer* renderer, Mode mode, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset) {
    switch (mode) {
        case Mode::Outline:
            drawSimpleGlyph(renderer, glyph, ftrans, xOffset, yOffset, GLYPH_THICKNESS);
            break;
        case Mode::OnCurvePoints:
            drawOnCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        case Mode::OffCurvePoints:
            drawOffCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        case Mode::AllPoints:
            drawOnCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            drawOffCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        case Mode::Dashed:
            drawSimpleGlyphDashed(renderer, glyph, ftrans, xOffset, yOffset, GLYPH_THICKNESS);
            break;
        case Mode::StraightLines:
            drawSimpleGlyphLines(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        case Mode::Combined:
            drawSimpleGlyphDashed(renderer, glyph, ftrans, xOffset, yOffset, GLYPH_THICKNESS);
            drawSimpleGlyphLines(renderer, glyph, ftrans, xOffset, yOffset);
            drawOnCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            drawOffCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        case Mode::Triangulated:
            drawTriangulatedGlyph(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        case Mode::CombinedTriangulated:
            drawTriangulatedGlyph(renderer, glyph, ftrans, xOffset, yOffset);
            drawSimpleGlyphDashed(renderer, glyph, ftrans, xOffset, yOffset, GLYPH_THICKNESS);
            drawSimpleGlyphLines(renderer, glyph, ftrans, xOffset, yOffset);
            drawOnCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            drawOffCurvePoints(renderer, glyph, ftrans, xOffset, yOffset);
            break;
        default:
            break;
    }
}

void renderFrame(SDL_Renderer* renderer, SDL_Texture* canvasTexture, ViewState& view, const vector<Glyph>& glyphs) {
    if (view.canvasDirty) {
        SDL_SetRenderTarget(renderer, canvasTexture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        int currentXOffset = 0;
        int currentYOffset = view.initialYOffset;

        for (size_t i = 0; i < glyphs.size(); ++i) {
            if (currentXOffset + static_cast<int>(view.ftrans.toPixels(glyphs[i].getAdvanceWidth())) >= CANVAS_WIDTH) {
                currentXOffset = 0;
                currentYOffset += view.advanceHeight;
            }
            try {
                drawGlyph(renderer, view.mode, glyphs[i], view.ftrans, currentXOffset, currentYOffset);
            } catch (const std::exception& err) {
                cerr << "Error drawing glyph " << i << ": " << err.what() << '\n';
            }
            currentXOffset += static_cast<int>(view.ftrans.toPixels(glyphs[i].getAdvanceWidth()));
        }

        SDL_SetRenderTarget(renderer, nullptr);
        view.canvasDirty = false;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_FRect srcRect = {static_cast<float>(view.viewportX), static_cast<float>(view.viewportY),
                         static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)};
    SDL_FRect dstRect = {0.0F, 0.0F, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)};
    SDL_RenderTexture(renderer, canvasTexture, &srcRect, &dstRect);
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./triangulation <font.ttf>\n";
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

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    TTFFile ttfFile = TTFFile::parse(buffer);

    ViewState view;
    view.ftrans = {INITIAL_FONT_SIZE, ttfFile.getHeadTable().unitsPerEm};
    const auto& hhea = ttfFile.getMetricsTable().getHheaTable();
    view.ascentUnits     = hhea.ascent;
    view.lineHeightUnits = view.ascentUnits - hhea.descent + hhea.lineGap;
    recomputeLayout(view);

    vector<Glyph> glyphs;
    try {
        const string textToRender = "eat";
        glyphs = ttfFile.parseGlyphs(buffer, textToRender);
    } catch (const std::exception& err) {
        cerr << "Error parsing glyphs: " << err.what() << '\n';
        return 1;
    }

    SDL_Window*   window        = initializeWindow("Triangulation", SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Renderer* renderer      = initializeRenderer(window);
    SDL_Texture*  canvasTexture = initializeTexture(renderer, window, CANVAS_WIDTH, CANVAS_HEIGHT);

    cout << "Controls: 1 = outline | 2 = on-curve points | arrows/wheel scroll | +/- zoom | Esc quit\n\n";
    announceMode(window, view.mode);

    SDL_Event evt;
    while (!view.quit) {
        while (SDL_PollEvent(&evt)) {
            handleEvent(evt, window, view);
        }

        renderFrame(renderer, canvasTexture, view, glyphs);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(canvasTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
