#pragma once
#include <SDL2-w64\include\SDL2\SDL.h>
#include <SDL2-w64\include\SDL2\SDL_image.h>
#include <iostream>

using namespace std;

class RenderWindow {
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;

    public:
        RenderWindow(const char* p_title, int p_w, int p_h);

        ~RenderWindow();

        SDL_Texture* load_texture(const char* p_filePath);

        void clear();
        void render(SDL_Texture* p_tex);
        void display();
};