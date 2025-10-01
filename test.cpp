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

double scalingFactor = 0.5;
int thickness = 2;

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;

struct Vertex {
  int16_t x;
  int16_t y;
  Vertex* prev;
  Vertex* next;

  Vertex(int16_t _x, int16_t _y) : x(_x), y(_y), prev(nullptr), next(nullptr) {}
};

struct Triangle {
  int16_t x1, y1;
  int16_t x2, y2;
  int16_t x3, y3;
};

static Vertex* createVertList(vector<int16_t> xCoordinates, vector<int16_t> yCoordinates) {
  if (xCoordinates.size() != yCoordinates.size() || xCoordinates.empty()) {
    return nullptr;
  }
  
  Vertex* head = nullptr;
  Vertex* tail = nullptr;
  
  for (size_t i = 0; i < xCoordinates.size(); ++i) {
    Vertex* v = new Vertex(xCoordinates[i], yCoordinates[i]);
    
    if (!head) {
      head = tail = v;
    } else {
      tail->next = v;
      v->prev = tail;
      tail = v;
    }
  }
  
  // Close the circular list
  if (head && tail) {
    tail->next = head;
    head->prev = tail;
  }
  
  return head;
}

// Remove a node from its ring (does NOT delete it). Returns the next node.
static Vertex* detachNode(Vertex* n) {
  if (!n) return nullptr;
  Vertex* nxt = n->next;
  Vertex* prv = n->prev;
  if (nxt && prv) {
    prv->next = nxt;
    nxt->prev = prv;
  }
  // isolate n
  n->next = n->prev = nullptr;
  return nxt;
}

inline int toScreenX(int16_t x, double scale) {
    return static_cast<int>(x * scale);
}

inline int toScreenY(int16_t y, double scale, int height) {
    return static_cast<int>(height - (y * scale) - 50);
}

void drawTriangle(SDL_Renderer* renderer, Vertex* p1, Vertex* p2, Vertex* p3, 
                  double scalingFactor, int height) {
    // Convert all points to screen coordinates
    int x1 = toScreenX(p1->x, scalingFactor);
    int y1 = toScreenY(p1->y, scalingFactor, height);
    
    int x2 = toScreenX(p2->x, scalingFactor);
    int y2 = toScreenY(p2->y, scalingFactor, height);
    
    int x3 = toScreenX(p3->x, scalingFactor);
    int y3 = toScreenY(p3->y, scalingFactor, height);

    //printf("Drawing Triangle at p1: (%i, %i), p2: (%i, %i), p3: (%i, %i)\n", x1, y1, x2, y2, x3, y3); 
    
    // Draw the three edges
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
    SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
}

static bool isCCW(Vertex* start) {
  int signedArea = 0;
  Vertex* cur = start->next;
  Vertex* next = cur->next;
  while (cur != start) {
    signedArea += (double)cur->x * next->y - (double)next->x * cur->y;
    cur = next;
    next = next->next;
  }

  return signedArea > 0;
}

bool isCollinear(Vertex* p0, Vertex* p1, Vertex* p2) {
  return abs((p2->y - p0->y) * (p1->x - p0->x) - (p1->y - p0->y) * (p2->x - p0->x)) < 1e-5;
}

float sign(Vertex* p1, Vertex* p2, Vertex* p3)
{
    return (p1->x - p3->x) * (p2->y - p3->y) - (p2->x - p3->x) * (p1->y - p3->y);
}

bool PointInTriangle (Vertex* pt, Vertex* v1, Vertex* v2, Vertex* v3)
{
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(pt, v1, v2);
    d2 = sign(pt, v2, v3);
    d3 = sign(pt, v3, v1);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

static bool isConvex(Vertex* i0, Vertex* i1, Vertex* i2, bool isCCW) {
  int64_t dx1 = i1->x - i0->x;
  int64_t dy1 = i1->y - i0->y;
  int64_t dx2 = i2->x - i1->x;
  int64_t dy2 = i2->y - i1->y;

  int64_t cross = dx1 * dy2 - dy1 * dx2;
  return isCCW ? (cross > 0) : (cross < 0);
}

static void drawTriangulationTest(SDL_Renderer* renderer, Glyph glyph) {
  // Gets data from the glyph
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();
  
  // vectors of relevant points for triangulation
  vector<int16_t> xOnCurve;
  vector<int16_t> yOnCurve;

  // vector of ears
  vector<Triangle> ears;

  // add only on curve points to our vertex list
  for (int i = 0; i < (int)xCoordinates.size(); i++) {
    if (flags[i] & 1) {
      xOnCurve.push_back(xCoordinates[i]);
      yOnCurve.push_back(yCoordinates[i]);
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      drawCircle(renderer, xCoordinates[i], yCoordinates[i], 4);
    }
  }

  // create the vertex list
  int numVerts = xOnCurve.size();
  Vertex* node = createVertList(xOnCurve, yOnCurve);

  // figure out winding direction
  bool ccw = isCCW(node);
  
  // main ear clipping loop
  while (numVerts > 2) {
    // current vertices
    Vertex* i0 = node->prev;
    Vertex* i1 = node;
    Vertex* i2 = node->next;
    
    // test convex or reflex triangle
    bool isCvx = isConvex(i0, i1, i2, ccw);

    // test if convex triangle is an ear
    bool isEar = true;
    if (isCvx) {
      // create a test point then loop over all points not in i0, i1, i2
      Vertex* testPoint = node->next->next;
      while ((testPoint != i0) and isEar) {
        // disqualify a tri if another vertex is in it
        isEar = !PointInTriangle(testPoint, i0, i1, i2);
        testPoint = testPoint->next;
      }
    // non convex tris cannot be ears
    } else {
      isEar = false;
    }

    // add ear to ears list, detatch it from the linked list and decrease the vert count
    if (isEar) {
      ears.push_back({i0->x, i0->y, i1->x, i1->y, i2->x, i2->y});
      node = detachNode(node);
      --numVerts;
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      drawTriangle(renderer, i0, i1, i2, 0.5, 500);
    // if not an ear move on
    } else {
      node = node->next;
    }
  }
}

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

    string textToRender = "S";

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
                drawTriangulationTest(renderer, glyphs[i]);
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
