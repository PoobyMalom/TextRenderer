#ifndef SDLINITIALIZER_H
#define SDLINITIALIZER_H

#include <SDL.h>
#include <iostream>

SDL_Window* initializeWindow(const char* title, int width, int height);
SDL_Renderer* initializeRenderer(SDL_Window* window);
SDL_Texture* intializeTexture(SDL_Renderer* renderer, SDL_Window* window, int width, int height);

#endif // SDLINITIALIZER_H