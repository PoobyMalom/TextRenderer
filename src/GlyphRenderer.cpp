#include "GlyphRenderer.h"

void GlyphRenderer::renderGlyph(SDL_Renderer* renderer, const Glyph& glyph, RenderMode mode, double scale) {
  switch (mode) {
    case TRIANGULATION: drawTriangulationTest(renderer, glyph, scale); break;
    case DEBUG_LINES: drawDebugLines(renderer, glyph, scale); break;
    case POINTS: drawPoints(renderer, glyph, scale); break;
    case INSIDE_OFF_CURVE: drawInsideOffCurvePoints(renderer, glyph, scale); break;
    case OUTSIDE_OFF_CURVE: drawOutsideOffCurvePoints(renderer, glyph, scale); break;
  }
}

void GlyphRenderer::drawTriangle(SDL_Renderer* renderer, Vertex* p1, Vertex* p2, Vertex* p3, 
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

void GlyphRenderer::drawTriangle(SDL_Renderer* renderer, Triangle* tri, double scalingFactor, int height) {
    // Convert all points to screen coordinates
    int x1 = toScreenX(tri->x1, scalingFactor);
    int y1 = toScreenY(tri->y1, scalingFactor, height);
    
    int x2 = toScreenX(tri->x2, scalingFactor);
    int y2 = toScreenY(tri->y2, scalingFactor, height);
    
    int x3 = toScreenX(tri->x3, scalingFactor);
    int y3 = toScreenY(tri->y3, scalingFactor, height);

    //printf("Drawing Triangle at p1: (%i, %i), p2: (%i, %i), p3: (%i, %i)\n", x1, y1, x2, y2, x3, y3); 
    
    // Draw the three edges
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
    SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
}

void GlyphRenderer::drawInsideOffCurvePoints(SDL_Renderer* renderer, Glyph glyph, double scale) {
  // Gets data from the glyph
  //puts("Start of drawTriangle Function");
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();
  
  // vectors of relevant points for triangulation
  vector<int16_t> xOnCurve;
  vector<int16_t> yOnCurve;
  vector<uint8_t> flagOnCurve;

  // create the vertex list
  int numVerts = xCoordinates.size();
  Vertex* initialNode = createVertexList(xCoordinates, yCoordinates, flags);
  //puts("Created vertex list");
  
  // figure out winding direction
  bool ccw = isCCW(initialNode);
  //puts("Determined Winding Direction");
  int consecutiveFailures = 0;

  for (int i = 0; i < numVerts; i++) {
    Vertex* i0 = initialNode->prev;
    Vertex* i1 = initialNode;
    Vertex* i2 = initialNode->next;

    bool isCvx = isConvex(i0, i1, i2, ccw);
    if (isCvx and !(i1->flag & 1)) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      drawCircle(renderer, toScreenX(i1->x, scale), toScreenY(i1->y, scale, 500), 4, scale, 500);
    } else if (!isCvx and !(i1->flag & 1)) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      drawCircle(renderer, toScreenX(i1->x, scale), toScreenY(i1->y, scale, 500), 4, scale, 500);
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
      drawCircle(renderer, toScreenX(i1->x, scale), toScreenY(i1->y, scale, 500), 4, scale, 500);
    }
    initialNode = initialNode->next;
  }
}

void GlyphRenderer::drawOutsideOffCurvePoints(SDL_Renderer* renderer, Glyph glyph, double scale) {
  // Gets data from the glyph
  //puts("Start of drawTriangle Function");
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();
  
  // vectors of relevant points for triangulation
  vector<int16_t> xOnCurve;
  vector<int16_t> yOnCurve;
  vector<uint8_t> flagOnCurve;

  // create the vertex list
  int numVerts = xCoordinates.size();
  Vertex* initialNode = createVertexList(xCoordinates, yCoordinates, flags);
  //puts("Created vertex list");
  
  // figure out winding direction
  bool ccw = isCCW(initialNode);
  //puts("Determined Winding Direction");
  int consecutiveFailures = 0;

  for (int i = 0; i < numVerts; i++) {
    Vertex* i0 = initialNode->prev;
    Vertex* i1 = initialNode;
    Vertex* i2 = initialNode->next;

    bool isCvx = isConvex(i0, i1, i2, ccw);
    if (!isCvx and !(i1->flag & 1)) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      drawCircle(renderer, toScreenX(i1->x, scale), toScreenY(i1->y, scale, 500), 4, scale, 500);
    } else {
      xOnCurve.push_back(xCoordinates[i]);
      yOnCurve.push_back(yCoordinates[i]);
      flagOnCurve.push_back(flags[i]);
    }
    initialNode = initialNode->next;
  }
}

void GlyphRenderer::drawDebugLines(SDL_Renderer* renderer, Glyph glyph, double scale) {
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();

  // create the vertex list
  int numVerts = xCoordinates.size();
  Vertex* initialNode = createVertexList(xCoordinates, yCoordinates, flags);

  for (int i = 0; i < numVerts; i++) {
    Vertex* i0 = initialNode->prev;
    Vertex* i1 = initialNode;

    int x1 = toScreenX(i0->x, scale);
    int y1 = toScreenY(i0->y, scale, 500);
    
    int x2 = toScreenX(i1->x, scale);
    int y2 = toScreenY(i1->y, scale, 500);
    
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    initialNode = initialNode->next;
  }
}

void GlyphRenderer::drawPoints(SDL_Renderer* renderer, Glyph glyph, double scale) {
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();

  for (int i = 0; i < (int)xCoordinates.size(); i++) {
    if (flags[i] & 1) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      drawCircle(renderer, toScreenX(xCoordinates[i], scale), toScreenY(yCoordinates[i], scale, 500), 4, scale, 500);
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      drawCircle(renderer, toScreenX(xCoordinates[i], scale), toScreenY(yCoordinates[i], scale, 500), 4, scale, 500);
    }
  }
}

void GlyphRenderer::drawTriangulationTest(SDL_Renderer* renderer, Glyph glyph, double scale) {
  // Gets data from the glyph
  //puts("Start of drawTriangle Function");
  vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
  vector<int16_t> xCoordinates = glyph.getXCoordinates();
  vector<int16_t> yCoordinates = glyph.getYCoordinates();
  vector<uint8_t> flags = glyph.getFlags();
  
  // vectors of relevant points for triangulation
  vector<int16_t> xOnCurve;
  vector<int16_t> yOnCurve;
  vector<uint8_t> flagOnCurve;

  // vector of ears
  vector<Triangle> ears;
  //puts("Created vectors");
  // add only on curve points to our vertex list
  for (int i = 0; i < (int)xCoordinates.size(); i++) {
    if (flags[i] & 1) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      drawCircle(renderer, toScreenX(xCoordinates[i], scale), toScreenY(yCoordinates[i], scale, 500), 4, scale, 500);
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      drawCircle(renderer, toScreenX(xCoordinates[i], scale), toScreenY(yCoordinates[i], scale, 500), 4, scale, 500);
    }
  }


  // create the vertex list
  int numVerts = xCoordinates.size();
  Vertex* initialNode = createVertexList(xCoordinates, yCoordinates, flags);

  if (!(initialNode->flag & 1)) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    initialNode = initialNode -> next;
  }
  //puts("Created vertex list");
  
  drawCircle(renderer, toScreenX(initialNode->x, scale), toScreenY(initialNode->y, scale, 500), 4, scale, 500);

  // figure out winding direction
  bool ccw = isCCW(initialNode);
  //puts("Determined Winding Direction");
  int consecutiveFailures = 0;

  for (int i = 0; i < numVerts; i++) {
    Vertex* i0 = initialNode->prev;
    Vertex* i1 = initialNode;
    Vertex* i2 = initialNode->next;

    if (!isCollinear(i0, i1, i2) and (i % 2 == 1)) {
      ears.push_back({i0->x, i0->y, i1->x, i1->y, i2->x, i2->y});
      // SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
      // drawTriangle(renderer, i0, i1, i2, 0.5, 500);
    }

    bool isCvx = isConvex(i0, i1, i2, ccw);
    if (isCvx and !(i1->flag & 1)) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      drawCircle(renderer, toScreenX(i1->x, scale), toScreenY(i1->y, scale, 500), 4, scale, 500);
    } else {
      xOnCurve.push_back(i1->x);
      yOnCurve.push_back(i1->y);
      flagOnCurve.push_back(i1->flag);
    }
    initialNode = initialNode->next;
  }


  numVerts = xOnCurve.size();
  Vertex* node = createVertexList(xOnCurve, yOnCurve, flagOnCurve);

  // main ear clipping loop
  while (numVerts > 2) {
    if (consecutiveFailures > numVerts) {
      // cout << "Num Nodes Left: " << numVerts << endl;
      // std::cerr << "Ear clipping failed - no ear found after full traversal\n";
      // std::cerr << "This likely means the polygon is self-intersecting or has other issues\n";
      break;
    }

    //("Running ear clipping loop with %i vertices left\n", numVerts);
    // current vertices
    Vertex* i0 = node->prev;
    Vertex* i1 = node;
    Vertex* i2 = node->next;
    
    bool isColine = isCollinear(i0, i1, i2);
    if (isColine) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
      drawCircle(renderer, toScreenX(i1->x, scale), toScreenY(i1->y, scale, 500), 4, scale, 500);
    }

    // test convex or reflex triangle
    bool isCvx = isConvex(i0, i1, i2, ccw);
    //puts("Determined tri convex");

    // test if convex triangle is an ear
    bool isEar = true;
    if (isCvx & !isColine) {
      // create a test point then loop over all points not in i0, i1, i2
      Vertex* testPoint = node->next->next;
      while ((testPoint != i0) and isEar) {
        // disqualify a tri if another vertex is in it
        isEar = !pointInTriangle(testPoint, i0, i1, i2);
        //puts("Checked if test point in triangle");
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
      //puts("Detached node");
      --numVerts;
      consecutiveFailures = 0;
    // if not an ear move on
    } else {
      node = node->next;
      consecutiveFailures++;
    }
  }

  for (Triangle tri : ears) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      drawTriangle(renderer, &tri, scale, 500);
  }

}