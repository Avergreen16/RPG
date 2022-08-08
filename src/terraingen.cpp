#define _USE_MATH_DEFINES
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>

#include <cmath>
#include <iostream>
#include <ctime>
#include <array>

#include "worldgen.cpp"

std::vector<unsigned char> data;

struct Mapgen {
    PerlinNoiseFactory pfac1;
    PerlinNoiseFactory pfac2;
    PerlinNoiseFactory pfac3;
    bool has_craters = false;
    std::vector<std::array<double, 3>> crater_v;
    RNG rng;

    void construct_3(double p_seed, int p_width1, int p_height1, int p_octaves1, double p_persistance1, int p_width2, int p_height2, int p_octaves2, double p_persistance2, int p_width3, int p_height3, int p_octaves3, double p_persistance3) {
        pfac1.set_parameters(p_seed, p_seed * 45.18754874623, p_width1, p_height1, p_octaves1, p_persistance1, 0.15, 0.5, true, false);
        pfac2.set_parameters(p_seed, p_seed * 21.1324254354, p_width2, p_height2, p_octaves2, p_persistance2, 0.2, 0.5, true, false);
        pfac3.set_parameters(p_seed, p_seed * 57.62634832648, p_width3, p_height3, p_octaves3, p_persistance3, 0.15, 0.5, true, false);
        pfac1.generate_vectors();
        pfac2.generate_vectors();
        pfac3.generate_vectors();
        rng = {p_seed, p_seed * 58.1397294};
    }

    void construct_1(double p_seed, int p_width1, int p_height1, int p_octaves1, double p_persistance1) {
        pfac1.set_parameters(p_seed, p_seed * 45.18754874623, p_width1, p_height1, p_octaves1, p_persistance1, 0.15, 0.5, true, true);
        pfac1.generate_vectors();
        rng = {p_seed, p_seed * 58.1397294};
    }

    void generate_craters(int crater_num, std::array<int, 2> map_size, int min_crater_radius, int max_crater_radius) {
        has_craters = true;
        crater_v.clear();
        for(int i = 0; i < crater_num; i++) {
            std::array<double, 3> crater = {map_size[0] * rng(), map_size[1] * rng(), (max_crater_radius - min_crater_radius) * rng() + min_crater_radius};
            if(crater[0] - crater[2] < 0 || crater[0] + crater[2] >= map_size[0] || crater[1] - crater[2] < 0 || crater[1] + crater[2] >= map_size[1]) i--;
            else {
                crater_v.push_back(crater);
            }
        }
    }

    std::array<unsigned char, 4> retrieve_r3(std::array<int, 2> location, std::array<int, 2> map_size, unsigned char* source_image) {
        double elevation = pfac1.retrieve(location[0], location[1], map_size[0], map_size[1]) * 12;
        double temperature = clamp((pfac2.retrieve(location[0], location[1], map_size[0], map_size[1]) - 0.5) * 3 + (0.5 - std::abs(double(location[1]) / map_size[1] - 0.5)) * 24, 0, 11.999);
        double humidity = pfac3.retrieve(location[0], location[1], map_size[0], map_size[1]) * 12;

        std::array<unsigned char, 4> biome = get_pixel_array(source_image, {floor(elevation) * 12 + floor(humidity), floor(temperature)}, 144);
        return biome;
    }

    std::array<unsigned char, 4> retrieve_g1(std::array<int, 2> location, std::array<int, 2> map_size, unsigned char* source_image) {
        double y = clamp((pfac1.retrieve(location[0], location[1], map_size[0], map_size[1]) - 0.5) * 3 + (double(location[1]) / map_size[1]) * 24, 0, 23.999);

        std::array<unsigned char, 4> biome = get_pixel_array(source_image, {0, floor(y)}, 1);
        return biome;
    }
    
    std::array<unsigned char, 4> retrieve_a1(std::array<int, 2> location, std::array<int, 2> map_size, double factor, unsigned char* source_image) {
        double height = clamp(((pfac1.retrieve(location[0], location[1], map_size[0], map_size[1]) - 0.5) * factor + (1 - hypot(double(location[0]) / map_size[0] - 0.5, double(location[1]) / map_size[1] - 0.5) * 2)) * 12, 0, 11.999);

        std::array<unsigned char, 4> biome = get_pixel_array(source_image, {floor(height), 0}, 12);
        if(has_craters && biome[3] == 0xFF) {
            for(std::array<double, 3> crater : crater_v) {
                if(hypot(location[0] - crater[0], location[1] - crater[1]) < crater[2]) {
                    biome = get_pixel_array(source_image, {12, 0}, 12);
                    break;
                }
            }
        }
        return biome;
    }

    std::array<unsigned char, 4> retrieve_x1(std::array<int, 2> location, std::array<int, 2> map_size, double factor, unsigned char* source_image, double drop_off_start, double drop_off_end) {
        double density = clamp((pfac1.retrieve(location[0], location[1], map_size[0], map_size[1])) * std::min((-1 / (drop_off_end - drop_off_start)) * (hypot(double(location[0]) / map_size[0] - 0.5, double(location[1]) / map_size[1] - 0.5) * 2 - drop_off_end), 1.0) * 12, 0, 11.999);

        std::array<unsigned char, 4> biome = get_pixel_array(source_image, {floor(density), 0}, 12);
        return biome;
    }
};

const char* compute_shader_source = R"""(
#version 460 core;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D screen;

void main() {
    
}
)""";

int main() {
    const uint width = 1024, height = 512;

    if(!glfwInit()) {
        std::cout << "glfw failure (init)\n";
        return 1;
    }
    gladLoadGL();

    uint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, &compute_shader_source, NULL);
    glCompileShader(compute_shader);

    uint compute_program = glCreateProgram();
    glAttachShader(compute_program, compute_shader);
    glLinkProgram(compute_program);

    glUseProgram(compute_program);
    glDispatchCompute(width, height, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}