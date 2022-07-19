#define _USE_MATH_DEFINES
#include <cmath>
#include <SDL2-w64\include\SDL2\SDL_image.h>
#include "planet_noise.hpp"

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

double interpolate(double x1, double x2, int position) {
    double returnvalue = (x2 - x1) * (6 * pow(position, 5) - 15 * pow(position, 4) + 10 * pow(position, 3)) + x1;
    return returnvalue;
}

double hypotenuse(double x, double y) {
    return sqrt(x * x + y * y);
}

RNG::RNG(double p_seed, double p_factor) {
    factor = p_factor;
    rng_number = p_seed;
}

double RNG::retrieve() {
    rng_number = rng_number * factor;
    rng_number -= floor(rng_number);
    return rng_number;

    //return floor(rng_number * pow(10, decimal_digits)) / pow(10, decimal_digits);
}

PerlinNoiseFactory::PerlinNoiseFactory(double p_seed, double p_factor, int p_main_vector_width, int p_main_vector_height, int p_octaves, double p_persistance, bool p_loop_x, bool p_loop_y) {
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
        double x1 = interpolate(dotproducts[0], dotproducts[1], fmod((double)x, updated_xscale) / updated_xscale);
        double x2 = interpolate(dotproducts[2], dotproducts[3], fmod((double)x, updated_xscale) / updated_xscale);
        returnvalue += (interpolate(x1, x2 , fmod((double)y, updated_yscale) / updated_yscale) + sqrt_half) / (sqrt_half * 2) * pow(persistance, -i);
    }
    return returnvalue / maxvalue;
}

v_Point::v_Point(double p_x, double p_y) {
    coordinates[0] = p_x;
    coordinates[1] = p_y;
}

VoronoiFactory::VoronoiFactory(double p_seed, double p_factor, int p_point_map_width, int p_point_map_height, bool p_loop_x, bool p_loop_y) : loop_x(p_loop_x), loop_y(p_loop_y), point_map_width(p_point_map_width), point_map_height(p_point_map_height) {
    rng = RNG(p_seed, p_factor);
}

void VoronoiFactory::generate_points() {
    for(int x = 0; x < point_map_width; x++) {
        for(int y = 0; y < point_map_height; y++) {
            double vp_x = (100.0 / point_map_width) * (x + rng.retrieve());
            double vp_y = (100.0 / point_map_height) * (y + rng.retrieve());

            point_map.insert({x + y * point_map_width, v_Point(vp_x, vp_y)});
        }
    }
}

int VoronoiFactory::pull_closest_point_key(double x, double y, int width, int height) {
    int topleft_x = floor(x / ((double)width / point_map_width));
    int topleft_y = floor(y / ((double)height / point_map_height));

    double scaled_x = x / width * 100;
    double scaled_y = y / height * 100;

    double dist = __DBL_MAX__;

    int closest_point_key;
    for(int i = -2; i < 3; i++) {
        for(int j = -2; j < 3; j++) {
            int point_x = topleft_x + i;
            int point_y = topleft_y + j;

            int edge_x = 0;
            int edge_y = 0;

            if(loop_x) {
                while (point_x < 0) {
                    point_x += point_map_width;
                    edge_x -= 100;
                }
                while (point_x >= point_map_width) {
                    point_x -= point_map_width;
                    edge_x += 100;
                }
            }
            if(loop_y) {
                while (point_y < 0) {
                    point_y += point_map_height;
                    edge_y -= 100;
                }
                while (point_y >= point_map_height) {
                    point_y -= point_map_height;
                    edge_y += 100;
                }
            }

            int point_key = point_x + point_y * point_map_width;
            
            if (point_map.find(point_key) == point_map.end()) {
                int vp_x = floor(rng.retrieve() * ((double)width / point_map_width) + ((double)width / point_map_width) * point_x);
                int vp_y = floor(rng.retrieve() * ((double)height / point_map_height) + ((double)height / point_map_height) * point_y);
                v_Point pt(vp_x, vp_y);
                point_map.insert({point_key, pt});
            }

            double active_point_dist = hypotenuse(point_map.at(point_key).coordinates[0] - scaled_x + edge_x, point_map.at(point_key).coordinates[1] - scaled_y + edge_y);

            if (active_point_dist < dist) {
                dist = active_point_dist;
                closest_point_key = point_key;
            }
        }
    }

    return closest_point_key;
}

PlanetTerrainGenerator::PlanetTerrainGenerator(double p_seed, int p_vwidth, int p_vheight, int p_main_elevwidth, int p_main_elevheight, int p_elevoctaves, int p_elevpersistance) {
    v_fac = VoronoiFactory(p_seed, 32.9287349, p_vwidth, p_vheight, true, true);
    p_fac_elev = PerlinNoiseFactory(p_seed, 57.2635826, p_main_elevwidth, p_main_elevheight, p_elevoctaves, p_elevpersistance, true, true);
}

std::string PlanetTerrainGenerator::get_biome(double elev) {
    if(elev < 0.5) {
        return "sea";
    } else if(elev < 0.7) {
        return "forest";
    } else {
        return "mountains";
    }
}

void draw_planet(SDL_Surface* surface) {
    std::map<std::string, Uint32> biome_lookup_table = {
        {"sea", SDL_MapRGBA(surface->format, 30, 30, 180, 255)},
        /*{"deep_sea", SDL_MapRGBA(surface->format, 10, 10, 160, 255)},
        {"reef", SDL_MapRGBA(surface->format, 50, 50, 150, 255)},
        {"frozen_sea", SDL_MapRGBA(surface->format, 150, 180, 220, 255)},
        {"grassland", SDL_MapRGBA(surface->format, 120, 200, 120, 255)},
        {"swamp", SDL_MapRGBA(surface->format, 50, 145, 80, 255)},*/
        {"forest", SDL_MapRGBA(surface->format, 10, 180, 10, 255)},
        /*{"rainforest", SDL_MapRGBA(surface->format, 10, 250, 50, 255)},
        {"boreal_forest", SDL_MapRGBA(surface->format, 20, 100, 10, 255)},
        {"shrubland", SDL_MapRGBA(surface->format, 180, 200, 20, 255)},
        {"desert", SDL_MapRGBA(surface->format, 220, 200, 50, 255)},
        {"ice_cap", SDL_MapRGBA(surface->format, 240, 240, 250, 255)},
        {"tundra", SDL_MapRGBA(surface->format, 100, 100, 20, 255)},
        {"river", SDL_MapRGBA(surface->format, 60, 60, 200, 255)},
        {"canyons", SDL_MapRGBA(surface->format, 200, 140, 20, 255)},*/
        {"mountains", SDL_MapRGBA(surface->format, 150, 150, 150, 255)},
        //{"glacier", SDL_MapRGBA(surface->format, 215, 215, 250, 255)}
    };

    
}