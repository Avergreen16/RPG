#include <SFML-2.5.1\include\SFML\Graphics.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <map>
#include <vector>
#include <iostream>

double clamp(double input, double min, double max);

double dotproduct(std::vector<double> v1, std::vector<double> v2);

double interpolate(double x1, double x2, int position);

double interpolate2(double x1, double x2, int coordinate, int scale);

class RNG {
    public:
        double seed;
        double factor;
        double rng_number;

        RNG() {}
        RNG(double p_seed, double p_factor);

        double retrieve();
};

struct PerlinNoiseFactory {
    double seed;
    int main_vector_width;
    int main_vector_height;
    int octaves;
    double persistance;
    bool loop_x;
    bool loop_y;
    double maxvalue;
    RNG rng;

    std::map<int, std::vector<double>> vector_map;

    void set_parameters(double p_seed, double p_factor, int p_main_vector_width, int p_main_vector_height, int p_octaves, double p_persistance, bool p_loop_x, bool p_loop_y);

    void generate_vectors();

    double pullvalue(int x, int y, int width, int height);
};

std::vector<std::vector<std::vector<int>>> biome_map_terra = {
    {{2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2},
    {4, 4, 4, 4, 4, 4, 4, 4}},

    {{2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2},
    {4, 4, 4, 4, 4, 4, 4, 4}},

    {{2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2},
    {4, 4, 4, 4, 4, 4, 4, 4}},

    {{2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2},
    {4, 4, 4, 4, 4, 4, 4, 4}},

    {{10, 10, 11, 11, 5, 12, 12, 9},
    {8, 8, 6, 6, 5, 5, 5, 9},
    {8, 8, 6, 7, 7, 7, 5, 9},
    {14, 14, 13, 13, 13, 13, 13, 13}},

    {{10, 10, 11, 11, 5, 12, 12, 9},
    {8, 8, 6, 6, 5, 5, 5, 9},
    {8, 8, 6, 7, 7, 7, 5, 9},
    {14, 14, 13, 13, 13, 13, 13, 13}},

    {{15, 15, 11, 11, 5, 12, 12, 9},
    {15, 15, 6, 6, 5, 5, 5, 9},
    {15, 15, 6, 7, 7, 7, 5, 9},
    {14, 14, 13, 13, 13, 13, 13, 13}},

    {{16, 16, 16, 16, 18, 18, 18, 18},
    {16, 16, 16, 16, 16, 16, 16, 16},
    {16, 16, 16, 16, 16, 16, 17, 17},
    {16, 16, 17, 17, 17, 17, 17, 17}}
};

struct TerrainGenerator {
    PerlinNoiseFactory pnf1;
    PerlinNoiseFactory pnf2;
    PerlinNoiseFactory pnf3;
    
    std::map<int, sf::Color> biome_lookup_table = {
        {1, sf::Color(10, 10, 160, 255)}, // deep sea
        {2, sf::Color(30, 30, 180, 255)}, // sea
        {3, sf::Color(50, 50, 150, 255)}, // reef
        {4, sf::Color(150, 180, 220, 255)}, // frozen sea
        {5, sf::Color(10, 180, 10, 255)}, // forest
        {6, sf::Color(120, 200, 120, 255)}, // grassland
        {7, sf::Color(20, 100, 10, 255)}, // boreal forest
        {8, sf::Color(220, 200, 50, 255)}, // desert
        {9, sf::Color(50, 145, 80, 255)}, // swamp
        {10, sf::Color(200, 140, 20, 255)}, // scorched desert
        {11, sf::Color(180, 200, 20, 255)}, // shrubland
        {12, sf::Color(10, 250, 50, 255)}, // rainforest
        {13, sf::Color(240, 240, 250, 255)}, // ice cap
        {14, sf::Color(100, 100, 20, 255)}, // tundra
        {15, sf::Color(170, 130, 20, 255)}, // canyons
        {16, sf::Color(150, 150, 150, 255)}, // mountains
        {17, sf::Color(215, 215, 250, 255)}, // glacier
        {18, sf::Color(150, 190, 150, 255)} // mossy mountains
        //{, sf::Color(60, 60, 200, 255)}, // river
    };

    void assign_parameters(double p_seed, int p_width1, int p_height1, int p_octaves1, int p_persistance1, int p_width2, int p_height2, int p_octaves2, int p_persistance2, int p_width3, int p_height3, int p_octaves3, int p_persistance3);

    int get_biome(double v1, double v2, double v3);

    sf::Image generate_map(int width, int height);
};




int main() {
    TerrainGenerator tgen;
    tgen.assign_parameters(0.56752375, 6, 3, 3, 2, 10, 5, 1, 1, 10, 5, 1, 1);
    sf::Image image = tgen.generate_map(600, 300);
    image.saveToFile("file_1.png");

    return 0;
}




double clamp(double input, double min, double max) {
    if (input <= min) {
        return min;
    } else if (input >= max) {
        return max;
    } else {
        return input;
    }
}

double dotproduct(std::vector<double> v1, std::vector<double> v2) {
    return v1[0] * v2[0] + v1[1] * v2[1];
}

double interpolate(double x1, double x2, double position) {
    double returnvalue = (x2 - x1) * (6 * pow(position, 5) - 15 * pow(position, 4) + 10 * pow(position, 3)) + x1;
    return returnvalue;
}

double interpolate2(double x1, double x2, int coordinate, int scale) {
    double scaled_coordinate = (double)(coordinate % scale) / scale;
    double returnvalue = (x2 - x1) * (6 * pow(scaled_coordinate, 5) - 15 * pow(scaled_coordinate, 4) + 10 * pow(scaled_coordinate, 3)) + x1;
    return returnvalue;
}

RNG::RNG(double p_seed, double p_factor) {
    factor = p_factor;
    rng_number = p_seed;
}

double RNG::retrieve() {
    rng_number = rng_number * factor;
    rng_number -= floor(rng_number);
    return rng_number;
}

void PerlinNoiseFactory::set_parameters(double p_seed, double p_factor, int p_main_vector_width, int p_main_vector_height, int p_octaves, double p_persistance, bool p_loop_x, bool p_loop_y) {
    seed = p_seed;
    main_vector_width = p_main_vector_width;
    main_vector_height = p_main_vector_height;
    octaves = p_octaves;
    persistance = p_persistance;
    loop_x = p_loop_x;
    loop_y = p_loop_y;
    rng = RNG(p_seed, p_factor);

    maxvalue = 0;
    for(int i = 0; i < octaves; i++) {
        maxvalue += 1 * pow(persistance, -i);
    }
}

void PerlinNoiseFactory::generate_vectors() {
    int array_width = main_vector_width * pow(2, octaves);
    int array_height = main_vector_height * pow(2, octaves);
    for(int k = 0; k < array_width * array_height; k++) {
        int vector_key = k;
        double theta = rng.retrieve() * 2 * M_PI;
        std::vector<double> insertvector = {std::cos(theta), std::sin(theta)};
        vector_map.insert({vector_key, insertvector});   
    }
}

double PerlinNoiseFactory::pullvalue(int x, int y, int p_width, int p_height) {
    double returnvalue = 0;
    double sqrt_half = std::sqrt(0.5);
    for(int i = 0; i < octaves; i++) {
        //updated scale
        int i_power = pow(2, i);
        double updated_xscale = ((double)p_width / main_vector_width) / i_power;
        double updated_yscale = ((double)p_height / main_vector_height) / i_power;

        // stores the point's 4 dot products for interpolation
        double dotproducts[4];

        // grabs the coordinate value of the top left vector of the 4 surrounding vectors in the "vector array", each vector gets an
        // integer coordinate. These values are used to determine the vector's key
        int topleft_x = floor((double)x / updated_xscale);
        int topleft_y = floor((double)y / updated_yscale);
        
        for(int j = 0; j < 4; j++) {
            // vector_x and vector_y are the x and y integer coordinates of the active vector, determined by j, in the "vector array"
            int vector_x = topleft_x + j % 2;
            int vector_y = topleft_y + floor(j / 2);

            // these are the true coordinates of the active vector so that the distance vector between the 
            // active vector's origin and the point's x and y can be calculated
            int pos_value_x = vector_x * updated_xscale;
            int pos_value_y = vector_y * updated_yscale;

            // these loops move vector_x and vector_y into the width and height range so the pattern can loop.
            // it only runs if the loop variables are true
            // it's separate for x and y so that noise that loops in only one axis is possible
            if(loop_x) {
                while(vector_x >= main_vector_width * i_power) {
                    vector_x -= main_vector_width * i_power;
                }
                while(vector_x < 0) {
                    vector_x += main_vector_width * i_power;
                }
            }
            if(loop_y) {
                while(vector_y >= main_vector_height * i_power) {
                    vector_y -= main_vector_height * i_power;
                }
                while(vector_y < 0) {
                    vector_y += main_vector_height * i_power;
                }
            }

            // creates vector key for lookup in the vector map
            int vector_key = vector_x + vector_y * main_vector_width * pow(2, i);

            // will create a vector if there isn't already one with the same key, and will store it in the 
            // vector map with that key. The key corresponds the the x and y position of the vector in the "vector array"
            /*if(vector_map.find(vector_key) == vector_map.end()) {
                double theta = rng.retrieve() * 2 * M_PI;
                std::vector<double> insertvector = {std::cos(theta), std::sin(theta)};
                vector_map.insert({vector_key, insertvector});
            }*/

            // vector of active point, current active point is one of the 4 closest vectors, this variable is assigned each of the
            // 4 vectors as the j loop runs.
            std::vector<double> active_vector = vector_map[vector_key];
            // distance vector between true position of active vector's origin and x and y coordinate
            std::vector<double> coordinate_distance_vector = {((double)x - pos_value_x) / updated_xscale, ((double)y - pos_value_y) / updated_yscale};

            dotproducts[j] = dotproduct(active_vector, coordinate_distance_vector);
        }

        // interpolates the dot products bilineraly and returns the result
        //(double)(coordinate % scale) / scale
        double x1 = interpolate(dotproducts[0], dotproducts[1], fmod((double)x, updated_xscale) / updated_xscale);
        double x2 = interpolate(dotproducts[2], dotproducts[3], fmod((double)x, updated_xscale) / updated_xscale);
        returnvalue += (interpolate(x1, x2 , fmod((double)y, updated_yscale) / updated_yscale) + sqrt_half) / (sqrt_half * 2) * pow(persistance, -i);
    }
    return returnvalue / maxvalue;
}

void TerrainGenerator::assign_parameters(double p_seed, int p_width1, int p_height1, int p_octaves1, int p_persistance1, int p_width2, int p_height2, int p_octaves2, int p_persistance2, int p_width3, int p_height3, int p_octaves3, int p_persistance3) {
    pnf1.set_parameters(p_seed, p_seed * 45.18754874623, p_width1, p_height1, p_octaves1, p_persistance1, true, true);
    pnf2.set_parameters(p_seed, p_seed * 21.1324254354, p_width2, p_height2, p_octaves2, p_persistance2, true, true);
    pnf3.set_parameters(p_seed, p_seed * 57.62634832648, p_width3, p_height3, p_octaves3, p_persistance3, true, true);
}

sf::Image TerrainGenerator::generate_map(int width, int height) {
    pnf1.generate_vectors();
    pnf2.generate_vectors();
    pnf3.generate_vectors();

    sf::Image return_image;
    return_image.create(width, height);
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int v1 = floor(pnf1.pullvalue(x, y, width, height) * 8);
            int v2 = floor(pnf2.pullvalue(x, y, width, height) * 4);
            int v3 = floor(pnf3.pullvalue(x, y, width, height) * 8);
            return_image.setPixel(x, y, biome_lookup_table.at(biome_map_terra[v1][v2][v3]));
        }
    }
    return return_image;
}