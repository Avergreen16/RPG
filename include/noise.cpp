#define _USE_MATH_DEFINES
#include <iostream>
#include <map>
#include <cmath>
#include <string>
#include <ctime>
#include <SFML-2.5.1\include\SFML\Graphics.hpp>

struct gen {
    std::vector<std::vector<std::vector<int>>> terra_biome_vector = {
        {{1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
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

        {{10, 8, 11, 11, 5, 12, 12, 9},
        {8, 8, 11, 6, 5, 5, 5, 9},
        {8, 8, 6, 7, 7, 7, 5, 9},
        {14, 14, 13, 13, 13, 13, 13, 13}},

        {{15, 8, 11, 11, 5,  5,  5,  12},
        {15,  8, 11, 6,  7,  7,  7,  5},
        {15,  8, 6,  7,  7,  7,  7,  5},
        {14,  14, 13, 13, 13, 13, 13, 13}},

        {{16, 16, 16, 16, 18, 18, 18, 18},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 17},
        {16, 16, 17, 17, 17, 17, 17, 17}}
    };

    std::vector<std::vector<int>> ares_biome_vector = {
        {20, 20, 20, 20, 20, 20, 20, 20},
        {25, 19, 19, 19, 19, 23, 23, 23},
        {21, 21, 21, 21, 21, 23, 23, 23},
        {22, 22, 22, 22, 24, 24, 24, 24}
    };

    std::vector<std::vector<int>> barren_biome_vector = {
        {26, 26, 26, 29},
        {26, 26, 26, 29},
        {27, 27, 27, 29},
        {28, 28, 28, 29}
    };

    std::vector<std::vector<int>> dry_barren_biome_vector = {
        {26, 26, 26, 26},
        {26, 26, 26, 26},
        {27, 27, 27, 27},
        {28, 28, 28, 28}
    };

    std::vector<std::vector<int>> wet_barren_biome_vector = {
        {32, 32, 32, 32},
        {26, 26, 26, 29},
        {27, 27, 27, 29},
        {28, 28, 28, 29}
    };

    std::vector<std::vector<int>> icy_barren_biome_vector = {
        {30, 30, 30, 30},
        {26, 26, 26, 29},
        {27, 27, 27, 29},
        {28, 28, 28, 29}
    };

    std::vector<std::vector<int>> scorched_barren_biome_vector = {
        {31, 31, 26, 26},
        {31, 26, 26, 26},
        {31, 27, 27, 27},
        {31, 28, 28, 28}
    };
};

struct gen_ares {
    
};

struct gen_barren {
    std::vector<std::vector<int>> biome_vector = {
        {26, 26, 26, 29},
        {26, 26, 26, 29},
        {27, 27, 27, 29},
        {28, 28, 28, 29}
    };
};

/*planet types:
terra
ares (icy barren)

barren <- scorched, dry, wet, icy -> eyeball 10
water
icy <- saline <- hot, cold -> eyeball 4

storm <- hot, cold -> eyeball 6

giant <- gas, ice, helium 3
*/

float sd1 = 0.148, sd21 = 0.111, sd22 = 0.114, sd31 = 0.096, sd32 = 0.102, sd41 = 0.096, sd42 = 0.099;

double dotproduct(std::vector<double> v1, std::vector<double> v2);

double interpolate(double x1, double x2, int position);

double pyth(double x, double y);

int sign(double input) {
    if(input > 0) return 1;
    if(input < 0) return -1;
    return 0;
}

double clamp(double input, double min, double max) {
    if(input >= max) return max;
    if(input <= min) return min;
    return input;
}

double normalCDF(double input, double sd, double mean) {
    double value = (input - mean) / sd;
    return 0.5 * erfc(-value * M_SQRT1_2);
}

struct RNG {
    double factor;
    double product;

    void set_seed(double p_seed, double p_factor);

    double retrieve();
};

struct PerlinNoiseFactory {
    int main_vector_width;
    int main_vector_height;
    int octaves;
    double persistance;
    bool loop_x;
    bool loop_y;
    double maxvalue;
    RNG rng;

    double sd;
    double mean;

    std::vector<std::vector<double>> vector_container;

    void set_parameters(double p_seed, double p_factor, int p_main_vector_width, int p_main_vector_height, int p_octaves, double p_persistance, double p_sd, double p_mean, bool p_loop_x, bool p_loop_y);

    void generate_vectors();

    double retrieve(int x, int y, int width, int height);
};

struct VoronoiBiomePoint {
    double x;
    double y;
    int biome_id = -1;

    VoronoiBiomePoint(double p_x, double p_y) : x(p_x), y(p_y) {};
};

struct VoronoiFactory {
    double seed;
    int point_container_width;
    int point_container_height;
    bool loop_x;
    bool loop_y;
    RNG rng;

    std::vector<VoronoiBiomePoint> point_container;

    void set_parameters(double p_seed, double p_factor, int p_point_container_width, int p_point_container_height, bool p_loop_x, bool p_loop_y);

    void generate_points();

    int retrieve_closest_point_key(double x, double y, int width, int height);
};

struct PlanetBiomeGenerator {
    VoronoiFactory vf;
    PerlinNoiseFactory pnf1;
    PerlinNoiseFactory pnf2;
    PerlinNoiseFactory pnf3;
    gen Gen;
    
    std::map<int, sf::Color> biome_lookup_table = {
        //Terra
        {1, sf::Color(0x1418ADFF)}, // deep sea
        {2, sf::Color(0x191FD3FF)}, // sea
        {3, sf::Color(0x1942D3FF)}, // reef
        {4, sf::Color(0x90B1DBFF)}, // frozen sea
        {5, sf::Color(0x008944FF)}, // forest
        {6, sf::Color(0x8BCE50FF)}, // grassland
        {7, sf::Color(0x287049FF)}, // boreal forest
        {8, sf::Color(0xE2DC98FF)}, // desert
        {9, sf::Color(0x1B541FFF)}, // swamp
        {10, sf::Color(0xE59647FF)}, // scorched desert
        {11, sf::Color(0xC8D16AFF)}, // shrubland
        {12, sf::Color(0x00B539FF)}, // rainforest
        {13, sf::Color(0xF4ECFFFF)}, // ice cap
        {14, sf::Color(0x9E8F62FF)}, // tundra
        {15, sf::Color(0xC6833FFF)}, // canyons
        {16, sf::Color(0x706A6AFF)}, // mountains
        {17, sf::Color(0xAED3D3FF)}, // glacier
        {18, sf::Color(0x6A7C6AFF)}, // mossy mountains
        //{, sf::Color(60, 60, 200, 255)}, // river
        //Ares
        {19, sf::Color(0xB59780FF)}, // basin
        {20, sf::Color(0xC2D9E5FF)}, // frozen sea
        {21, sf::Color(0x897761FF)}, // highlands
        {22, sf::Color(0x706558FF)}, // cliffs
        {23, sf::Color(0xF7EDEEFF)}, // ice cap
        {24, sf::Color(0xC4AAADFF)}, // polar cliffs
        {25, sf::Color(0xAF7E98FF)}, // infested basin

        //barren planet
        {26, sf::Color(0xBFBFBFFF)}, // basin 
        {27, sf::Color(0x7F7F7FFF)}, // highlands
        {28, sf::Color(0x3F3F3FFF)}, // cliffs
        {29, sf::Color(0xFFFFFFFF)}, // ice cap
        {30, sf::Color(0x0F0F0FFF)}, // frozen sea
        {31, sf::Color(0x000000FF)}, // magma sea
        {32, sf::Color(0x1F1F1FFF)}, // sea
    };

    void set_parameters(double p_seed, int p_vwidth, int p_vheight, int p_width1, int p_height1, int p_octaves1, double p_persistance1, int p_width2, int p_height2, int p_octaves2, double p_persistance2, int p_width3, int p_height3, int p_octaves3, double p_persistance3);

    void generate_map(/*int width, int height*/);

    sf::Color get_biome(int x, int y, int width, int height) {
        int v1 = floor(pnf1.retrieve(x, y, width, height) * 7);
        int v2 = floor(clamp(((pnf2.retrieve(x, y, width, height) - 0.5) * 0.125 + std::abs((double)y / height - 0.5)) * 8, 0, 3));
        int v3 = floor(pnf3.retrieve(x, y, width, height) * 8);;

        return biome_lookup_table.at(Gen.terra_biome_vector[v1][v2][v3]);
    }
};

//-------------------------------------------------------------------------------

double dotproduct(std::vector<double> v1, std::vector<double> v2) {
    return v1[0] * v2[0] + v1[1] * v2[1];
}

double interpolate(double x1, double x2, double position) {
    double returnvalue = (x2 - x1) * (6 * pow(position, 5) - 15 * pow(position, 4) + 10 * pow(position, 3)) + x1;
    return returnvalue;
}

double pyth(double x, double y) {
    return std::sqrt(x * x + y * y);
}

void RNG::set_seed(double p_seed, double p_factor) {
    factor = p_factor;
    product = p_seed;
}

double RNG::retrieve() {
    product = product * factor;
    product -= floor(product);
    return product;
}

void PerlinNoiseFactory::set_parameters(double p_seed, double p_factor, int p_main_vector_width, int p_main_vector_height, int p_octaves, double p_persistance, double p_sd, double p_mean, bool p_loop_x, bool p_loop_y) {
    main_vector_width = p_main_vector_width;
    main_vector_height = p_main_vector_height;
    octaves = p_octaves;
    persistance = p_persistance;
    loop_x = p_loop_x;
    loop_y = p_loop_y;
    rng.set_seed(p_seed, p_factor);

    sd = p_sd;
    mean = p_mean;

    maxvalue = 0;
    for(int i = 0; i < p_octaves; i++) {
        maxvalue += 1 * pow(p_persistance, -i);
    }
}

void PerlinNoiseFactory::generate_vectors() {
    vector_container.clear();
    int array_width = main_vector_width * pow(2, octaves);
    int array_height = main_vector_height * pow(2, octaves);
    for(int k = 0; k < array_width * array_height; k++) {
        double theta = rng.retrieve() * 2 * M_PI;
        std::vector<double> insertvector = {std::cos(theta), std::sin(theta)};
        vector_container.push_back(insertvector);
    }
}

double PerlinNoiseFactory::retrieve(int x, int y, int p_width, int p_height) {
    double returnvalue = 0;
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
            /*if(vector_container.find(vector_key) == vector_container.end()) {
                double theta = rng.retrieve() * 2 * M_PI;
                std::vector<double> insertvector = {std::cos(theta), std::sin(theta)};
                vector_container.insert({vector_key, insertvector});
            }*/

            // vector of active point, current active point is one of the 4 closest vectors, this variable is assigned each of the
            // 4 vectors as the j loop runs.
            std::vector<double> active_vector = vector_container[vector_key];
            // distance vector between true position of active vector's origin and x and y coordinate
            std::vector<double> coordinate_distance_vector = {((double)x - pos_value_x) / updated_xscale, ((double)y - pos_value_y) / updated_yscale};

            dotproducts[j] = dotproduct(active_vector, coordinate_distance_vector);
        }

        // interpolates the dot products bilineraly and returns the result
        //(double)(coordinate % scale) / scale
        double x1 = interpolate(dotproducts[0], dotproducts[1], fmod((double)x, updated_xscale) / updated_xscale);
        double x2 = interpolate(dotproducts[2], dotproducts[3], fmod((double)x, updated_xscale) / updated_xscale);
        returnvalue += (interpolate(x1, x2 , fmod((double)y, updated_yscale) / updated_yscale) + M_SQRT1_2) / (M_SQRT1_2 * 2) * pow(persistance, -i);
    }
    return normalCDF(returnvalue / maxvalue, sd, mean);
}

void VoronoiFactory::set_parameters(double p_seed, double p_factor, int p_point_container_width, int p_point_container_height, bool p_loop_x, bool p_loop_y) {
    seed = p_seed;
    point_container_width = p_point_container_width;
    point_container_height = p_point_container_height;
    loop_x = p_loop_x;
    loop_y = p_loop_y;
    rng.set_seed(p_seed, p_factor);
}

void VoronoiFactory::generate_points() {
    double rect_x = (double)1 / point_container_width;
    double rect_y = (double)1 / point_container_height;
    
    for(int y = 0; y < point_container_height; y++) {
        for(int x = 0; x < point_container_width; x++) {
            double p_x = (rng.retrieve() + x) * rect_x;
            double p_y = (rng.retrieve() + y) * rect_y;
            point_container.push_back(VoronoiBiomePoint(p_x, p_y));
        }
    }
}

int VoronoiFactory::retrieve_closest_point_key(double x, double y, int width, int height) {
    int return_key;

    int center_key_x = floor((x / width) * point_container_width);
    int center_key_y = floor((y / height) * point_container_height);
    double distance = __DBL_MAX__;
    for(int ix = -2; ix <= 2; ix++) {
        for(int iy = -2; iy <= 2; iy++) {
            int x_translate = 0;
            int y_translate = 0;
            int active_key_x = center_key_x + ix;
            int active_key_y = center_key_y + iy;

            if(active_key_x >= point_container_width) {
                if(loop_x) {
                    active_key_x -= point_container_width;
                    x_translate = 1;
                } else {
                    continue;
                }
            } else if(active_key_x < 0) {
                if(loop_x) {
                    active_key_x += point_container_width;
                    x_translate = -1; 
                } else {
                    continue;
                }
            }

            if(active_key_y >= point_container_height) {
                if(loop_y) {
                    active_key_y -= point_container_height;
                    y_translate = 1;
                } else {
                    continue;
                }
            } else if(active_key_y < 0) {
                if(loop_y) {
                    active_key_y += point_container_height;
                    y_translate = -1; 
                } else {
                    continue;
                }
            }

            int active_key = active_key_y * point_container_width + active_key_x;
            VoronoiBiomePoint active_point = point_container[active_key];
            double active_distance = pyth((active_point.x + x_translate) * width - x, (active_point.y + y_translate) * height - y);
            if(active_distance < distance) {
                distance = active_distance;
                return_key = active_key;
            }
        }
    }

    return return_key;
}

void PlanetBiomeGenerator::set_parameters(double p_seed, int p_vwidth, int p_vheight, int p_width1, int p_height1, int p_octaves1, double p_persistance1, int p_width2, int p_height2, int p_octaves2, double p_persistance2, int p_width3, int p_height3, int p_octaves3, double p_persistance3) {
    pnf1.set_parameters(p_seed, p_seed * 45.18754874623, p_width1, p_height1, p_octaves1, p_persistance1, 0.15, 0.5, true, true);
    pnf2.set_parameters(p_seed, p_seed * 21.1324254354, p_width2, p_height2, p_octaves2, p_persistance2, 0.2, 0.5, true, true);
    pnf3.set_parameters(p_seed, p_seed * 57.62634832648, p_width3, p_height3, p_octaves3, p_persistance3, 0.2, 0.5, true, true);
}

void PlanetBiomeGenerator::generate_map(/*int width, int height*/) {
    //vf.generate_points();
    pnf1.generate_vectors();
    pnf2.generate_vectors();
    pnf3.generate_vectors();
    //gen Gen;

    /*sf::Image return_image;
    return_image.create(width, height);
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int point_key = vf.retrieve_closest_point_key(x, y, width, height);
            if(vf.point_container[point_key].biome_id == -1) {
                int v1 = floor(pnf1.retrieve(x, y, width, height) * 7);
                int v2 = floor(clamp(((pnf2.retrieve(x, y, width, height) - 0.5) * 0.125 + std::abs((double)y / height - 0.5)) * 8, 0, 3));
                int v3 = floor(pnf3.retrieve(x, y, width, height) * 8);
                vf.point_container[point_key].biome_id = Gen.terra_biome_vector[v1][v2][v3];
            }
            return_image.setPixel(x, y, biome_lookup_table.at(vf.point_container[point_key].biome_id));
        }
    }
    return return_image;*/
}



int main() {
    time_t starting_time = std::time(NULL);
    RNG rng;
    rng.set_seed(0.565869, 54.843648732);

    for(int i = 135; i < 140; i++) {
        double val = rng.retrieve();
        if(i == 31) {
            std::cout << std::to_string(val) << "\n";
        }
        PlanetBiomeGenerator pgen;
        pgen.set_parameters(val, 150, 75, 2, 3, 5, 1.6, 4, 4, 1, 1, 4, 4, 1, 1);

        //sf::Image img = pgen.generate_map(1500, 750);
        //img.saveToFile("file" + std::to_string(i) + "u.png");
    };
    /*double accumulator = 0;
    double sd;
    double sd_accumulator = 0;

    PerlinNoiseFactory pnf;
    RNG rng;
    rng.set_seed(0.862387, 54.92367);
    for(int i = 0; i < 16; i++) {
        pnf.set_parameters(rng.retrieve(), rng.retrieve() * 55.25536, 4, 4, 4, 2, true, true);
        pnf.generate_vectors();
        for(int x = 0; x < 500; x++) {
            for(int y = 0; y < 500; y++) {
                accumulator += pow(pnf.retrieve(x, y, 50, 50) - 0.5, 2);
            }
        }
        sd = std::sqrt(accumulator / (500 * 500));
        sd_accumulator += sd;
        std::cout << std::to_string(sd) << std::endl;
        accumulator = 0;
    }

    std::cout << "\n" << std::to_string(sd_accumulator / 16) << std::endl;*/

    time_t ending_time = std::time(NULL);
    std::cout << ending_time - starting_time << std::endl;
    //std::cout << pgen.vf.point_container.size() << "\n";

    return 0;
}