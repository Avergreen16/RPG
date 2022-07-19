#pragma once
#include <SDL2-w64\include\SDL2\SDL.h>
#include <SDL2-w64\include\SDL2\SDL_image.h>

class Entity {
    private:
        double x, y;
        SDL_Rect currentFrame;
        SDL_Texture* tex;
    public:
        Entity(double p_x, double p_y, SDL_Texture* p_tex);
};