// Benchmarks the constrained-Delaunay pipeline (the same contour-building +
// triangulateConstrained() + drawTriangle() path drawTriangulatedGlyph in
// Renderer.cpp uses) across every font in src/fonts, timing calculation and
// render separately per glyph.
#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <sys/resource.h>
#include <vector>

#include "TTFFile.h"
#include "Metrics.h"
#include "GlyphTable.h"
#include "cdt/constrain.h"
#include "cdt/draw.h"

using namespace std;
using Clock = std::chrono::steady_clock;

namespace {

const float FONT_SIZE = 400.0F;
const string TEST_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

struct GlyphResult {
    char character;
    size_t contourCount;
    size_t pointCount;
    size_t triangleCount;
    double calcMicros;
    double renderMicros;
};

struct FontResult {
    string name;
    double parseMicros = 0.0;
    vector<GlyphResult> glyphs;
    int skipped = 0;
    int failed = 0;
};

vector<Contour> buildContoursFromGlyph(const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset) {
    const vector<uint16_t>& endpoints = glyph.getEndPtsOfContours();
    const vector<int16_t>& xCoordinates = glyph.getXCoordinates();
    const vector<int16_t>& yCoordinates = glyph.getYCoordinates();
    const vector<uint8_t>& flags = glyph.getFlags();

    vector<Contour> contours;
    contours.reserve(endpoints.size());

    int contourStartIndex = 0;
    for (uint16_t endpoint : endpoints) {
        Contour contour;
        size_t count = endpoint - contourStartIndex + 1;
        for (size_t k = 0; k < count; ++k) {
            size_t idx = contourStartIndex + k;
            if ((flags[idx] & 1) != 0) {
                int px = static_cast<int>(ftrans.toPixels(xCoordinates[idx]) + xOffset);
                int py = ftrans.toScreenY(yCoordinates[idx], yOffset);
                contour.push_back({px, py});
            }
        }
        contours.push_back(contour);
        contourStartIndex = static_cast<int>(endpoint) + 1;
    }
    return contours;
}

bool validForTriangulation(const vector<Contour>& contours) {
    if (contours.empty()) {
        return false;
    }
    for (const Contour& contour : contours) {
        if (contour.size() < 3) {
            return false;
        }
    }
    return true;
}

double toMicros(Clock::duration duration) {
    return std::chrono::duration<double, std::micro>(duration).count();
}

double sum(const vector<GlyphResult>& glyphs, double GlyphResult::*field) {
    double total = 0.0;
    for (const GlyphResult& g : glyphs) {
        total += g.*field;
    }
    return total;
}

double sumSize(const vector<GlyphResult>& glyphs, size_t GlyphResult::*field) {
    double total = 0.0;
    for (const GlyphResult& g : glyphs) {
        total += static_cast<double>(g.*field);
    }
    return total;
}

FontResult benchmarkFont(const string& path, SDL_Renderer* renderer) {
    FontResult result;
    result.name = std::filesystem::path(path).filename().string();

    ifstream file(path, ios::binary);
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);
    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    TTFFile ttfFile = [&] {
        auto start = Clock::now();
        TTFFile parsed = TTFFile::parse(buffer);
        result.parseMicros = toMicros(Clock::now() - start);
        return parsed;
    }();

    FontTransform ftrans{FONT_SIZE, ttfFile.getHeadTable().unitsPerEm};
    int yOffset = static_cast<int>(ftrans.toPixels(ttfFile.getMetricsTable().getHheaTable().ascent));

    for (char ch : TEST_CHARS) {
        optional<Glyph> glyph;
        try {
            glyph = ttfFile.parseGlyph(buffer, static_cast<uint32_t>(ch));
        } catch (const std::exception&) {
            ++result.failed;
            continue;
        }

        vector<Contour> contours = buildContoursFromGlyph(*glyph, ftrans, 0, yOffset);
        if (!validForTriangulation(contours)) {
            ++result.skipped;
            continue;
        }

        vector<Triangle> triangles;
        try {
            auto calcStart = Clock::now();
            triangles = triangulateConstrained(contours);
            double calcMicros = toMicros(Clock::now() - calcStart);

            auto renderStart = Clock::now();
            for (const Triangle& triangle : triangles) {
                drawTriangle(renderer, triangle);
            }
            double renderMicros = toMicros(Clock::now() - renderStart);

            size_t pointCount = 0;
            for (const Contour& contour : contours) {
                pointCount += contour.size();
            }

            result.glyphs.push_back({ch, contours.size(), pointCount, triangles.size(), calcMicros, renderMicros});
        } catch (const std::exception&) {
            ++result.failed;
        }
    }

    return result;
}

void printFontRow(const FontResult& font) {
    if (font.glyphs.empty()) {
        cout << left << setw(34) << font.name << "  (no glyphs benchmarked -- skipped=" << font.skipped
             << " failed=" << font.failed << ")\n";
        return;
    }

    double totalCalc = sum(font.glyphs, &GlyphResult::calcMicros);
    double totalRender = sum(font.glyphs, &GlyphResult::renderMicros);
    double totalTriangles = sumSize(font.glyphs, &GlyphResult::triangleCount);
    size_t n = font.glyphs.size();

    auto maxCalc = std::max_element(font.glyphs.begin(), font.glyphs.end(),
                                     [](const GlyphResult& a, const GlyphResult& b) { return a.calcMicros < b.calcMicros; });

    cout << left << setw(34) << font.name << right << setw(6) << n << " glyphs" << setw(12) << fixed
         << setprecision(1) << totalCalc << " us calc" << setw(12) << (totalCalc / static_cast<double>(n))
         << " us/glyph" << setw(12) << totalRender << " us render" << setw(12)
         << (totalRender / static_cast<double>(n)) << " us/glyph" << setw(10)
         << static_cast<long long>(totalTriangles) << " tris"
         << "   slowest='" << maxCalc->character << "' " << maxCalc->calcMicros << "us"
         << "  parse=" << font.parseMicros << "us"
         << (font.skipped || font.failed ? "  [skipped=" + to_string(font.skipped) + " failed=" + to_string(font.failed) + "]" : "")
         << "\n";
}

} // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("CDT Benchmark", 64, 64, SDL_WINDOW_HIDDEN);
    if (window == nullptr) {
        cerr << "Window could not be created: " << SDL_GetError() << '\n';
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        cerr << "Renderer could not be created: " << SDL_GetError() << '\n';
        return 1;
    }

    vector<string> fontPaths;
    for (const auto& entry : std::filesystem::directory_iterator("src/fonts")) {
        if (entry.path().extension() == ".ttf") {
            fontPaths.push_back(entry.path().string());
        }
    }
    std::sort(fontPaths.begin(), fontPaths.end());

    cout << "Constrained Delaunay triangulation benchmark -- " << TEST_CHARS.size()
         << " test characters per font, " << fontPaths.size() << " fonts\n\n";

    vector<FontResult> results;
    for (const string& path : fontPaths) {
        FontResult result;
        try {
            result = benchmarkFont(path, renderer);
        } catch (const std::exception& err) {
            result.name = std::filesystem::path(path).filename().string();
            cout << result.name << "  -- font-level parse threw: " << err.what() << " (skipped entirely)\n";
            results.push_back(std::move(result));
            continue;
        }
        printFontRow(result);
        results.push_back(std::move(result));
    }

    // Overall summary across every font.
    double grandCalc = 0.0;
    double grandRender = 0.0;
    double grandTriangles = 0.0;
    size_t grandGlyphs = 0;
    for (const FontResult& font : results) {
        grandCalc += sum(font.glyphs, &GlyphResult::calcMicros);
        grandRender += sum(font.glyphs, &GlyphResult::renderMicros);
        grandTriangles += sumSize(font.glyphs, &GlyphResult::triangleCount);
        grandGlyphs += font.glyphs.size();
    }

    cout << "\n--- overall ---\n";
    cout << "fonts benchmarked:   " << results.size() << "\n";
    cout << "glyphs benchmarked:  " << grandGlyphs << "\n";
    cout << "total calc time:     " << fixed << setprecision(1) << grandCalc << " us ("
         << (grandCalc / 1000.0) << " ms)\n";
    cout << "total render time:   " << grandRender << " us (" << (grandRender / 1000.0) << " ms)\n";
    cout << "avg calc/glyph:      " << (grandGlyphs ? grandCalc / static_cast<double>(grandGlyphs) : 0.0) << " us\n";
    cout << "avg render/glyph:    " << (grandGlyphs ? grandRender / static_cast<double>(grandGlyphs) : 0.0) << " us\n";
    cout << "total triangles:     " << static_cast<long long>(grandTriangles) << "\n";

    struct rusage usage {};
    getrusage(RUSAGE_SELF, &usage);
    cout << "peak RSS:            " << (usage.ru_maxrss / 1024) << " MB\n";

    // Per-glyph CSV for anyone who wants to dig into outliers.
    ofstream csv("benchmark_results.csv");
    csv << "font,character,contours,points,triangles,calc_us,render_us\n";
    for (const FontResult& font : results) {
        for (const GlyphResult& glyph : font.glyphs) {
            csv << font.name << "," << glyph.character << "," << glyph.contourCount << "," << glyph.pointCount
                << "," << glyph.triangleCount << "," << glyph.calcMicros << "," << glyph.renderMicros << "\n";
        }
    }
    cout << "\nper-glyph details written to benchmark_results.csv\n";

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
