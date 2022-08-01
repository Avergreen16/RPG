#pragma once
#include <array>
#include <queue>

#include "global.cpp"
#include "text.cpp"
#include "render.cpp"

enum states{IDLE, WALKING, RUNNING, SWIMMING};

std::array<int, 3> player_spritesheet_info, bandit0_spritesheet_info, bandit1_spritesheet_info;
uint player_spritesheet = generate_texture("res\\entities\\Player\\avergreen_spritesheet.png", player_spritesheet_info);
uint bandit0_spritesheet = generate_texture("res\\entities\\Bandit\\bandit0_spritesheet.png", bandit0_spritesheet_info);
uint bandit1_spritesheet = generate_texture("res\\entities\\Bandit\\bandit1_spritesheet.png", bandit1_spritesheet_info);

struct Double_counter {
    double value = 0;
    double limit;

    void construct(double plimit) {
        limit = plimit;
    };

    bool increment(double change) {
        value += change;
        if(value >= limit) {
            value = 0;
            return true;
        }
        return false;
    }

    void reset() {
        value = 0;
    }
};

struct Int_counter {
    int value = 0;
    int limit;

    void construct(int plimit) {
        limit = plimit;
    };

    bool increment() {
        value++;
        if(value >= limit) {
            value = 0;
            return true;
        }
        return false;
    }

    void reset() {
        value = 0;
    }
};

struct entity_queue_struct {
    std::array<double, 2> position;
    directions direction;
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
    std::array<int, 2> active_sprite = {0, 3};

    Double_counter cycle_timer;
    Int_counter sprite_counter;

    states state = IDLE;
    directions direction_facing = SOUTH;
    states texture_version = WALKING;

    std::queue<entity_queue_struct> movement_queue;
    entity_queue_struct previous_packet = {{-0x100, -0x100}, SOUTH, UINT32_MAX};
    time_t last_input_time;
    uint interpolation_time = 0;

    Entity(uint64_t id, uint spritesheet, text_struct username, std::array<double, 2> position, directions direction_facing) {
        this->id = id;
        this->spritesheet = spritesheet;
        this->username = username;
        this->position = position;
        this->direction_facing = direction_facing;
        this->active_sprite[0] = direction_facing;
    }

    void render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size) {
        draw_tile(shader, spritesheet, {float((position[0] - camera_pos[0] + visual_offset[0]) * scale), float((position[1] - camera_pos[1] + visual_offset[1]) * scale), visual_size[0] * scale, visual_size[1] * scale}, {active_sprite[0], active_sprite[1], 1, 1, 4, 8}, window_size, (position[1] - 0.125 - reference_y) * 0.01 + 0.1);
    }

    int insert_position(std::array<double, 2> position, directions direction) {
        time_t time_container = clock();
        movement_queue.emplace(entity_queue_struct{position, direction, uint(time_container - last_input_time)});
        last_input_time = time_container;
    }

    void tick(double delta) {
        if(movement_queue.size() != 0) {
            if(previous_packet.delta_time == UINT32_MAX) previous_packet = movement_queue.front();

            direction_facing = movement_queue.front().direction;
            active_sprite[0] = direction_facing;
            
            interpolation_time += delta;
            if(interpolation_time > movement_queue.front().delta_time) {
                interpolation_time -= movement_queue.front().delta_time;
                previous_packet = movement_queue.front();
                movement_queue.pop();
                if(movement_queue.size() != 0) {
                    double lerp_value = interpolation_time / movement_queue.front().delta_time;
                    position = {lerp(movement_queue.front().position[0], previous_packet.position[0], lerp_value), lerp(movement_queue.front().position[1], previous_packet.position[1], lerp_value)};
                } else {
                    position = previous_packet.position;
                }
            } else {
                double lerp_value = interpolation_time / movement_queue.front().delta_time;
                position = {lerp(movement_queue.front().position[0], previous_packet.position[0], lerp_value), lerp(movement_queue.front().position[1], previous_packet.position[1], lerp_value)};
            }
        }
    }
};