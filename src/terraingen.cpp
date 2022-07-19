#define _USE_MATH_DEFINES
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

int main() {
    //sf::Clock clock;
    stbi_flip_vertically_on_write(true);
    RNG rng;

    double seed;
    int w, h, type, count = 1;
    double p, f;
    int num_craters = 0, min_rad, max_rad;
    double drop_off_start, drop_off_end;
    std::string source_filepath;
    std::string input_filepath;

    std::cout << "seed (number between 0 and 1): ";
    std::cin >> seed;
    std::cout << "image width: ";
    std::cin >> w;
    std::cout << "image height: ";
    std::cin >> h;
    std::cout << "map type: ";
    std::cin >> type;
    if(type == 1) {
        std::cout << "persistance value: ";
        std::cin >> p;
    } else if(type == 2) {
        std::cout << "persistance value: ";
        std::cin >> p;
        std::cout << "factor value: ";
        std::cin >> f;
        std::cout << "number of maps: ";
        std::cin >> count;
        std::cout << "number of craters: ";
        std::cin >> num_craters;
        std::cout << "min crater radius: ";
        std::cin >> min_rad;
        std::cout << "max crater radius: ";
        std::cin >> max_rad;
        if(count != 1) rng = {seed, 76.9812749};
    } else if(type == 3) {
        std::cout << "value drop off start: ";
        std::cin >> drop_off_start;
        std::cout << "value drop off end: ";
        std::cin >> drop_off_end;
    }
    std::cout << "source image filepath/file name: ";
    std::cin >> source_filepath;
    std::cout << "filepath/file name to save: ";
    std::cin >> input_filepath;

    Mapgen mgen;
    if(type == 0) {
        mgen.construct_3(seed, 6, 3, 7, 1.5, 4, 4, 3, 1.5, 4, 4, 4, 1.3);
    } else if(type == 1) {
        mgen.construct_1(seed, 4, 4, 3, p);
        stbi_set_flip_vertically_on_load(true);
    } else if(type == 2) {
        mgen.construct_1(seed, 4, 4, 3, p);
    } else if(type == 3) {
        mgen.construct_1(seed, 4, 4, 3, 1.5);
    }



    for(int t = 0; t < count; t++) {
        clock_t time_start = clock();
        int width, height, n_channels;
        unsigned char* source_img = stbi_load(source_filepath.c_str(), &width, &height, &n_channels, 4);
        std::string filepath;
        if(num_craters != 0) mgen.generate_craters(num_craters, {w, h}, min_rad, max_rad);
        if(count != 1) {
            if(t != 0) data.clear();
            if(type == 2) {
                mgen.construct_1(rng(), 4, 4, 3, p);
                filepath = input_filepath + std::to_string(t) + ".png";
            }
        } else {
            filepath = input_filepath;
        }
        
        if(type == 0) {
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                    std::array<unsigned char, 4> color = mgen.retrieve_r3({x, y}, {w, h}, source_img);
                    data.push_back(color[0]);
                    data.push_back(color[1]);
                    data.push_back(color[2]);
                    data.push_back(color[3]);
                }
            }
        } else if(type == 1) {
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                    std::array<unsigned char, 4> color = mgen.retrieve_g1({x, y}, {w, h}, source_img);
                    data.push_back(color[0]);
                    data.push_back(color[1]);
                    data.push_back(color[2]);
                    data.push_back(color[3]);
                }
            }
        } else if(type == 2) {
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                    std::array<unsigned char, 4> color = mgen.retrieve_a1({x, y}, {w, h}, f, source_img);
                    data.push_back(color[0]);
                    data.push_back(color[1]);
                    data.push_back(color[2]);
                    data.push_back(color[3]);
                }
            }
        } else if(type == 3) {
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                    std::array<unsigned char, 4> color = mgen.retrieve_x1({x, y}, {w, h}, f, source_img, drop_off_start, drop_off_end);
                    data.push_back(color[0]);
                    data.push_back(color[1]);
                    data.push_back(color[2]);
                    data.push_back(color[3]);
                }
            }
        }
        
        stbi_write_png(filepath.c_str(), w, h, 4, data.data(), 0);
        clock_t time_end = clock();

        std::cout << "File saved as " << filepath << "\n";
        std::cout << "The map took " << double(time_end - time_start) / 1000 << " seconds to generate.\n";
    }
}