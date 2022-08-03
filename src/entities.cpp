#pragma once
#include <array>
#include <queue>
#include <iostream>
#include <algorithm>

#include "global.cpp"
#include "text.cpp"
#include "asio_packets.cpp"
#include "worldgen.cpp"
//#include "asio_client.cpp"
#include "render.cpp"

uint player_spritesheet;
uint bandit0_spritesheet;
uint bandit1_spritesheet;

std::vector<tile_ID> get_blocks_around(std::unordered_map<uint, Chunk_data*> &loaded_chunks, std::array<double, 4> bounding_box) {
    std::vector<tile_ID> return_vector;

    int lower_x = bounding_box[0];
    int lower_y = bounding_box[1];
    int upper_x = bounding_box[0] + bounding_box[2];
    int upper_y = bounding_box[1] + bounding_box[3];

    for(int y = lower_y; y <= upper_y; ++y) {
        for(int x = lower_x; x <= upper_x; ++x) {
            int chunk_x = x / 16;
            int chunk_y = y / 16;
            uint chunk_key = chunk_x + chunk_y * world_size_chunks[0];
            int pos_x = x % 16;
            int pos_y = y % 16;
            return_vector.push_back(get_tile(loaded_chunks, chunk_key, pos_x, pos_y));
        }
    }

    return return_vector;
}

struct Double_counter {
    double value;
    double limit;

    void construct(double limit) {
        this->limit = limit;
    };

    bool increment(double change) {
        value += change;
        if(value >= limit) {
            value = fmod(value, limit);
            return true;
        }
        return false;
    }

    void set_limit(double new_limit) {
        limit = new_limit;
        if(value >= limit) value = 0;
    }

    void reset() {
        value = 0;
    }
};

struct Int_counter {
    int value;
    int limit;

    bool increment() {
        value++;
        if(value >= limit) {
            value = 0;
            return true;
        }
        return false;
    }

    bool increment_by(int change) {
        value += change;
        if(value >= limit) {
            value %= limit;
            return true;
        }
        return false;
    }

    void set_limit(int new_limit) {
        limit = new_limit;
        if(value >= limit) value = 0;
    }

    void reset() {
        value = 0;
    }
};

struct entity_queue_struct {
    std::array<double, 2> position;
    directions direction;
    states state;
    uint delta_time;
};

struct Entity {
    uint64_t id;
    uint spritesheet;
    text_struct username;

    std::array<float, 2> visual_size = {2, 2};
    std::array<float, 2> visual_offset = {-1, -0.125};
    std::array<double, 2> position = {-0x100, -0x100};
    std::array<int, 2> sprite_size = {32, 32};
    std::array<int, 2> active_sprite = {3, 2};
    std::array<double, 4> walk_box = {-0.375, -0.125, 0.75, 0.25};

    Double_counter cycle_timer = {0, 150};
    Int_counter sprite_counter = {3, 4};

    states state = IDLE;
    directions direction_facing = SOUTH;
    states texture_version = WALKING;

    std::queue<entity_queue_struct> movement_queue;
    entity_queue_struct previous_packet = {{-0x100, -0x100}, SOUTH, IDLE, UINT32_MAX};
    time_t last_input_time;
    uint interpolation_time = 0;

    Entity(uint64_t id, uint spritesheet, text_struct username, std::array<double, 2> position, directions direction_facing) {
        this->id = id;
        this->spritesheet = spritesheet;
        this->username = username;
        this->position = position;
        this->direction_facing = direction_facing;
        this->active_sprite[1] = direction_facing;
        last_input_time = clock();
    }

    void render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size) {
        draw_tile(shader, spritesheet, {float((position[0] - camera_pos[0] + visual_offset[0]) * scale), float((position[1] - camera_pos[1] + visual_offset[1]) * scale), visual_size[0] * scale, visual_size[1] * scale}, {active_sprite[0], active_sprite[1], 1, 1, 4, 8}, window_size, (position[1] - 0.125 - reference_y) * 0.01 + 0.1);
    }

    int insert_position(netwk::entity_movement_packet_toclient input_packet) {
        time_t time_container = clock();
        movement_queue.emplace(entity_queue_struct{input_packet.position, input_packet.direction, input_packet.state, uint(time_container - last_input_time)});
        last_input_time = time_container;
    }

    void tick(uint delta) {
        if(movement_queue.size() != 0) {
            if(previous_packet.delta_time == UINT32_MAX) {
                direction_facing = movement_queue.front().direction;
                previous_packet = movement_queue.front();
            }
            
            interpolation_time += delta;
            if(interpolation_time > movement_queue.front().delta_time) { // switch packets
                interpolation_time -= movement_queue.front().delta_time;
                previous_packet = movement_queue.front();
                movement_queue.pop();
                direction_facing = movement_queue.front().direction;
                if(previous_packet.position[0] == movement_queue.front().position[0] &&
                previous_packet.position[1] == movement_queue.front().position[1]) {
                    state = IDLE;
                } else {
                    state = (movement_queue.front().state == IDLE) ? WALKING : movement_queue.front().state;
                }

                if(movement_queue.size() != 0) {
                    double lerp_value = (movement_queue.front().delta_time == 0) ? 1.0 : double(interpolation_time) / movement_queue.front().delta_time;
                    position = {lerp(previous_packet.position[0], movement_queue.front().position[0], lerp_value), lerp(previous_packet.position[1], movement_queue.front().position[1], lerp_value)};
                } else {
                    position = previous_packet.position;
                }
            } else {
                double lerp_value = (movement_queue.front().delta_time == 0) ? 1.0 : double(interpolation_time) / movement_queue.front().delta_time;
                position = {lerp(previous_packet.position[0], movement_queue.front().position[0], lerp_value), lerp(previous_packet.position[1], movement_queue.front().position[1], lerp_value)};
            }
        }

        std::vector<tile_ID> tiles_stepped_on = get_blocks_around(loaded_chunks, {walk_box[0] + position[0], walk_box[1] + position[1], walk_box[2], walk_box[3]});
        if(std::count(tiles_stepped_on.begin(), tiles_stepped_on.end(), WATER) == tiles_stepped_on.size()) {
            texture_version = SWIMMING;
            active_sprite[1] = direction_facing + 4;
            sprite_counter.set_limit(2);
        } else {
            texture_version = WALKING;
            active_sprite[1] = direction_facing;
            sprite_counter.set_limit(4);
        }

        switch(state) {
            case IDLE: {
                switch(texture_version) {
                    case WALKING: {
                        active_sprite[0] = 3;
                        break;
                    }
                    case SWIMMING: {
                        active_sprite[0] = 1;
                        break;
                    }
                }
                break;
            }
            case WALKING: {
                double timer_increment = delta;
                switch(texture_version) {
                    case SWIMMING: {
                        timer_increment *= 0.6;
                        break;
                    }
                }
                if(cycle_timer.increment(timer_increment)) {
                    sprite_counter.increment();
                }
                break;
            }
            case RUNNING: {
                double timer_increment = delta;
                switch(texture_version) {
                    case WALKING: {
                        timer_increment *= 1.3;
                        break;
                    }
                    case SWIMMING: {
                        timer_increment *= 0.6;
                        break;
                    }
                }
                if(cycle_timer.increment(timer_increment)) {
                    sprite_counter.increment();
                }
                break;
            }
        }

        active_sprite[0] = sprite_counter.value;
    }
};