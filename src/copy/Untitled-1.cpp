#include <glew-2.1.0\include\GL\glew.h>
#include <GL\gl.h>
#include <GL\glu.h>
#include <SDL2-w64\include\SDL2\SDL.h>
#include <SDL2-w64\include\SDL2\SDL_image.h>
#include <SDL2-w64\include\SDL2\SDL_opengl.h>
#include <iostream>
#include <cmath>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const char *vertexShaderSource = R"""(#version 460 core
layout (location = 0) in vec3 aPos;

void main()
{
   gl_Position = vec4(aPos, 1.0f);
})""";

const char* fragmentShaderSource = R"""(#version 460 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0f, 1.0f, 0.5f, 1.0f);
})""";

struct CMD {
    SDL_Window* window;
    int width;
    int height;
    std::vector<double> view_position = {0, 0};
    float scale = 1;

    CMD(SDL_Window* p_window, int p_width, int p_height) : window(p_window), width(p_width), height(p_height) {};
};

int main(int argc, char* argv[]) {
    int width = 1200, height = 800;

    glewExperimental = GL_TRUE;

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Game Window", 0, 20, width, height, SDL_WINDOW_OPENGL);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    if(window == nullptr) {
        std::cout << "SDL_CreateWindow error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    CMD cmd(window, width, height);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    glewInit();
    
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -0.5f, 0.0f,
        1.0f,  1.0f, 0.0f
    };

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment_shader);

    GLuint shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int width0, height0, num_channels0;
    unsigned char* data = stbi_load("file123.png", &width0, &height0, &num_channels0, 0);

    GLuint texture0;
    glGenTextures(1, &texture0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    SDL_Event event;
    bool game_running = true;

    bool left_mouse_button_pressed = false;
    
    while(game_running) {
        while (SDL_PollEvent(&event)){
	    //If user closes the window
	        if (event.type == SDL_QUIT) game_running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) left_mouse_button_pressed = true;
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) left_mouse_button_pressed = false;
            if (event.type == SDL_MOUSEMOTION && left_mouse_button_pressed) {
                cmd.view_position[0] -= event.motion.xrel / cmd.scale;
                cmd.view_position[1] -= event.motion.yrel / cmd.scale;
            }
            if (event.type == SDL_MOUSEWHEEL) cmd.scale *= pow(1.2, event.wheel.y);
        }

        //glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shader_program);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        SDL_GL_SwapWindow(window);
        
        /*SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Surface* surface = SDL_CreateRGBSurface(0, round(100*cmd.scale), round(100*cmd.scale), 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        p_gen.assign_colors(surface);
        SDL_LockSurface(surface);
        p_gen.draw_planet(surface);
        SDL_UnlockSurface(surface);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect;
        rect.w = round(100*cmd.scale * 2);
        rect.h = round(100*cmd.scale * 2);
        rect.x = round((cmd.width - rect.w) / 2) - cmd.view_position[0] * cmd.scale;
        rect.y = round((cmd.height - rect.h) / 2) - cmd.view_position[1] * cmd.scale;
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        SDL_RenderPresent(renderer);*/
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    //glDeleteTextures(1, &texture0);

    return 0;
}