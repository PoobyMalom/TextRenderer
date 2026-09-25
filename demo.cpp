#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <SDL2/SDL.h>

#include "TTFFile.h"
#include "Metrics.h"
#include "GlyphTable.h"
#include "Renderer.h"
#include "SDLInitializer.h"
#include "DemoPhases.h"

using namespace std;

namespace {

const int SCREEN_WIDTH  = 1920;
const int SCREEN_HEIGHT = 1080;
const int GLYPH_THICKNESS = 2;

const string DEFAULT_PRIMARY_FONT = "src/fonts/JetBrainsMono-Regular.ttf";
const string MONO_FONT_PATH       = "src/fonts/JetBrainsMono-Regular.ttf";
const string PROP_FONT_PATH       = "src/fonts/papyrus.ttf";
const string TALL_FONT_PATH       = "src/fonts/MonsieurLaDoulaise-Regular.ttf";

const string SPACING_SAMPLE_TEXT   = "iiiMMMiii";
const string PARAGRAPH_SAMPLE_LINE = "Jibby quartz pond fig";
const int PARAGRAPH_LINE_COUNT     = 3;

struct LoadedFont {
    vector<char> buffer;
    TTFFile ttf;
};

LoadedFont loadFont(const string& path) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("Cannot open font: " + path);
    }
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    TTFFile ttf = TTFFile::parse(buffer);
    return {std::move(buffer), std::move(ttf)};
}

// Converts font units to pixels using float math throughout (unlike
// FontTransform::toPixels, which takes an int16_t) -- used only for the
// aggregate "naive constant advance" values, which are averages and don't
// fit the per-point int16_t path.
float unitsToPixels(float units, const FontTransform& ftrans) {
    return units * ftrans.pixelsPerEm / static_cast<float>(ftrans.unitsPerEm);
}

float averageAdvanceUnits(const vector<Glyph>& glyphs) {
    float sum = 0.0F;
    for (const Glyph& glyph : glyphs) {
        sum += static_cast<float>(glyph.getAdvanceWidth());
    }
    return sum / static_cast<float>(glyphs.size());
}

void drawStringConstantAdvance(SDL_Renderer* renderer, const vector<Glyph>& glyphs, FontTransform& ftrans,
                                float constantAdvanceUnits, int baselineY) {
    float cursorX = 40.0F;
    float constantAdvancePixels = unitsToPixels(constantAdvanceUnits, ftrans);
    for (const Glyph& glyph : glyphs) {
        drawSimpleGlyph(renderer, glyph, ftrans, static_cast<int>(cursorX), baselineY, GLYPH_THICKNESS);
        cursorX += constantAdvancePixels;
    }
}

void drawStringHmtxAdvance(SDL_Renderer* renderer, const vector<Glyph>& glyphs, FontTransform& ftrans, int baselineY) {
    float cursorX = 40.0F;
    for (const Glyph& glyph : glyphs) {
        drawSimpleGlyph(renderer, glyph, ftrans, static_cast<int>(cursorX), baselineY, GLYPH_THICKNESS);
        cursorX += ftrans.toPixels(glyph.getAdvanceWidth());
    }
}

void drawParagraph(SDL_Renderer* renderer, const vector<Glyph>& glyphs, FontTransform& ftrans,
                    float lineHeightUnits, int firstBaselineY) {
    float lineHeightPixels = unitsToPixels(lineHeightUnits, ftrans);
    for (int line = 0; line < PARAGRAPH_LINE_COUNT; ++line) {
        float cursorX = 40.0F;
        auto baselineY = static_cast<int>(static_cast<float>(firstBaselineY) + (lineHeightPixels * static_cast<float>(line)));
        for (const Glyph& glyph : glyphs) {
            drawSimpleGlyph(renderer, glyph, ftrans, static_cast<int>(cursorX), baselineY, GLYPH_THICKNESS);
            cursorX += ftrans.toPixels(glyph.getAdvanceWidth());
        }
    }
}

// Holds everything the render loop needs: the parsed fonts/glyphs, one
// FontTransform per phase group, and the precomputed layout numbers each
// phase draws with.
struct DemoContext {
    LoadedFont primary;
    LoadedFont mono;
    LoadedFont prop;
    LoadedFont tall;

    Glyph rawG;
    Glyph curveG;
    vector<Glyph> monoGlyphs;
    vector<Glyph> propGlyphs;
    vector<Glyph> paraGlyphs;
    vector<Glyph> tallParaGlyphs;

    FontTransform zoomFtrans;
    FontTransform monoFtrans;
    FontTransform propFtrans;
    FontTransform paraFtrans;
    FontTransform tallFtrans;

    int zoomXOffset = 0;
    int zoomBaselineY = 0;

    // Font-unit values, not pixels -- converted to pixels live at draw time
    // via unitsToPixels(), so they stay correct as pixelsPerEm changes.
    float constantAdvanceMonoUnits = 0.0F;
    float constantAdvancePropUnits = 0.0F;

    float naiveLineHeightUnits = 0.0F;
    float tallNaiveLineHeightUnits = 0.0F;
    float tallRealLineHeightUnits = 0.0F;
    int paraFirstBaselineY = 0;

    int spacingBaselineY = 0;
};

void recomputeZoomLayout(DemoContext& ctx) {
    float xMin = static_cast<float>(ctx.rawG.getXMin());
    float xMax = static_cast<float>(ctx.rawG.getXMax());
    float yMin = static_cast<float>(ctx.rawG.getYMin());
    float yMax = static_cast<float>(ctx.rawG.getYMax());

    float midXUnits = (xMin + xMax) / 2.0F;
    float midYUnits = (yMin + yMax) / 2.0F;

    ctx.zoomXOffset = static_cast<int>((static_cast<float>(SCREEN_WIDTH) / 2.0F) - unitsToPixels(midXUnits, ctx.zoomFtrans));
    ctx.zoomBaselineY = static_cast<int>((static_cast<float>(SCREEN_HEIGHT) / 2.0F) + unitsToPixels(midYUnits, ctx.zoomFtrans));
}

DemoContext buildContext(const string& primaryFontPath) {
    LoadedFont primary = loadFont(primaryFontPath);
    LoadedFont mono = loadFont(MONO_FONT_PATH);
    LoadedFont prop = loadFont(PROP_FONT_PATH);
    LoadedFont tall = loadFont(TALL_FONT_PATH);

    Glyph rawG = primary.ttf.parseGlyph(primary.buffer, static_cast<uint32_t>('g'), false);
    Glyph curveG = primary.ttf.parseGlyph(primary.buffer, static_cast<uint32_t>('g'), true);

    vector<Glyph> monoGlyphs = mono.ttf.parseGlyphs(mono.buffer, SPACING_SAMPLE_TEXT);
    vector<Glyph> propGlyphs = prop.ttf.parseGlyphs(prop.buffer, SPACING_SAMPLE_TEXT);
    vector<Glyph> paraGlyphs = primary.ttf.parseGlyphs(primary.buffer, PARAGRAPH_SAMPLE_LINE);
    vector<Glyph> tallParaGlyphs = tall.ttf.parseGlyphs(tall.buffer, PARAGRAPH_SAMPLE_LINE);

    FontTransform zoomFtrans{900.0F, primary.ttf.getHeadTable().unitsPerEm};
    FontTransform monoFtrans{64.0F, mono.ttf.getHeadTable().unitsPerEm};
    FontTransform propFtrans{64.0F, prop.ttf.getHeadTable().unitsPerEm};
    FontTransform paraFtrans{64.0F, primary.ttf.getHeadTable().unitsPerEm};
    FontTransform tallFtrans{64.0F, tall.ttf.getHeadTable().unitsPerEm};

    float constantAdvanceMonoUnits = averageAdvanceUnits(monoGlyphs);
    float constantAdvancePropUnits = averageAdvanceUnits(propGlyphs);

    const HheaTable& hhea = primary.ttf.getMetricsTable().getHheaTable();
    float naiveLineHeightUnits = static_cast<float>(hhea.ascent);

    const HheaTable& tallHhea = tall.ttf.getMetricsTable().getHheaTable();
    float tallNaiveLineHeightUnits = static_cast<float>(tallHhea.ascent);
    float tallRealLineHeightUnits = static_cast<float>(tallHhea.ascent - tallHhea.descent + tallHhea.lineGap);

    DemoContext ctx{
        std::move(primary), std::move(mono), std::move(prop), std::move(tall),
        std::move(rawG), std::move(curveG),
        std::move(monoGlyphs), std::move(propGlyphs), std::move(paraGlyphs), std::move(tallParaGlyphs),
        zoomFtrans, monoFtrans, propFtrans, paraFtrans, tallFtrans
    };

    ctx.constantAdvanceMonoUnits = constantAdvanceMonoUnits;
    ctx.constantAdvancePropUnits = constantAdvancePropUnits;
    ctx.naiveLineHeightUnits = naiveLineHeightUnits;
    ctx.tallNaiveLineHeightUnits = tallNaiveLineHeightUnits;
    ctx.tallRealLineHeightUnits = tallRealLineHeightUnits;
    ctx.paraFirstBaselineY = 150;
    ctx.spacingBaselineY = SCREEN_HEIGHT / 2;

    recomputeZoomLayout(ctx);
    return ctx;
}

FontTransform& activeTransform(Phase phase, DemoContext& ctx) {
    switch (phase) {
        case Phase::RawPoints:
        case Phase::ColoredPoints:
        case Phase::Skeleton:
        case Phase::ControlHandles:
        case Phase::BezierCurves:
        case Phase::EarmarkingContours:
            return ctx.zoomFtrans;
        case Phase::MonoConstantSpacing:
            return ctx.monoFtrans;
        case Phase::BrokenConstantSpacing:
        case Phase::HmtxSpacing:
            return ctx.propFtrans;
        case Phase::NaiveVerticalSpacing:
            return ctx.paraFtrans;
        case Phase::TallFontNaiveVerticalSpacing:
        case Phase::RealVerticalSpacing:
            return ctx.tallFtrans;
        default:
            return ctx.paraFtrans;
    }
}

void renderPhase(SDL_Renderer* renderer, Phase phase, int earmarkStep, DemoContext& ctx) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    const int dotRadius = 6;

    switch (phase) {
        case Phase::RawPoints:
            drawGlyphPoints(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY, false, dotRadius);
            break;
        case Phase::ColoredPoints:
            drawGlyphPoints(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY, true, dotRadius);
            break;
        case Phase::Skeleton:
            drawGlyphSkeleton(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY);
            drawGlyphPoints(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY, true, dotRadius);
            break;
        case Phase::ControlHandles:
            drawStraightSegments(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY);
            drawControlHandles(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY);
            drawGlyphPoints(renderer, ctx.rawG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY, true, dotRadius);
            break;
        case Phase::BezierCurves:
            drawSimpleGlyph(renderer, ctx.curveG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY, GLYPH_THICKNESS);
            break;
        case Phase::MonoConstantSpacing:
            drawStringConstantAdvance(renderer, ctx.monoGlyphs, ctx.monoFtrans, ctx.constantAdvanceMonoUnits, ctx.spacingBaselineY);
            break;
        case Phase::BrokenConstantSpacing:
            drawStringConstantAdvance(renderer, ctx.propGlyphs, ctx.propFtrans, ctx.constantAdvancePropUnits, ctx.spacingBaselineY);
            break;
        case Phase::HmtxSpacing:
            drawStringHmtxAdvance(renderer, ctx.propGlyphs, ctx.propFtrans, ctx.spacingBaselineY);
            break;
        case Phase::NaiveVerticalSpacing:
            drawParagraph(renderer, ctx.paraGlyphs, ctx.paraFtrans, ctx.naiveLineHeightUnits, ctx.paraFirstBaselineY);
            break;
        case Phase::TallFontNaiveVerticalSpacing:
            drawParagraph(renderer, ctx.tallParaGlyphs, ctx.tallFtrans, ctx.tallNaiveLineHeightUnits, ctx.paraFirstBaselineY);
            break;
        case Phase::RealVerticalSpacing:
            drawParagraph(renderer, ctx.tallParaGlyphs, ctx.tallFtrans, ctx.tallRealLineHeightUnits, ctx.paraFirstBaselineY);
            break;
        case Phase::EarmarkingContours:
            if (earmarkStep == 0) {
                drawFlattenedOutline(renderer, ctx.curveG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY);
            } else {
                drawWindingDirections(renderer, ctx.curveG, ctx.zoomFtrans, ctx.zoomXOffset, ctx.zoomBaselineY);
            }
            break;
        default:
            break;
    }
}

void announcePhase(SDL_Window* window, Phase phase) {
    const PhaseInfo& info = getPhaseInfo(phase);
    int phaseNumber = static_cast<int>(phase) + 1;

    ostringstream title;
    title << "TextRenderer Demo -- Phase " << phaseNumber << "/" << kNumPhases << ": " << info.name;
    SDL_SetWindowTitle(window, title.str().c_str());

    cout << "Phase " << phaseNumber << "/" << kNumPhases << " -- " << info.name << '\n'
         << "  " << info.description << '\n';
}

void announceEarmarkStep(SDL_Window* window, Phase phase, int step) {
    const PhaseInfo& info = getPhaseInfo(phase);
    const SubStepInfo& sub = getEarmarkSubStepInfo(step);
    int phaseNumber = static_cast<int>(phase) + 1;

    ostringstream title;
    title << "TextRenderer Demo -- Phase " << phaseNumber << "/" << kNumPhases << ": " << info.name
          << " -- Step " << (step + 1) << "/" << kNumEarmarkSteps << ": " << sub.name;
    SDL_SetWindowTitle(window, title.str().c_str());

    cout << "Phase " << phaseNumber << "/" << kNumPhases << " -- " << info.name
         << " -- Step " << (step + 1) << "/" << kNumEarmarkSteps << " -- " << sub.name << '\n'
         << "  " << sub.description << '\n';
}

void announceCurrent(SDL_Window* window, Phase phase, int earmarkStep) {
    if (phase == Phase::EarmarkingContours) {
        announceEarmarkStep(window, phase, earmarkStep);
    } else {
        announcePhase(window, phase);
    }
}

Phase phaseFromDigitKey(SDL_Keycode key) {
    // '1'-'9' select phases 1-9; '0' selects phase 10. Phases beyond 10
    // (there are kNumPhases total) are only reachable via '[' / ']'.
    if (key == SDLK_0) {
        return static_cast<Phase>(9);
    }
    return static_cast<Phase>(key - SDLK_1);
}

bool isDigitKey(SDL_Keycode key) {
    return key == SDLK_0 || (key >= SDLK_1 && key <= SDLK_9);
}

bool isZoomPhase(Phase phase) {
    return phase == Phase::RawPoints || phase == Phase::ColoredPoints || phase == Phase::Skeleton ||
           phase == Phase::ControlHandles || phase == Phase::BezierCurves || phase == Phase::EarmarkingContours;
}

void handleEvent(const SDL_Event& evt, SDL_Window* window, Phase& phase, int& earmarkStep, bool& quit, DemoContext& ctx) {
    if (evt.type == SDL_QUIT) {
        quit = true;
        return;
    }
    if (evt.type != SDL_KEYDOWN) {
        return;
    }

    SDL_Keycode key = evt.key.keysym.sym;

    if (key == SDLK_ESCAPE) {
        quit = true;
        return;
    }

    if (isDigitKey(key)) {
        Phase requested = phaseFromDigitKey(key);
        if (static_cast<int>(requested) < kNumPhases && requested != phase) {
            phase = requested;
            earmarkStep = 0;
            announceCurrent(window, phase, earmarkStep);
        }
        return;
    }

    if (key == SDLK_LEFTBRACKET) {
        if (phase == Phase::EarmarkingContours && earmarkStep > 0) {
            --earmarkStep;
            announceEarmarkStep(window, phase, earmarkStep);
            return;
        }
        int prevIdx = std::max(0, static_cast<int>(phase) - 1);
        if (prevIdx != static_cast<int>(phase)) {
            phase = static_cast<Phase>(prevIdx);
            earmarkStep = 0;
            announceCurrent(window, phase, earmarkStep);
        }
        return;
    }
    if (key == SDLK_RIGHTBRACKET) {
        if (phase == Phase::EarmarkingContours && earmarkStep < kNumEarmarkSteps - 1) {
            ++earmarkStep;
            announceEarmarkStep(window, phase, earmarkStep);
            return;
        }
        int nextIdx = std::min(kNumPhases - 1, static_cast<int>(phase) + 1);
        if (nextIdx != static_cast<int>(phase)) {
            phase = static_cast<Phase>(nextIdx);
            earmarkStep = 0;
            announceCurrent(window, phase, earmarkStep);
        }
        return;
    }

    FontTransform& ftrans = activeTransform(phase, ctx);
    if (key == SDLK_PLUS || key == SDLK_EQUALS) {
        ftrans.pixelsPerEm += 10.0F;
        if (isZoomPhase(phase)) {
            recomputeZoomLayout(ctx);
        }
    } else if (key == SDLK_MINUS) {
        ftrans.pixelsPerEm = std::max(ftrans.pixelsPerEm - 10.0F, 10.0F);
        if (isZoomPhase(phase)) {
            recomputeZoomLayout(ctx);
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    string primaryFontPath = argc > 1 ? string(argv[1]) : DEFAULT_PRIMARY_FONT;

    DemoContext ctx = buildContext(primaryFontPath);

    SDL_Window*   window   = initializeWindow("TextRenderer Demo", SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Renderer* renderer = initializeRenderer(window);

    Phase phase = Phase::RawPoints;
    int earmarkStep = 0;
    bool quit = false;
    SDL_Event evt;

    cout << "Controls: 1-9 = phases 1-9 | 0 = phase 10 | [ ] step (also steps sub-stages on phase 12) | +/- zoom | Esc quit\n\n";
    announceCurrent(window, phase, earmarkStep);

    while (!quit) {
        while (SDL_PollEvent(&evt) != 0) {
            handleEvent(evt, window, phase, earmarkStep, quit, ctx);
        }

        renderPhase(renderer, phase, earmarkStep, ctx);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
