#include "SDLInitializer.h"

using namespace std;

SDL_Window* initializeWindow(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return nullptr;
    }

    SDL_Window* window = SDL_CreateWindow(title, width, height, 0);
    if (window == nullptr) {
        cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << '\n';
        SDL_Quit();
        return nullptr;
    }
    return window;
}

SDL_Renderer* initializeRenderer(SDL_Window* window) {
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return nullptr;
    }
    return renderer;
}

SDL_Texture* initializeTexture(SDL_Renderer* renderer, SDL_Window* window, int width, int height) {
    SDL_Texture* canvasTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (canvasTexture == nullptr) {
        cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << '\n';
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return nullptr;
    }
    return canvasTexture;
}