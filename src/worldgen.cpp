#pragma once
#define _USE_MATH_DEFINES

#include <cmath>
#include <vector>
#include <array>
#include <unordered_map>

#include "global.cpp"

int w, h, nrChannels;
unsigned char* biome_map_terra = stbi_load("biome_map_t0.png", &w, &h, &nrChannels, 4);

unsigned int get_pixel(unsigned char* data, std::array<int, 2> pixel, int image_width) {
    int pixel_index = pixel[0] + pixel[1] * image_width;
    return 0x01000000 * data[pixel_index * 4] + 0x00010000U * data[pixel_index * 4 + 1] + 0x00000100U * data[pixel_index * 4 + 2] + 0xFFU;
}

std::array<unsigned char, 4> get_pixel_array(unsigned char* data, std::array<int, 2> pixel, int image_width) {
    int pixel_index = pixel[0] + pixel[1] * image_width;
    return {data[pixel_index * 4], data[pixel_index * 4 + 1], data[pixel_index * 4 + 2], data[pixel_index * 4 + 3]};
}

struct RNG {
    double product;
    double factor;

    double operator() () {
        product = product * factor;
        product -= floor(product);
        return product;
    }
};

double clamp(double input, double min, double max) {
    return std::max(std::min(input, max), min);
}

double normalCDF(double input, double sd, double mean) {
    double value = (input - mean) / sd;
    return 0.5 * erfc(-value * M_SQRT1_2);
}

double dotproduct(std::array<double, 2> v1, std::array<double, 2> v2) {
    return v1[0] * v2[0] + v1[1] * v2[1];
}

double interpolate(double x1, double x2, double pos) {
    double returnvalue = (x2 - x1) * (6 * pos * pos * pos * pos * pos - 15 * pos * pos * pos * pos + 10 * pos * pos * pos) + x1;
    return returnvalue;
}

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

    std::vector<std::array<double, 2>> vector_container;

    void set_parameters(double seed, double factor, int main_vector_width, int main_vector_height, int octaves, double persistance, double sd, double mean, bool loop_x, bool loop_y);

    void generate_vectors();

    double retrieve(double x, double y, double width, double height);
};

void PerlinNoiseFactory::set_parameters(double seed, double factor, int main_vector_width, int main_vector_height, int octaves, double persistance, double sd, double mean, bool loop_x, bool loop_y) {
    this->main_vector_width = main_vector_width;
    this->main_vector_height = main_vector_height;
    this->octaves = octaves;
    this->persistance = persistance;
    this->loop_x = loop_x;
    this->loop_y = loop_y;
    rng = {seed, factor};
    
    this->sd = sd;
    this->mean = mean;

    maxvalue = 0;
    for(int i = 0; i < octaves; i++) {
        maxvalue += 1 * pow(persistance, -i);
    }
}

void PerlinNoiseFactory::generate_vectors() {
    vector_container.clear();
    int array_width = main_vector_width * pow(2, octaves);
    int array_height = main_vector_height * pow(2, octaves);
    int num_vectors = array_width * array_height;
    if(!loop_x) {
        num_vectors += array_height;
        if(!loop_y) {
            num_vectors += array_width + 1;
        }
    }
    else if(!loop_y) num_vectors += array_width;
    for(int k = 0; k < num_vectors; k++) {
        double theta = rng() * 2 * M_PI;
        std::array<double, 2> insertvector = {std::cos(theta), std::sin(theta)};
        vector_container.push_back(insertvector);
    }
}

double PerlinNoiseFactory::retrieve(double x, double y, double width, double height) {
    double returnvalue = 0;
    for(int i = 0; i < octaves; i++) {
        //updated scale
        int i_power = 1 << i;
        double updated_xscale = double(width) / (main_vector_width * i_power) * 0.99999999;
        double updated_yscale = double(height) / (main_vector_height * i_power) * 0.99999999;

        // stores the point's 4 dot products for interpolation
        double dotproducts[4];

        // grabs the coordinate value of the top left vector of the 4 surrounding vectors in the "vector array", each vector gets an
        // integer coordinate. These values are used to determine the vector's key
        int topleft_x = double(x) * i_power * main_vector_width / width;
        int topleft_y = double(y) * i_power * main_vector_height / height;
        
        for(int j = 0; j < 4; j++) {
            // vector_x and vector_y are the x and y integer coordinates of the active vector, determined by j, in the "vector array"
            int vector_x = topleft_x + j % 2;
            int vector_y = topleft_y + floor(j / 2);

            // these are the true coordinates of the active vector so that the distance vector between the 
            // active vector's origin and the point's x and y can be calculated
            double pos_value_x = vector_x * updated_xscale;
            double pos_value_y = vector_y * updated_yscale;

            vector_x -= main_vector_width * i_power * (loop_x) * (vector_x >= main_vector_width * i_power);
            vector_y -= main_vector_height * i_power * (loop_y) * (vector_y >= main_vector_height * i_power);

            // creates vector key for lookup in the vector map
            int vector_key = vector_x + vector_y * main_vector_width * i_power;

            // vector of active point, current active point is one of the 4 closest vectors, this variable is assigned each of the
            // 4 vectors as the j loop runs.
            std::array<double, 2> active_vector = vector_container[vector_key];
            // distance vector between true position of active vector's origin and x and y coordinate
            std::array<double, 2> coordinate_distance_vector = {(double(x) - pos_value_x) / updated_xscale, (double(y) - pos_value_y) / updated_yscale};
            //if(coordinate_distance_vector[0] == 0 || coordinate_distance_vector[1] == 0) printf("check");

            dotproducts[j] = dotproduct(active_vector, coordinate_distance_vector);
        }

        // interpolates the dot products bilineraly and returns the result
        //(double)(coordinate % scale) / scale
        double x1 = interpolate(dotproducts[0], dotproducts[1], fmod(double(x), updated_xscale) / updated_xscale);
        double x2 = interpolate(dotproducts[2], dotproducts[3], fmod(double(x), updated_xscale) / updated_xscale);
        returnvalue += (interpolate(x1, x2 , fmod(double(y), updated_yscale) / updated_yscale) + M_SQRT1_2) / (M_SQRT1_2 * 2) * pow(persistance, -i);
    }
    if(sd != 0) return normalCDF(returnvalue / maxvalue, sd, mean);
    return returnvalue / maxvalue;
}

struct Worldgen {
    PerlinNoiseFactory pfac1;
    PerlinNoiseFactory pfac2;
    PerlinNoiseFactory pfac3;
    RNG rng;

    void construct_3(double seed, int width1, int height1, int octaves1, double persistance1, int width2, int height2, int octaves2, double persistance2, int width3, int height3, int octaves3, double persistance3) {
        pfac1.set_parameters(seed, seed * 45.18754874623, width1, height1, octaves1, persistance1, 0.15, 0.5, true, false);
        pfac2.set_parameters(seed, seed * 21.1324254354, width2, height2, octaves2, persistance2, 0.2, 0.5, true, false);
        pfac3.set_parameters(seed, seed * 57.62634832648, width3, height3, octaves3, persistance3, 0.15, 0.5, true, false);
        pfac1.generate_vectors();
        pfac2.generate_vectors();
        pfac3.generate_vectors();
        rng = {seed, seed * 60.5564301};
    }

    unsigned int retrieve(std::array<int, 2> location, std::array<int, 2> map_size) {
        double elevation = pfac1.retrieve(location[0], location[1], map_size[0], map_size[1]) * 12;
        double temperature = clamp((pfac2.retrieve(location[0], location[1], map_size[0], map_size[1]) - 0.5) * 3 + (0.5 - std::abs(double(location[1]) / map_size[1] - 0.5)) * 24, 0, 11.999);
        double humidity = pfac3.retrieve(location[0], location[1], map_size[0], map_size[1]) * 12;

        unsigned int biome = get_pixel(biome_map_terra, {floor(elevation) * 12 + floor(humidity), floor(temperature)}, 144);
        if(elevation > 7 && elevation <= 7.03 && biome != 0xE59647FF && biome != 0xF4ECFFFF && biome != 0x2D541BFF && temperature >= 3) biome = 0xFFFBB3FF;
        //printf("%u ", biome);
        return biome;
    }
};

enum tile_ID{VOIDTILE, SAND, GRASS, WATER, TALL_GRASS};

struct tile {
    tile_ID id;
    int texture;
};

struct Chunk_data {
    std::array<tile, 256> floor_tiles;
    std::array<tile, 256> object_tiles;
    std::array<int, 2> corner;
};

tile get_floor_tile(unsigned int biome, Worldgen* worldgen) {
    double rand;
    tile_ID tile_type;
    int texture;
    switch(biome) {
        case 0xfffbb3ff:
            tile_type = SAND;
            rand = worldgen->rng();
            if(rand < 0.25) texture = 24;
            else texture = floor(worldgen->rng() * 4) + 25;
            break;
        
        case 0x8bce50ff:
            tile_type = GRASS;
            rand = worldgen->rng();
            if(rand < 0.5) texture = floor(worldgen->rng() * 2) + 1;
            else texture = floor(worldgen->rng() * 5) + 3;
            break;

        case 0x008944ff:
            tile_type = GRASS;
            rand = worldgen->rng();
            if(rand < 0.5) texture = floor(worldgen->rng() * 2) + 9;
            else texture = floor(worldgen->rng() * 5) + 11;
            break;
        
        case 0x286059ff:
            tile_type = GRASS;
            rand = worldgen->rng();
            if(rand < 0.5) texture = floor(worldgen->rng() * 2) + 16;
            else texture = floor(worldgen->rng() * 5) + 18;
            break;
        
        case 0xe2dc98ff:
            tile_type = SAND;
            rand = worldgen->rng();
            if(rand < 0.5) texture = 32;
            else if(rand < 0.9) texture = floor(worldgen->rng() * 2) + 33;
            else texture = floor(worldgen->rng() * 2) + 35;
            break;
        
        case 0xe59647ff:
            tile_type = SAND;
            rand = worldgen->rng();
            if(rand < 0.5) texture = 40;
            else if(rand < 0.9) texture = floor(worldgen->rng() * 2) + 41;
            else texture = floor(worldgen->rng() * 2) + 43;
            break;
        
        case 0xc8d16aff:
            tile_type = GRASS;
            rand = worldgen->rng();
            if(rand < 0.6) texture = floor(worldgen->rng() * 2) + 48;
            else texture = floor(worldgen->rng() * 5) + 50;
            break;
        
        case 0x00b539ff:
            tile_type = GRASS;
            texture = floor(worldgen->rng() * 5) + 10;
            break;
        
        case 0x191fd3ff:
            tile_type = WATER;
            texture = 56;
            break;
        
        case 0x1942d3ff:
            tile_type = WATER;
            texture = 56;
            break;

        case 0x1418adff:
            tile_type = WATER;
            texture = 56;
            break;
        
        default:
            tile_type = VOIDTILE;
            texture = 8;
            break;
    }
    return {tile_type, texture};
}

int get_adjacent_chunk_key(std::array<int, 2> chunk_pos, std::array<int, 2> world_size_chunks, std::array<int, 2> chunk_diff) {
    std::array<int, 2> new_chunk_pos = {chunk_pos[0] + chunk_diff[0], chunk_pos[1] + chunk_diff[1]};

    if(new_chunk_pos[0] < 0) new_chunk_pos[0] = world_size_chunks[0] - new_chunk_pos[0];
    else if(new_chunk_pos[0] >= world_size_chunks[0]) new_chunk_pos[0] = new_chunk_pos[0] - world_size_chunks[0];

    if(new_chunk_pos[1] < 0) new_chunk_pos[1] = world_size_chunks[1] - new_chunk_pos[1];
    else if(new_chunk_pos[1] >= world_size_chunks[1]) new_chunk_pos[1] = new_chunk_pos[1] - world_size_chunks[1];

    return new_chunk_pos[0] + new_chunk_pos[1] * world_size_chunks[0];
}

int get_adjacent_chunk_key(unsigned int chunk_key, std::array<int, 2> world_size_chunks, std::array<int, 2> chunk_diff) {
    std::array<int, 2> new_chunk_pos = {(chunk_key % world_size_chunks[0]) + chunk_diff[0], (chunk_key / world_size_chunks[0]) + chunk_diff[1]};

    if(new_chunk_pos[0] < 0) new_chunk_pos[0] = world_size_chunks[0] - new_chunk_pos[0];
    else if(new_chunk_pos[0] >= world_size_chunks[0]) new_chunk_pos[0] = new_chunk_pos[0] - world_size_chunks[0];

    if(new_chunk_pos[1] < 0) new_chunk_pos[1] = world_size_chunks[1] - new_chunk_pos[1];
    else if(new_chunk_pos[1] >= world_size_chunks[1]) new_chunk_pos[1] = new_chunk_pos[1] - world_size_chunks[1];

    return new_chunk_pos[0] + new_chunk_pos[1] * world_size_chunks[0];
}

tile_ID get_tile(std::unordered_map<unsigned int, Chunk_data*> &loaded_chunks, unsigned int chunk_key, int x, int y) {
    if(loaded_chunks.contains(chunk_key)) {
        return loaded_chunks[chunk_key]->floor_tiles[x + y * 16].id;
    }
    return VOIDTILE;
}

int get_texture_variation(tile_ID target, std::array<tile_ID, 4> neighbors) {
    if(neighbors[0] == target) {
        if(neighbors[1] == target) {
            if(neighbors[2] == target) {
                if(neighbors[3] == target) return 15;
                return 13;
            } else if(neighbors[3] == target) return 14;
            return 9;
        } else if(neighbors[2] == target) {
            if(neighbors[3] == target) return 12;
            return 8;
        } else if(neighbors[3] == target) return 7;
        return 2;
    } else if(neighbors[1] == target) {
        if(neighbors[2] == target) {
            if(neighbors[3] == target) return 11;
            return 6;
        } else if(neighbors[3] == target) return 5;
        return 1;
    } else if(neighbors[2] == target) {
        if(neighbors[3] == target) return 10;
        return 4;
    } else if(neighbors[3] == target) return 3;
    return 0;
}

tile get_object_tile(unsigned int biome, Worldgen* worldgen) {
    double rand;
    switch(biome) {
        
        case 0x8bce50ff:
        case 0xc8d16aff:
            rand = worldgen->rng();
            if(rand < 0.15) return {TALL_GRASS, int(worldgen->rng() * 3) + 45};
            return {VOIDTILE, 0};
        
        default:
            return {VOIDTILE, 0};
    }
}

void update_tile_water(std::unordered_map<unsigned int, Chunk_data*> loaded_chunks, std::array<int, 2> world_size_chunks, unsigned int chunk_key, int x, int y) {
    int i = x + y * 16;
    if(loaded_chunks[chunk_key]->floor_tiles[i].id == WATER) {
        std::array<tile_ID, 4> neighbors;
        if(i % 16 == 15) {
            if(i < 16) {
                neighbors = {loaded_chunks[chunk_key]->floor_tiles[i + 16].id, get_tile(loaded_chunks, chunk_key - world_size_chunks[0], i, 15), loaded_chunks[chunk_key]->floor_tiles[i - 1].id, get_tile(loaded_chunks, chunk_key + 1, 0, int(i / 16))};
            } else if(i > 239) {
                neighbors = {get_tile(loaded_chunks, chunk_key + world_size_chunks[0], i % 16, 0), loaded_chunks[chunk_key]->floor_tiles[i - 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 1].id, get_tile(loaded_chunks, chunk_key + 1, 0, int(i / 16))};
            } else {
                neighbors = {loaded_chunks[chunk_key]->floor_tiles[i + 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 1].id, get_tile(loaded_chunks, chunk_key + 1, 0, int(i / 16))};
            }
        } else if(i % 16 == 0) {
            if(i < 16) {
                neighbors = {loaded_chunks[chunk_key]->floor_tiles[i + 16].id, get_tile(loaded_chunks, chunk_key - world_size_chunks[0], i, 15), get_tile(loaded_chunks, chunk_key - 1, 15, int(i / 16)), loaded_chunks[chunk_key]->floor_tiles[i + 1].id};
            } else if(i > 239) {
                neighbors = {get_tile(loaded_chunks, chunk_key + world_size_chunks[0], i % 16, 0), loaded_chunks[chunk_key]->floor_tiles[i - 16].id, get_tile(loaded_chunks, chunk_key - 1, 15, int(i / 16)), loaded_chunks[chunk_key]->floor_tiles[i + 1].id};
            } else {
                neighbors = {loaded_chunks[chunk_key]->floor_tiles[i + 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 16].id, get_tile(loaded_chunks, chunk_key - 1, 15, int(i / 16)), loaded_chunks[chunk_key]->floor_tiles[i + 1].id};
            }
        } else if(i < 16) {
            neighbors = {loaded_chunks[chunk_key]->floor_tiles[i + 16].id, get_tile(loaded_chunks, chunk_key - world_size_chunks[0], i, 15), loaded_chunks[chunk_key]->floor_tiles[i - 1].id, loaded_chunks[chunk_key]->floor_tiles[i + 1].id};
        } else if(i > 239) {
            neighbors = {get_tile(loaded_chunks, chunk_key + world_size_chunks[0], i % 16, 0), loaded_chunks[chunk_key]->floor_tiles[i - 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 1].id, loaded_chunks[chunk_key]->floor_tiles[i + 1].id};
        } else {
            neighbors = {loaded_chunks[chunk_key]->floor_tiles[i + 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 16].id, loaded_chunks[chunk_key]->floor_tiles[i - 1].id, loaded_chunks[chunk_key]->floor_tiles[i + 1].id};
        }

        int variation = get_texture_variation(WATER, neighbors);
        switch(variation) {
            case 0:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 56;
                break;
            case 1:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 104;
                break;
            case 2:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 116;
                break;
            case 3:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 112;
                break;
            case 4:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 108;
                break;
            case 5:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 88;
                break;
            case 6:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 92;
                break;
            case 7:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 96;
                break;
            case 8:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 100;
                break;
            case 9:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 84;
                break;
            case 10:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 80;
                break;
            case 11:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 68;
                break;
            case 12:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 64;
                break;
            case 13:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 72;
                break;
            case 14:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 76;
                break;
            case 15:
                loaded_chunks[chunk_key]->floor_tiles[i].texture = 60;
                break;
        }
    }
}

void update_chunk_water(std::unordered_map<unsigned int, Chunk_data*> loaded_chunks, std::array<int, 2> world_size_chunks, unsigned int chunk_key) {
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            update_tile_water(loaded_chunks, world_size_chunks, chunk_key, x, y);
        }
    }
    unsigned int chunk_key_north = chunk_key + world_size_chunks[0];
    unsigned int chunk_key_east = chunk_key + 1;
    unsigned int chunk_key_south = chunk_key - world_size_chunks[0];
    unsigned int chunk_key_west = chunk_key - 1;
    if(loaded_chunks.contains(chunk_key_north)) {
        for(int x = 0; x < 16; x++) {
            update_tile_water(loaded_chunks, world_size_chunks, chunk_key_north, x, 0);
        }
    }
    if(loaded_chunks.contains(chunk_key_east)) {
        for(int y = 0; y < 16; y++) {
            update_tile_water(loaded_chunks, world_size_chunks, chunk_key_east, 0, y);
        }
    }
    if(loaded_chunks.contains(chunk_key_south)) {
        for(int x = 0; x < 16; x++) {
            update_tile_water(loaded_chunks, world_size_chunks, chunk_key_south, x, 15);
        }
    }
    if(loaded_chunks.contains(chunk_key_west)) {
        for(int y = 0; y < 16; y++) {
            update_tile_water(loaded_chunks, world_size_chunks, chunk_key_west, 15, y);
        }
    }
}

Chunk_data* generate_chunk(Worldgen* worldgen, std::array<int, 2> world_size_chunks, std::array<int, 2> southwest) {
    std::array<int, 2> world_size = {world_size_chunks[0] * 16, world_size_chunks[1] * 16};
    std::array<tile, 256> floor_tiles;
    std::array<tile, 256> object_tiles;
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            std::array<int, 2> position = {southwest[0] + x, (southwest[1] + y)};
            unsigned int biome = worldgen->retrieve(position, world_size);

            tile floor = get_floor_tile(biome, worldgen);
            tile object = get_object_tile(biome, worldgen);
            floor_tiles[x + y * 16] = floor;
            object_tiles[x + y * 16] = object;
        }
    }

    return new Chunk_data{floor_tiles, object_tiles, southwest};
}

bool insert_chunk(std::unordered_map<unsigned int, Chunk_data*> &loaded_chunks, std::array<int, 2> world_size_chunks, unsigned int chunk_key, Worldgen* worldgen) {
    if(!loaded_chunks.contains(chunk_key)) {
        std::array<int, 2> corner = {int(chunk_key % world_size_chunks[0] * 16), int(chunk_key / world_size_chunks[0]) * 16};
        printf("%i %i\n", corner[0], corner[1]);
        loaded_chunks.insert({chunk_key, generate_chunk(worldgen, world_size_chunks, corner)});
        return true;
    }
    return false;
}