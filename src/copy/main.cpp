#include <SDL2-w64\include\SDL2\SDL.h>
#include <SDL2-w64\include\SDL2\SDL_image.h>
#include <iostream>
#include <cmath>
#include <vector>

#include "gravsim.hpp"
#include "Noisefactory.hpp"

template <typename T>
T signum(T input) {
    if(input > 0) return 1;
    if(input < 0) return -1;
    return 0;
}

void normalize(std::vector<double>* input_vector) {
    double magnitude = sqrt((*input_vector)[0] * (*input_vector)[0] + (*input_vector)[1] * (*input_vector)[1]);
    if(magnitude != 0) {
        (*input_vector)[0] /= magnitude;
        std::cout << (*input_vector)[0] << std::endl;
        (*input_vector)[1] /= magnitude;
    }
}

void move_towards(std::vector<double>* modify_vector, std::vector<double>* input_vector, double speed) {
    (*modify_vector)[0] += speed * signum((*input_vector)[0] - (*modify_vector)[0]);
    (*modify_vector)[1] += speed * signum((*input_vector)[1] - (*modify_vector)[1]);
}

void add_vectors(std::vector<double>* vector, std::vector<double>* addend) {
    (*vector)[0] += (*addend)[0];
    (*vector)[1] += (*addend)[1];
}

struct Circle {
    int x, y;
    double radius;

    Circle(int x_, int y_, double radius_) : x(x_), y(y_), radius(radius_) {};
};

struct CMD {
    SDL_Renderer* renderer;
    int width;
    int height;
    std::vector<double> center_vector = {0, 0};
    float scale = 1;

    VoronoiTerrainMapGenerator v_gen;

    CMD(SDL_Renderer* renderer_, int width_, int height_) : renderer(renderer_), width(width_), height(height_) {
        v_gen.initialise_terrain_map(0.693646, 100, 100, 45, 45, 3, 3, 2, 1, 3, 3, 1, 1, 3, 3, 1, 1);
    };

    void draw_circle(Circle cir){
        int scaled_radius = round(cir.radius * scale);
        SDL_Surface* circle_image = SDL_CreateRGBSurface(0, scaled_radius * 2, scaled_radius * 2, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
        SDL_LockSurface(circle_image);
        Uint32* circle_image_pixels = (Uint32*)circle_image->pixels;
        //SDL_Texture* circle_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, radius_ * 2, radius_ * 2);
        //SDL_LockTexture(circle_texture, NULL, );
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        double width_at_height;
        Uint32 color_ = SDL_MapRGBA(circle_image->format, 255, 0, 0, 255);

        v_gen.generate(circle_image);
        /*for(int y = 0; y < scaled_radius * 2; y++){
            width_at_height = sqrt((long long)scaled_radius * scaled_radius - (y + 0.5 - scaled_radius) * (y + 0.5 - scaled_radius));
            for(int x = round(scaled_radius - width_at_height); x < round(scaled_radius + width_at_height); x++) {
                circle_image_pixels[x + y * circle_image->w] = color_;
            }
            //SDL_RenderDrawLine(renderer, clamp(center_x - width_at_height, -1, width), y, clamp(center_x + width_at_height, -1, width), y);
        }
        SDL_UnlockSurface(circle_image);*/
        SDL_Texture* circle_texture = SDL_CreateTextureFromSurface(renderer, circle_image);
        //SDL_UnlockTexture(circle_texture);
        SDL_Rect dstrect;
        dstrect.w = scaled_radius * 2 * 2;
        dstrect.h = scaled_radius * 2 * 2;
        dstrect.x = scale * (cir.x - cir.radius - center_vector[0]) + width / 2;
        dstrect.y = scale * (cir.y - cir.radius - center_vector[1]) + height / 2;
        SDL_RenderCopy(renderer, circle_texture, NULL, &dstrect);
        //SDL_RenderDrawPoint(renderer, center_x, center_y);
    }    
};

int main(int argc, char* argv[]) {
    int width = 1200, height = 800;

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Game Window", 0, 20, width, height, 0);
    if(window == nullptr) {
        std::cout << "SDL_CreateWindow error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == nullptr) {
        SDL_DestroyWindow(window);
        std::cout << "SDL_CreateRenderer error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Event event;
    bool game_running = true;
    CMD cmd(renderer, width, height);

    bool left_mouse_button_pressed = false;
    
    Circle cir(0, 0, 100);
    
    while(game_running) {
        while (SDL_PollEvent(&event)){
	    //If user closes the window
	        if (event.type == SDL_QUIT) game_running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) left_mouse_button_pressed = true;
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) left_mouse_button_pressed = false;
            if (event.type == SDL_MOUSEMOTION && left_mouse_button_pressed) {
                cmd.center_vector[0] -= event.motion.xrel / cmd.scale;
                cmd.center_vector[1] -= event.motion.yrel / cmd.scale;
            }
            if (event.type == SDL_MOUSEWHEEL) cmd.scale *= pow(1.2, event.wheel.y);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        cmd.draw_circle(cir);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}