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
#include "HheaTable.h"
#include "GlyphTable.h"
#include "TTFFile.h"
#include "Helpers.h"
#include "SDLInitializer.h"
using namespace std;

double scalingFactor = 0.5;
int thickness = 2;

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;

// Iterate a ring once and run a callable f(node*)
template <typename F>
static void forEachInRing(PointNode* ring, F f) {
  if (!ring) return;
  PointNode* cur = ring;
  do {
    f(cur);
    cur = cur->next;
  } while (cur != ring);
}

// Build a circular doubly linked ring from [start, end] (inclusive) indices
// using coordinates/flags arrays. Returns pointer to an arbitrary node in the ring (the "head").
static PointNode* buildRing(const std::vector<int16_t>& xs,
                            const std::vector<int16_t>& ys,
                            const std::vector<uint8_t>& flags,
                            size_t start, size_t end) {
  if (start > end) return nullptr;
  PointNode* head = nullptr;
  PointNode* tail = nullptr;
  for (size_t i = start; i <= end; ++i) {
    bool onCurve = (flags[i] & 1) != 0;
    PointNode* n = new PointNode(xs[i], ys[i], onCurve);
    if (!head) {
      head = tail = n;
    } else {
      // append
      tail->next = n;
      n->prev = tail;
      tail = n;
    }
  }
  // close the ring
  if (head && tail) {
    head->prev = tail;
    tail->next = head;
  }
  return head;
}

// Remove a node from its ring (does NOT delete it). Returns the next node.
static PointNode* detachNode(PointNode* n) {
  if (!n) return nullptr;
  PointNode* nxt = n->next;
  PointNode* prv = n->prev;
  if (nxt && prv) {
    prv->next = nxt;
    nxt->prev = prv;
  }
  // isolate n
  n->next = n->prev = nullptr;
  return nxt;
}

// Delete an entire ring (safe if ring is already nullptr).
static void destroyRing(PointNode*& ring) {
  if (!ring) return;
  PointNode* cur = ring->next;
  while (cur && cur != ring) {
    PointNode* tmp = cur;
    cur = cur->next;
    delete tmp;
  }
  delete ring;
  ring = nullptr;
}


float angleCCW(PointNode a, PointNode b) {
  float dot = a.x*b.x + a.y*b.y;
  double det = double(a.x)*double(b.y) - double(a.y)*double(b.x); 
  float angle = atan2(det, dot);
  if (angle < 0.0) {
    angle = 2.0 * M_PI + angle;
  }
  return angle;
}

bool isConvex(PointNode* prev, PointNode* cur, PointNode* next, bool isCCW) {
  int64_t dx1 = prev->x - cur->x;
  int64_t dy1 = prev->y - cur->y;
  int64_t dx2 = next->x - cur->x;
  int64_t dy2 = next->y - cur->y;
  
  int64_t cross = dx1 * dy2 - dy1 * dx2;
  
  return isCCW ? (cross > 0) : (cross < 0);
}

inline int64_t dot(const PointNode& a, const PointNode& b) {
  return static_cast<int64_t>(a.x) * static_cast<int64_t>(b.x) +
         static_cast<int64_t>(a.y) * static_cast<int64_t>(b.y);
}

bool insideTriangle(PointNode* a, PointNode* b, PointNode* c, PointNode* p) {
  PointNode v0 = {c->x - a->x, c->y - a->y, a->onCurve};
  PointNode v1 = {b->x - a->x, b->y - a->y, a->onCurve};
  PointNode v2 = {p->x - a->x, p->y - a->y, a->onCurve};

  int64_t dot00 = dot(v0, v0);
  int64_t dot01 = dot(v0, v1);
  int64_t dot02 = dot(v0, v2);
  int64_t dot11 = dot(v1, v1);
  int64_t dot12 = dot(v1, v2);

  double denom = (double)dot00 * dot11 - (double)dot01 * dot01;  // FIX
  if (fabs(denom) < 1e-10) {
    return true;  // FIX: degenerate triangle, reject
  }
  
  double invDenom = 1.0 / denom;  // FIX: use double
  double u = ((double)dot11 * dot02 - (double)dot01 * dot12) * invDenom;
  double v = ((double)dot00 * dot12 - (double)dot01 * dot02) * invDenom;

  return (u >= 0) && (v >= 0) && ((u + v) <= 1);  // use <= for boundary
}

static void drawTriangulationTest(SDL_Renderer* renderer, Glyph glyph) {
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();
  
  // Build linked rings per contour
  std::vector<PointNode*> contours;
  contours.reserve(endpoints.size());

  size_t start = 0;
  for (size_t ci = 0; ci < endpoints.size(); ++ci) {
    size_t end = endpoints[ci];  // inclusive
    PointNode* ring = buildRing(xCoordinates, yCoordinates, flags, start, end);
    if (ring) contours.push_back(ring);
    start = end + 1;
  }

  // Validate contours
  if (contours.empty() || !contours.front()) {
    std::cerr << "No contours or null ring head\n";
    return;
  }

  cout << "Number of contours: " << contours.size() << endl;

  // Verify ring integrity and count nodes
  auto* head = contours.front();
  int ringLen = 0;
  bool bad = false;
  forEachInRing(head, [&](PointNode* n){
    if (!n || !n->prev || !n->next) {
      std::cerr << "Broken link at node=" << n << " prev=" << (n ? n->prev : nullptr)
                << " next=" << (n ? n->next : nullptr) << "\n";
      bad = true;
    }
    ++ringLen;
  });
  
  if (bad || ringLen < 3) {
    std::cerr << "Ring invalid or too short, len=" << ringLen << "\n";
    return;
  }

  // Debug render the points (blue on-curve, red off-curve)
  for (PointNode* ring : contours) {
    forEachInRing(ring, [&](PointNode* n){
      if (n->onCurve) SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      else            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      const int px = static_cast<int>(n->x * scalingFactor);
      const int py = static_cast<int>(SCREEN_HEIGHT - (n->y * scalingFactor) - 50);
      drawCircle(renderer, px, py, 2);
    });
  }

  std::cout << "Vertices in order:\n";
  int idx = 0;
  forEachInRing(contours.front(), [&](PointNode* n){
      std::cout << idx++ << ": (" << n->x << "," << n->y << ")\n";
  });

  // Calculate signed area to determine winding order
  double signedArea = 0;
  forEachInRing(contours.front(), [&](PointNode* n){
      PointNode* next = n->next;
      signedArea += (double)n->x * next->y - (double)next->x * n->y;
  });
  
  bool isCCW = signedArea > 0;
  std::cout << "Signed area: " << signedArea 
            << (isCCW ? " (CCW)" : " (CW)") << "\n";
    
  // Store triangles as coordinate values instead of pointers
  vector<Triangle> triangles;
  triangles.reserve(ringLen - 2);  // A polygon with n vertices has n-2 triangles
  
  PointNode* node = contours.front();
  int numNodes = ringLen;   
  int consecutiveFailures = 0;
  
  while (numNodes > 2) {
    if (consecutiveFailures > numNodes) {
      cout << "Num Nodes Left: " << numNodes << endl;
      std::cerr << "Ear clipping failed - no ear found after full traversal\n";
      std::cerr << "This likely means the polygon is self-intersecting or has other issues\n";
      break;
    }

    PointNode* i = node->prev;
    PointNode* j = node;
    PointNode* k = node->next;

    // Check if this vertex forms a convex angle
    bool isConvexVertex = isConvex(i, j, k, isCCW);
    bool isEar = isConvexVertex;  // Start with assumption that convex = ear

    if (isConvexVertex) {
        PointNode* testNode = k->next;
        int testCount = 0;
        while (testNode != i) {
            if (insideTriangle(i, j, k, testNode)) {
                std::cout << "  Point (" << testNode->x << "," << testNode->y 
                          << ") is inside triangle, blocking ear\n";
                isEar = false;
                break;
            }
            testNode = testNode->next;
            testCount++;
        }
        if (isEar) {
            std::cout << "  Tested " << testCount << " points, all outside\n";
        }
    }

    // If convex, check if any other vertices are inside the triangle
    if (isConvexVertex) {
      PointNode* testNode = k->next;  // Start after k
      while (testNode != i) {  // Check all vertices except i, j, k
        if (insideTriangle(i, j, k, testNode)) {
          isEar = false;
          break;  // Found a point inside, not an ear
        }
        testNode = testNode->next;
      }
    }

    // Visual debug: draw candidate triangles
    if (isEar) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  // Yellow for ears
    } else if (isConvexVertex) {
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);     // Green for convex non-ears
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);   // Magenta for reflex
    }
    drawTriangle(renderer, i, j, k, scalingFactor, SCREEN_HEIGHT);

    // If this is an ear, clip it
    if (isEar) {
      // Store triangle coordinates (not pointers)
      triangles.push_back({i->x, i->y, j->x, j->y, k->x, k->y});
      
      std::cout << "Clipping ear: (" << i->x << "," << i->y << ") -> " 
                << "(" << j->x << "," << j->y << ") -> "
                << "(" << k->x << "," << k->y << ")" << std::endl;
      
      // Detach the ear tip vertex
      PointNode* nextStart = k;
      detachNode(j);
      delete j;  // Clean up the detached node
      
      node = nextStart;
      --numNodes;
      consecutiveFailures = 0;  // Reset counter on success
    } else {
      // Move to next vertex
      node = node->next;
      consecutiveFailures++;
    }
  }

  // Draw final triangles
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  for (const Triangle& t : triangles) {
    // Create temporary PointNode structs for drawing
    PointNode p1 = {t.x1, t.y1, true};
    PointNode p2 = {t.x2, t.y2, true};
    PointNode p3 = {t.x3, t.y3, true};
    drawTriangle(renderer, &p1, &p2, &p3, scalingFactor, SCREEN_HEIGHT);
  }

  std::cout << "Generated " << triangles.size() << " triangles from " 
            << ringLen << " vertices\n";

  // Clean up remaining nodes in the ring
  for (PointNode* ring : contours) {
    destroyRing(ring);
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

    string textToRender = "I";

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

        int currentXOffset = 0;
        int currentYOffset = 100;

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
