#pragma once
#define _USE_MATH_DEFINES

#include <cmath>
#include <vector>
#include <array>
#include <unordered_map>

#include "global.cpp"

#include "PerlinNoise-master\PerlinNoise.hpp"

const double PI = 3.141592653589793;
const double SQRT1_2 = sqrt(2) / 2;

std::array<int, 2> world_size_chunks = {2450, 1225};
std::array<int, 2> world_size = {world_size_chunks[0] * 16, world_size_chunks[1] * 16};

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

struct Worldgen {
    const unsigned int width = 1024, height = 512;
    const double river_threshold = 1.0/256;
    const int seed = 0x1003; // 0x1001 (4097)
    const double elev_scale = 1.2; // 1.1
    const double river_scale = 2.3;
    const double mountain_scale = 1.8;

    std::array<siv::PerlinNoise, 5> noise = {siv::PerlinNoise(seed), siv::PerlinNoise(seed + 1), siv::PerlinNoise(seed + 2), siv::PerlinNoise(seed + 3), siv::PerlinNoise(seed + 4)};
    RNG rng;

    Worldgen(double seed) {
        rng = {seed, seed * 60.5564301};
    }

    uint32_t retrieve(std::array<int, 2> location, std::array<int, 2> map_size) {
        double angle = 2 * PI * (double(location[0]) / world_size[0]);
        std::array<double, 2> xy1{cos(angle), sin(angle)};

        double z_angle = PI * (double(location[1]) / world_size[1] - 0.5);
        double xy_magnitude = cos(z_angle);
        std::array<double, 3> position = {xy1[0] * xy_magnitude, xy1[1] * xy_magnitude, sin(z_angle)}; // z axis inverted for consistency with coordinates in the simulation
        double elev = noise[0].normalizedOctave3D(position[0] * elev_scale + 0.798, position[1] * elev_scale - 0.332, position[2] * elev_scale, 7, 0.6);
        double mountain = -abs(noise[4].normalizedOctave3D(position[0] * mountain_scale - 0.225, position[1] * mountain_scale - 0.808, position[2] * mountain_scale, 5, 0.55)) + 1.0;
        if(mountain < 0.9) mountain = 0.0;
        double temp = noise[1].normalizedOctave3D(position[0], position[1] + 0.776, position[2] - 1.113, 3, 0.55) * 7 + (1 - abs(z_angle) / PI * 2) * 12;
        double humid = noise[2].normalizedOctave3D(position[0] + 0.106, position[1], position[2], 5, 0.55);

        double height_val = std::min(elev * 10 + 6.5, 9.0) + mountain * elev * 10;
        uint32_t biome = get_pixel(biome_map_terra, {int(clamp(floor(height_val), 0.0, 11.0) * 12 + clamp(floor(humid * 18 + 6), 0.0, 11.0)), int(clamp(floor(temp), 0.0, 11.0))}, 144);

        if(biome != 0xf4ecffff && biome != 0x2d541bff && height_val >= 7.0 && height_val < 7.02734375) {
            biome = 0xfffbb3ff;
        }

        if(biome != 0x90b1dbff && biome != 0x1418adff && biome != 0x191fd3ff && biome != 0x1942d3ff && biome != 0xf4ecffff && biome != 0xaed3d3ff) {
            double river = noise[3].normalizedOctave3D(position[0] * river_scale + 5.667, position[1] * river_scale, position[2] * river_scale - 3.332, 4, 0.5);
            if(river < river_threshold && river > -river_threshold) {
                biome = 0x1942d3ff;
            } else {
                river = noise[3].normalizedOctave3D(position[0] * river_scale * 2 + 6.443, position[1] * river_scale * 2, position[2] * river_scale * 2 + 5.098, 4, 0.5);
                if(river < river_threshold / 2 && river > -river_threshold / 2) {
                    biome = 0x1942d3ff;
                }
            }
        }

        return biome;
    }
};

enum tile_ID{VOIDTILE, SAND, GRASS, WATER, TALL_GRASS};

struct Tile {
    tile_ID id;
    glm::vec2 texture;

    Tile() = default;
    Tile(tile_ID id, int texture) {
        this->id = id;
        this->texture = {(texture % 8) * 16, (texture / 8) * 16};
    }
};

struct Server_chunk {
    std::array<Tile, 512> tiles;
    glm::ivec2 chunk_index;
};

Tile get_floor_tile(unsigned int biome, Worldgen& worldgen) {
    double rand;
    tile_ID tile_type;
    int texture;
    switch(biome) {
        case 0xfffbb3ff:
            tile_type = SAND;
            rand = worldgen.rng();
            if(rand < 0.25) texture = 24;
            else texture = floor(worldgen.rng() * 4) + 25;
            break;
        
        case 0x8bce50ff:
            tile_type = GRASS;
            rand = worldgen.rng();
            if(rand < 0.5) texture = floor(worldgen.rng() * 2) + 1;
            else texture = floor(worldgen.rng() * 5) + 3;
            break;

        case 0x008944ff:
            tile_type = GRASS;
            rand = worldgen.rng();
            if(rand < 0.5) texture = floor(worldgen.rng() * 2) + 9;
            else texture = floor(worldgen.rng() * 5) + 11;
            break;
        
        case 0x286059ff:
            tile_type = GRASS;
            rand = worldgen.rng();
            if(rand < 0.5) texture = floor(worldgen.rng() * 2) + 16;
            else texture = floor(worldgen.rng() * 5) + 18;
            break;
        
        case 0xe2dc98ff:
            tile_type = SAND;
            rand = worldgen.rng();
            if(rand < 0.5) texture = 32;
            else if(rand < 0.9) texture = floor(worldgen.rng() * 2) + 33;
            else texture = floor(worldgen.rng() * 2) + 35;
            break;
        
        case 0xe59647ff:
            tile_type = SAND;
            rand = worldgen.rng();
            if(rand < 0.5) texture = 40;
            else if(rand < 0.9) texture = floor(worldgen.rng() * 2) + 41;
            else texture = floor(worldgen.rng() * 2) + 43;
            break;
        
        case 0xc8d16aff:
            tile_type = GRASS;
            rand = worldgen.rng();
            if(rand < 0.6) texture = floor(worldgen.rng() * 2) + 48;
            else texture = floor(worldgen.rng() * 5) + 50;
            break;
        
        case 0x00b539ff:
            tile_type = GRASS;
            texture = floor(worldgen.rng() * 5) + 10;
            break;
        
        case 0x191fd3ff:
            tile_type = WATER;
            texture = 60;
            break;
        
        case 0x1942d3ff:
            tile_type = WATER;
            texture = 60;
            break;

        case 0x1418adff:
            tile_type = WATER;
            texture = 60;
            break;
        
        default:
            tile_type = VOIDTILE;
            texture = 8;
            break;
    }
    return Tile(tile_type, texture);
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

Tile get_object_tile(unsigned int biome, Worldgen& worldgen) {
    double rand;
    switch(biome) {
        case 0x8bce50ff:
        case 0xc8d16aff:
            rand = worldgen.rng();
            if(rand < 0.15) return Tile(TALL_GRASS, int(worldgen.rng() * 3) + 45);
            return Tile(VOIDTILE, 0);
        
        default:
            return Tile(VOIDTILE, 0);
    }
}

/*void update_tile_water(std::unordered_map<unsigned int, Chunk_data>& loaded_chunks, std::array<int, 2> world_size_chunks, unsigned int chunk_key, int x, int y) {
    int i = x + y * 16;
    if(loaded_chunks[chunk_key].tiles[i].id == WATER) {
        std::array<tile_ID, 4> neighbors;
        if(i % 16 == 15) {
            if(i < 16) {
                neighbors = {loaded_chunks[chunk_key].tiles[i + 16].id, get_tile(loaded_chunks, chunk_key - world_size_chunks[0], i, 15), loaded_chunks[chunk_key].tiles[i - 1].id, get_tile(loaded_chunks, chunk_key + 1, 0, int(i / 16))};
            } else if(i > 239) {
                neighbors = {get_tile(loaded_chunks, chunk_key + world_size_chunks[0], i % 16, 0), loaded_chunks[chunk_key].tiles[i - 16].id, loaded_chunks[chunk_key].tiles[i - 1].id, get_tile(loaded_chunks, chunk_key + 1, 0, int(i / 16))};
            } else {
                neighbors = {loaded_chunks[chunk_key].tiles[i + 16].id, loaded_chunks[chunk_key].tiles[i - 16].id, loaded_chunks[chunk_key].tiles[i - 1].id, get_tile(loaded_chunks, chunk_key + 1, 0, int(i / 16))};
            }
        } else if(i % 16 == 0) {
            if(i < 16) {
                neighbors = {loaded_chunks[chunk_key].tiles[i + 16].id, get_tile(loaded_chunks, chunk_key - world_size_chunks[0], i, 15), get_tile(loaded_chunks, chunk_key - 1, 15, int(i / 16)), loaded_chunks[chunk_key].tiles[i + 1].id};
            } else if(i > 239) {
                neighbors = {get_tile(loaded_chunks, chunk_key + world_size_chunks[0], i % 16, 0), loaded_chunks[chunk_key].tiles[i - 16].id, get_tile(loaded_chunks, chunk_key - 1, 15, int(i / 16)), loaded_chunks[chunk_key].tiles[i + 1].id};
            } else {
                neighbors = {loaded_chunks[chunk_key].tiles[i + 16].id, loaded_chunks[chunk_key].tiles[i - 16].id, get_tile(loaded_chunks, chunk_key - 1, 15, int(i / 16)), loaded_chunks[chunk_key].tiles[i + 1].id};
            }
        } else if(i < 16) {
            neighbors = {loaded_chunks[chunk_key].tiles[i + 16].id, get_tile(loaded_chunks, chunk_key - world_size_chunks[0], i, 15), loaded_chunks[chunk_key].tiles[i - 1].id, loaded_chunks[chunk_key].tiles[i + 1].id};
        } else if(i > 239) {
            neighbors = {get_tile(loaded_chunks, chunk_key + world_size_chunks[0], i % 16, 0), loaded_chunks[chunk_key].tiles[i - 16].id, loaded_chunks[chunk_key].tiles[i - 1].id, loaded_chunks[chunk_key].tiles[i + 1].id};
        } else {
            neighbors = {loaded_chunks[chunk_key].tiles[i + 16].id, loaded_chunks[chunk_key].tiles[i - 16].id, loaded_chunks[chunk_key].tiles[i - 1].id, loaded_chunks[chunk_key].tiles[i + 1].id};
        }

        int variation = get_texture_variation(WATER, neighbors);
        switch(variation) {
            case 0:
                loaded_chunks[chunk_key].tiles[i].texture = 56;
                break;
            case 1:
                loaded_chunks[chunk_key].tiles[i].texture = 104;
                break;
            case 2:
                loaded_chunks[chunk_key].tiles[i].texture = 116;
                break;
            case 3:
                loaded_chunks[chunk_key].tiles[i].texture = 112;
                break;
            case 4:
                loaded_chunks[chunk_key].tiles[i].texture = 108;
                break;
            case 5:
                loaded_chunks[chunk_key].tiles[i].texture = 88;
                break;
            case 6:
                loaded_chunks[chunk_key].tiles[i].texture = 92;
                break;
            case 7:
                loaded_chunks[chunk_key].tiles[i].texture = 96;
                break;
            case 8:
                loaded_chunks[chunk_key].tiles[i].texture = 100;
                break;
            case 9:
                loaded_chunks[chunk_key].tiles[i].texture = 84;
                break;
            case 10:
                loaded_chunks[chunk_key].tiles[i].texture = 80;
                break;
            case 11:
                loaded_chunks[chunk_key].tiles[i].texture = 68;
                break;
            case 12:
                loaded_chunks[chunk_key].tiles[i].texture = 64;
                break;
            case 13:
                loaded_chunks[chunk_key].tiles[i].texture = 72;
                break;
            case 14:
                loaded_chunks[chunk_key].tiles[i].texture = 76;
                break;
            case 15:
                loaded_chunks[chunk_key].tiles[i].texture = 60;
                break;
        }
    }
}*/

/*void update_chunk_water(std::unordered_map<unsigned int, Chunk_data>& loaded_chunks, std::array<int, 2> world_size_chunks, unsigned int chunk_key) {
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
}*/

Server_chunk generate_chunk(Worldgen& worldgen, std::array<int, 2> world_size_chunks, glm::ivec2 index) {
    std::array<int, 2> world_size = {world_size_chunks[0] * 16, world_size_chunks[1] * 16};
    std::array<Tile, 512> tiles;
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            std::array<int, 2> position = {index[0] * 16 + x, index[1] * 16 + y};
            unsigned int biome = worldgen.retrieve(position, world_size);

            Tile floor = get_floor_tile(biome, worldgen);
            Tile object = get_object_tile(biome, worldgen);
            tiles[x + y * 16] = floor;
            tiles[x + y * 16 + 256] = object;
        }
    }

    return Server_chunk{tiles, index};
}

bool insert_chunk(std::unordered_map<unsigned int, Server_chunk>& loaded_chunks, std::array<int, 2> world_size_chunks, unsigned int chunk_key, Worldgen& worldgen) {
    if(!loaded_chunks.contains(chunk_key)) {
        glm::ivec2 index = {chunk_key % world_size_chunks[0], chunk_key / world_size_chunks[0]};
        loaded_chunks.insert({chunk_key, generate_chunk(worldgen, world_size_chunks, index)});
        return true;
    }
    return false;
}

std::array<int, 25> offsets = {
    -2 + 2 * world_size_chunks[0], -1 + 2 * world_size_chunks[0], 2 * world_size_chunks[0], 1 + 2 * world_size_chunks[0], 2 + 2 * world_size_chunks[0],
    -2 + world_size_chunks[0], -1 + world_size_chunks[0], world_size_chunks[0], 1 + world_size_chunks[0], 2 + world_size_chunks[0],
    -2, -1, 0, 1, 2,
    -2 - world_size_chunks[0], -1 - world_size_chunks[0], -world_size_chunks[0], 1 - world_size_chunks[0], 2 - world_size_chunks[0],
    -2 - 2 * world_size_chunks[0], -1 - 2 * world_size_chunks[0], -2 * world_size_chunks[0], 1 - 2 * world_size_chunks[0], 2 - 2 * world_size_chunks[0]
};