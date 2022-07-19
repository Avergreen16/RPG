#include <SDL2-w64\include\SDL2\SDL.h>
#include <SDL2-w64\include\SDL2\SDL_image.h>
#include <iostream>

#include "RenderWindow.hpp"

using namespace std;

RenderWindow::RenderWindow(const char* p_title, int p_w, int p_h)
    :window(NULL), renderer(NULL)
{
    window = SDL_CreateWindow(p_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, p_w, p_h, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        cout << "Window failed to init. Error: " << SDL_GetError() << endl;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

RenderWindow::~RenderWindow() {
    SDL_DestroyWindow(window);
}

SDL_Texture* RenderWindow::load_texture(const char* p_filePath) {
    SDL_Texture* texture = NULL;
    texture = IMG_LoadTexture(renderer, p_filePath);

    if(texture == NULL) {
        cout << "Failed to load a texture. ERROR: " << SDL_GetError() << endl;
    }

    return texture;
}

void RenderWindow::clear() {
    SDL_RenderClear(renderer);
}

void RenderWindow::render(SDL_Texture* p_tex) {
    SDL_Rect src;
    src.x = 0;
    src.y = 0;
    src.w = 16;
    src.h = 16;

    SDL_Rect dst;
    dst.x = 600;
    dst.y = 400;
    dst.w = 16 * 8;
    dst.h = 16 * 8;

    SDL_RenderCopy(renderer, p_tex, &src, &dst);
}

void RenderWindow::display() {
    SDL_RenderPresent(renderer);
}