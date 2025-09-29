#include <SDL2/SDL.h>
#include "third_party/glad/include/glad/glad.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <bitset>
#include <cstring>
#include <cstdint>
#include <limits>

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

// -------------------------
// Types & helpers
// -------------------------
static_assert(sizeof(Line) == 4*sizeof(float), "Line must be 4 floats");

static std::string read_text_file(const std::string& path) {
    std::ifstream f(path, std::ios::in);
    if (!f) throw std::runtime_error("Failed to open file: " + path);
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static vector<Line> getLineSegs(vector<Glyph> glyphs, double scalingFactor, HmtxTable htmxTable) {
    vector<Line> totalSegs;

    int xOffset = 0;
    
    for (Glyph glyph : glyphs) {
        vector<Line> segs = glyph.getGlyphSegments(xOffset, 0, scalingFactor, 600);
        xOffset += htmxTable.getHMetrics()[glyph.getGID()].advanceWidth * scalingFactor;
        cout << "Glyph " << glyph.getGID() << " has " << segs.size() << "Line segments" << endl;
        for (Line line : segs) {
            totalSegs.push_back(line);
        }
    }

    return totalSegs;
}

static GLuint compile_shader_from_src(GLenum type, const char* src, const char* debugName) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint n = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &n);
        std::string log(std::max(1, n), '\0');
        glGetShaderInfoLog(s, n, nullptr, log.data());
        std::cerr << "Shader compile error (" << debugName << ")\n" << log << std::endl;
    }
    return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint n = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &n);
        std::string log(std::max(1, n), '\0');
        glGetProgramInfoLog(p, n, nullptr, log.data());
        std::cerr << "Program link error\n" << log << std::endl;
    }
    return p;
}

// -------------------------
// Shaders (inline)
// -------------------------
// Fullscreen vertex (no VBO)
static const char* kRayVS = R"GLSL(
#version 330 core
const vec2 V[3] = vec2[](
  vec2(-1.0, -1.0),
  vec2( 3.0, -1.0),
  vec2(-1.0,  3.0)
);
void main(){ gl_Position = vec4(V[gl_VertexID], 0.0, 1.0); }
)GLSL";

// Fragment: winding vs even-odd using a TBO of segments in NDC
static const char* kRayFS = R"GLSL(
#version 330 core
uniform samplerBuffer u_segments; // RGBA32F texels: (ax, ay, bx, by)
uniform int   u_count;
uniform vec2  u_resolution;      // window size in pixels
out vec4 FragColor;

float isLeft(vec2 a, vec2 b, vec2 p){
    return (b.x - a.x)*(p.y - a.y) - (b.y - a.y)*(p.x - a.x);
}
int windingNumber(vec2 p){
    int wn = 0;
    for (int i=0;i<u_count;++i){
        vec4 s = texelFetch(u_segments, i);
        vec2 a = s.xy, b = s.zw;
        if (a.y <= p.y && b.y >  p.y && isLeft(a,b,p) > 0.0) wn += 1; // up
        if (a.y >  p.y && b.y <= p.y && isLeft(a,b,p) < 0.0) wn -= 1; // down
    }
    return wn;
}
bool insideEvenOdd(vec2 p){
    bool odd=false;
    for (int i=0;i<u_count;++i){
        vec4 s = texelFetch(u_segments, i);
        vec2 a = s.xy, b = s.zw;
        bool straddles = ((a.y > p.y) != (b.y > p.y));
        if (straddles){
            float xint = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (xint > p.x) odd = !odd;
        }
    }
    return odd;
}
void main(){
    vec2 uv = gl_FragCoord.xy / u_resolution; // 0..1
    vec2 p  = uv*2.0 - 1.0;                   // -1..+1
    int wn = windingNumber(p);
    bool eo = insideEvenOdd(p);
    vec3 col = vec3(0.0);
    if (wn != 0) col += vec3(0.0,1.0,0.0);
    if (eo)      col += vec3(1.0,0.0,0.0);
    FragColor = vec4(col, 1.0);
}
)GLSL";

// Overlay VS: draw NDC triangles from VBO (aPos)
static const char* kOverlayVS = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
void main(){ gl_Position = vec4(aPos, 0.0, 1.0); }
)GLSL";
// Overlay FS: constant color with alpha
static const char* kOverlayFS = R"GLSL(
#version 330 core
out vec4 FragColor;
uniform vec4 u_color; // rgba
void main(){ FragColor = u_color; }
)GLSL";

int main(int, char**){
    // -------------------------
    // Load + parse font, build glyph segments
    // -------------------------
    // std::ifstream file("src/fonts/Monsieur_La_Doulaise/MonsieurLaDoulaise-Regular.ttf", std::ios::binary);
    std::ifstream file("src/fonts/JetBrainsMono-Bold.ttf", std::ios::binary);
    file.seekg(0, std::ios::end); std::streampos fileSize = file.tellg(); file.seekg(0, std::ios::beg);
    if (fileSize <= 0) { std::cerr << "Font file missing/empty" << std::endl; return 1; }
    std::vector<char> buffer((size_t)fileSize); file.read(buffer.data(), fileSize);

    TTFFile ttfFile = TTFFile::parse(buffer);
    std::string textToRender = "a";

    std::vector<Glyph> glyphs;
    try { glyphs = ttfFile.parseGlyphs(buffer, textToRender); }
    catch (const std::exception& e) { std::cerr << "parseGlyphs: " << e.what() << std::endl; return 1; }

    // Take first glyph -> segments in screen space (your function)
    std::vector<Line> glyphSegs = getLineSegs(glyphs, 0.5, ttfFile.getHmtxTable());
    cout << "Length of glyphSegs: " << glyphSegs.size() << endl;

    // Normalize to NDC (flipY true if your input Y was screen-style)
    std::vector<Line> normalizedSegs = normalizeToNDC(glyphSegs, /*margin*/0.00f, /*flipY*/true);
    cout << "Total Line Segments: " << normalizedSegs.size() << endl;
    // -------------------------
    // SDL + GL init
    // -------------------------
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { std::cerr << "SDL_Init: " << SDL_GetError() << std::endl; return 1; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* win = SDL_CreateWindow("Winding Ray Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 800, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if (!win) { std::cerr << "SDL_CreateWindow: " << SDL_GetError() << std::endl; return 1; }
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { std::cerr << "SDL_GL_CreateContext: " << SDL_GetError() << std::endl; return 1; }
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) { std::cerr << "Failed to load OpenGL via GLAD" << std::endl; return 1; }
    std::cout << "GL Renderer: " << (const char*)glGetString(GL_RENDERER)
              << " | Version: " << (const char*)glGetString(GL_VERSION) << "\n";

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // -------------------------
    // Build segment TBO (samplerBuffer)
    // -------------------------
    std::vector<float> segTexels; segTexels.reserve(normalizedSegs.size()*4);
    for (const auto& s : normalizedSegs) { segTexels.insert(segTexels.end(), {s.ax,s.ay,s.bx,s.by}); }

    GLuint segBuf=0, segTex=0;
    glGenBuffers(1, &segBuf);
    glBindBuffer(GL_TEXTURE_BUFFER, segBuf);
    glBufferData(GL_TEXTURE_BUFFER, segTexels.size()*sizeof(float), segTexels.data(), GL_DYNAMIC_DRAW);
    glGenTextures(1, &segTex);
    glBindTexture(GL_TEXTURE_BUFFER, segTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, segBuf);

    // -------------------------
    // Build wireframe VBO
    // -------------------------
    bool showWire = true;  // top of main, near other state

    std::vector<float> lineVerts; 
    lineVerts.reserve(normalizedSegs.size() * 4);
    for (const auto& s : normalizedSegs) {
        lineVerts.push_back(s.ax); lineVerts.push_back(s.ay);
        lineVerts.push_back(s.bx); lineVerts.push_back(s.by);
    }

    GLuint lineVAO = 0, lineVBO = 0;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(float), lineVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // -------------------------
    // Create programs
    // -------------------------
    GLuint rayVS = compile_shader_from_src(GL_VERTEX_SHADER,   kRayVS, "ray VS");
    GLuint rayFS = compile_shader_from_src(GL_FRAGMENT_SHADER, kRayFS, "ray FS");
    GLuint rayProg = link_program(rayVS, rayFS); glDeleteShader(rayVS); glDeleteShader(rayFS);

    GLuint ovVS = compile_shader_from_src(GL_VERTEX_SHADER,   kOverlayVS, "overlay VS");
    GLuint ovFS = compile_shader_from_src(GL_FRAGMENT_SHADER, kOverlayFS, "overlay FS");
    GLuint ovProg = link_program(ovVS, ovFS); glDeleteShader(ovVS); glDeleteShader(ovFS);

    // Fullscreen VAO (no VBO)
    GLuint fsVAO=0; glGenVertexArrays(1, &fsVAO);

    // Optional overlay: triangles built from segments (each line -> triangle to a corner)
    std::vector<float> triVerts; triVerts.reserve(normalizedSegs.size()*6);
    for (const auto& s : normalizedSegs) {
        triVerts.push_back(s.ax); triVerts.push_back(s.ay);
        triVerts.push_back(s.bx); triVerts.push_back(s.by);
        triVerts.push_back(-1.0f); triVerts.push_back( 1.0f); // corner
    }
    GLuint ovVAO=0, ovVBO=0;
    glGenVertexArrays(1, &ovVAO);
    glGenBuffers(1, &ovVBO);
    glBindVertexArray(ovVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ovVBO);
    glBufferData(GL_ARRAY_BUFFER, triVerts.size()*sizeof(float), triVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);

    // -------------------------
    // Main loop
    // -------------------------
    uint64_t frame = 0; Uint64 perfFreq = SDL_GetPerformanceFrequency(); Uint64 tLast = SDL_GetPerformanceCounter();
    bool running = true; int w=0, h=0;
    while (running) {
        SDL_Event e; while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                glViewport(0, 0, e.window.data1, e.window.data2);
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w) showWire = !showWire;
            }
        }
        SDL_GL_GetDrawableSize(win, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- Pass 1: Ray test visualization (fullscreen triangle)
        glUseProgram(rayProg);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_BUFFER, segTex);
        glUniform1i(glGetUniformLocation(rayProg, "u_segments"), 0);
        glUniform1i(glGetUniformLocation(rayProg, "u_count"), (int)normalizedSegs.size());
        glUniform2f(glGetUniformLocation(rayProg, "u_resolution"), (float)w, (float)h);
        glBindVertexArray(fsVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- Pass 2: Overlay triangles (low alpha) to see overlaps/coverage
        // glUseProgram(ovProg);
        // glUniform4f(glGetUniformLocation(ovProg, "u_color"), 1.0f, 0.7f, 0.2f, 0.10f); // 10% alpha
        // glBindVertexArray(ovVAO);
        // glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(triVerts.size()/2));

        // --- Pass 2: Wireframe lines for segment edges
        if (showWire) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glUseProgram(ovProg);
            glUniform4f(glGetUniformLocation(ovProg, "u_color"), 0.10f, 0.80f, 1.00f, 1.00f); // cyan-ish
            glBindVertexArray(lineVAO);
            glLineWidth(1.5f);              // tweak to taste
            glDrawArrays(GL_LINES, 0, (GLsizei)(lineVerts.size() / 2));
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        SDL_GL_SwapWindow(win);

        // FPS print (once a second)
        Uint64 tNow = SDL_GetPerformanceCounter(); double dt = double(tNow - tLast)/double(perfFreq); tLast = tNow;
        static double acc=0.0; static int n=0; acc+=dt; n++; frame++;
        if (acc >= 1.0) { std::cout << "frame=" << frame << " fps=" << (n/acc) << "\r" << std::flush; acc=0.0; n=0; }
    }

    // Cleanup
    glDeleteProgram(rayProg);
    glDeleteProgram(ovProg);
    glDeleteBuffers(1, &ovVBO);
    glDeleteVertexArrays(1, &ovVAO);
    glDeleteVertexArrays(1, &fsVAO);
    glDeleteTextures(1, &segTex);
    glDeleteBuffers(1, &segBuf);
    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &lineVAO);

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
