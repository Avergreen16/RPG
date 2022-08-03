#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <array>
#include <map>
#include <ctime>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <list>

#include "render.cpp"
#include "worldgen.cpp"
#include "pathfinding.cpp"
#include "text.cpp"
#include "asio_client.cpp"
#include "entities.cpp"

//

const uint chunk_load_x = 2;
const uint chunk_load_y = 2;
const uint total_loaded_chunks = (chunk_load_x * 2 + 1) * (chunk_load_y * 2 + 1);

enum game_activity{PLAY, CHAT};

game_activity current_activity = PLAY;

float vertices[] = {
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

std::array<double, 2> camera_pos;
int scale = 48;
double sqrt1_5 = std::sqrt(1.5);

std::map<int, bool> key_down_array = {
    {GLFW_KEY_W, false},
    {GLFW_KEY_S, false},
    {GLFW_KEY_A, false},
    {GLFW_KEY_D, false},
    {GLFW_KEY_SPACE, false},
    {GLFW_KEY_LEFT_SHIFT, false}
};

bool rect_collision(std::array<double, 4> a, std::array<double, 4> b) {
    if(a[0] < b[0] + b[2] && a[0] + a[2] > b[0] && a[1] < b[1] + b[3] && a[1] + a[3] > b[1])
        return true;
    return false;
}

bool enter_pressed = false;

int width = 1000, height = 1000;

int last_direction_key_pressed;

uint current_chunk = 0xffffffffu;
std::array<uint, total_loaded_chunks> active_chunks;

std::string char_callback_string = "";
std::list<text_struct> chat_list;
int type_pos = 0; // position in string of cursor
int type_start = 0; // position in string of leftmost displayed char
int type_len = 0; // length of string in pixels

int total_chat_lines = 0;
int chat_line = 0; // topmost row displayed of chat messages

int get_chat_list_lines() {
    int lines = 0;
    for(text_struct message : chat_list) {
        lines += message.lines;
    }
    return lines;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        if(current_activity == PLAY) {
            if(key_down_array.contains(key)) {
                key_down_array[key] = true;
                if(key == GLFW_KEY_W || key == GLFW_KEY_D || key == GLFW_KEY_S || key == GLFW_KEY_A) last_direction_key_pressed = key;
            }
            else if(key == GLFW_KEY_R) {
                chat_line = 0;
                total_chat_lines = 0;
                chat_list.clear();
            } else if(key == GLFW_KEY_N) {
                loaded_chunks.clear();
                current_chunk = 0xffffffffu;
            } else if(key == GLFW_KEY_T) {
                current_activity = CHAT;
                char_callback_string = "";
            }
        } else if(current_activity == CHAT) {
            if(key == GLFW_KEY_BACKSPACE) {
                if(char_callback_string.size() != 0 && type_pos != 0) {
                    int char_pos = type_pos - 1;
                    if(standard_chars.contains(char_callback_string[char_pos])) type_len -= standard_chars[char_callback_string[char_pos]][1] * 2 + 2;
                    char_callback_string.erase(char_callback_string.begin() + char_pos);
                    --type_pos;
                    if(type_pos < type_start) {
                        --type_start;
                        type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_ESCAPE) {
                current_activity = PLAY;
                type_pos = 0;
                type_start = 0;
                type_len = 0;
                chat_line = std::max(total_chat_lines - 12, 0);
                char_callback_string = "";
            } else if(key == GLFW_KEY_ENTER) {
                enter_pressed = true;
            } else if(key == GLFW_KEY_LEFT) {
                if(char_callback_string.size() != 0 && type_pos != 0) {
                    --type_pos;
                    if(standard_chars.contains(char_callback_string[type_pos])) type_len -= standard_chars[char_callback_string[type_pos]][1] * 2 + 2;
                    if(type_pos < type_start) {
                        --type_start;
                        type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_RIGHT) {
                if(char_callback_string.size() != 0 && type_pos < char_callback_string.size()) {
                    if(standard_chars.contains(char_callback_string[type_pos])) type_len += standard_chars[char_callback_string[type_pos]][1] * 2 + 2;
                    ++type_pos;
                    while(type_len >= width - 20) {
                        if(standard_chars.contains(char_callback_string[type_start])) type_len -= standard_chars[char_callback_string[type_start]][1] * 2 + 2;
                        ++type_start;
                    }
                }
            }
        }
    } else if(action == GLFW_RELEASE) {
        if(current_activity == PLAY && (!key_down_array[GLFW_KEY_LEFT_SHIFT] || key == GLFW_KEY_LEFT_SHIFT)) {
            if(key_down_array.find(key) != key_down_array.end()) key_down_array[key] = false;
        }
    } else if(action == GLFW_REPEAT) {
        if(current_activity == CHAT) {
            if(key == GLFW_KEY_BACKSPACE) {
                if(char_callback_string.size() != 0 && type_pos != 0) {
                    int char_pos = type_pos - 1;
                    if(standard_chars.contains(char_callback_string[char_pos])) type_len -= standard_chars[char_callback_string[char_pos]][1] * 2 + 2;
                    char_callback_string.erase(char_callback_string.begin() + char_pos);
                    --type_pos;
                    if(type_pos < type_start) {
                        --type_start;
                        type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_LEFT) {
                if(char_callback_string.size() != 0 && type_pos != 0) {
                    --type_pos;
                    if(standard_chars.contains(char_callback_string[type_pos])) type_len -= standard_chars[char_callback_string[type_pos]][1] * 2 + 2;
                    if(type_pos < type_start) {
                        --type_start;
                        type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_RIGHT) {
                if(char_callback_string.size() != 0 && type_pos < char_callback_string.size()) {
                    if(standard_chars.contains(char_callback_string[type_pos])) type_len += standard_chars[char_callback_string[type_pos]][1] * 2 + 2;
                    ++type_pos;
                    while(type_len >= width - 20) {
                        if(standard_chars.contains(char_callback_string[type_start])) type_len -= standard_chars[char_callback_string[type_start]][1] * 2 + 2;
                        ++type_start;
                    }
                }
            }
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(current_activity == PLAY) {
        if(yoffset > 0) {
            if(scale < 64) scale += 16;
        } else {
            if(scale > 16) scale -= 16;
        }
    } else if(current_activity == CHAT) {
        if(yoffset > 0) {
            if(chat_line >= 4) chat_line -= 4;
            else chat_line = 0;
        } else {
            if(chat_line <= get_chat_list_lines() - 16) chat_line += 4;
            else chat_line = get_chat_list_lines() - 12;
        }
    }
}

void char_callback(GLFWwindow* window, uint codepoint) {
    if(current_activity == CHAT) {
        if(standard_chars.contains(codepoint)) type_len += standard_chars[codepoint][1] * 2 + 2;
        char_callback_string.insert(char_callback_string.begin() + type_pos, codepoint);
        ++type_pos;
        while(type_len >= width - 20) {
            if(standard_chars.contains(char_callback_string[type_start])) type_len -= standard_chars[char_callback_string[type_start]][1] * 2 + 2;
            ++type_start;
        }
    }
}

//world gen

bool check_point_inclusion(std::array<double, 4> intrect, std::array<double, 2> point) {
    if(point[0] >= intrect[0] && point[0] < intrect[0] + intrect[2] && point[1] >= intrect[1] && point[1] < intrect[1] + intrect[3]) return true;
    return false;
}

bool check_point_inclusion(std::array<int, 4> intrect, std::array<double, 2> point) {
    if(point[0] >= intrect[0] && point[0] < intrect[0] + intrect[2] && point[1] >= intrect[1] && point[1] < intrect[1] + intrect[3]) return true;
    return false;
}

bool check_if_moved_chunk(std::array<double, 2> player_pos) {
    uint pos = (uint)(player_pos[0] / 16) + (uint)(player_pos[1] / 16) * world_size_chunks[0];
    if(pos != current_chunk) {
        current_chunk = pos;
        return true;
    }
    return false;
}

//entities

struct Approach {
    double value = 0.0;
    double target;

    void modify(double rising_change, double falling_change) {
        if(value != target) {
            if(value < target) {
                value += rising_change;
                if(value > target) value = target;
            } else {
                value -= falling_change;
                if(value < target) value = target;
            }
        }
    }

    void modify(double change) {
        if(value != target) {
            if(value < target) {
                value += change;
                if(value > target) value = target;
            } else {
                value -= change;
                if(value < target) value = target;
            }
        }
    }
};

struct Player {
    uint spritesheet;

    std::array<float, 2> visual_size = {2, 2};
    std::array<float, 2> visual_offset = {-1, -0.125};
    std::array<double, 2> position;
    std::array<int, 2> sprite_size = {32, 32};
    std::array<int, 2> active_sprite = {3, 2};
    std::array<double, 4> walk_box = {-0.375, -0.125, 0.75, 0.25};

    Double_counter cycle_timer = {0, 150};
    Int_counter sprite_counter = {0, 4};
    Approach speed;

    states state = IDLE;
    states texture_version = WALKING;
    directions direction_moving = SOUTH;
    directions direction_facing = SOUTH;
    bool keys_pressed = false;

    text_struct name;

    std::array<int, 2> get_current_chunk() {
        return {int(position[0] / 16), int(position[1] / 16)};
    }

    Player(std::array<double, 2> position, uint image, std::string name) {
        this->position = position;
        spritesheet = image;
        speed.target = 0.0;
        this->name.set_values(name, 0x10000, 1);
    }
    
    void render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size) {
        draw_tile(shader, spritesheet, {float((position[0] - camera_pos[0] + visual_offset[0]) * scale), float((position[1] - camera_pos[1] + visual_offset[1]) * scale), visual_size[0] * scale, visual_size[1] * scale}, {active_sprite[0], active_sprite[1], 1, 1, 4, 8}, window_size, (position[1] - 0.125 - reference_y) * 0.01 + 0.1);
    }

    void tick(uint delta) {
        if(state != IDLE) {
            double change_x = 0.0;
            double change_y = 0.0;

            active_sprite[1] = direction_facing;
            
            switch(direction_moving) {
                case NORTH:
                    change_y = 0.006 * delta;
                    break;
                case NORTHEAST:
                    change_x = 0.004243 * delta;
                    change_y = 0.004243 * delta;
                    break;
                case EAST:
                    change_x = 0.006 * delta;
                    break;
                case SOUTHEAST:
                    change_x = 0.004243 * delta;
                    change_y = -0.004243 * delta;
                    break;
                case SOUTH:
                    change_y = -0.006 * delta;
                    break;
                case SOUTHWEST:
                    change_x = -0.004243 * delta;
                    change_y = -0.004243 * delta;
                    break;
                case WEST:
                    change_x = -0.006 * delta;
                    break;
                case NORTHWEST:
                    change_x = -0.004243 * delta;
                    change_y = 0.004243 * delta;
                    break;
            }
            
            switch(state) {
                case WALKING:
                    texture_version = WALKING;
                    sprite_counter.limit = 4;
                    if(keys_pressed) {
                        speed.target = 1.0;
                        speed.modify(0.01 * delta);
                    } else {
                        speed.target = 0.0;
                        speed.modify(0.02 * delta);
                    }
                    if(cycle_timer.increment(delta)) {
                        sprite_counter.increment();
                    }
                    
                    position[0] += change_x * speed.value;
                    position[1] += change_y * speed.value;
                    break;

                case RUNNING:
                    texture_version = WALKING;
                    sprite_counter.limit = 4;
                    if(keys_pressed) {
                        speed.target = 1.6;
                        speed.modify(0.01 * delta);
                    } else {
                        speed.target = 0.0;
                        speed.modify(0.02 * delta);
                    }
                    if(cycle_timer.increment(delta * 1.3)) {
                        sprite_counter.increment();
                    }

                    position[0] += change_x * speed.value;
                    position[1] += change_y * speed.value;
                    break;

                case SWIMMING:
                    texture_version = SWIMMING;
                    sprite_counter.limit = 2;
                    if(sprite_counter.value > 1) sprite_counter.reset();
                    active_sprite[1] = direction_facing + 4;
                    if(keys_pressed) {
                        speed.target = 0.5;
                        speed.modify(0.005 * delta, 0.04 * delta);
                    } else {
                        speed.target = 0.0;
                        speed.modify(0.04 * delta);
                    }
                    if(cycle_timer.increment(delta * 0.6)) {
                        sprite_counter.increment();
                    }

                    position[0] += change_x * speed.value;
                    position[1] += change_y * speed.value;
                    break;
            }

            active_sprite[0] = sprite_counter.value;
        } else {
            sprite_counter.reset();
            cycle_timer.reset();
            if(texture_version == WALKING) active_sprite[0] = 3;
            else if(texture_version == SWIMMING) active_sprite[0] = 1;
        }
    }
};

struct Enemy {
    uint spritesheet;

    std::array<float, 2> visual_size = {2, 2};
    std::array<float, 2> visual_offset = {-1, -0.125};
    std::array<double, 2> position = {13150 + 0.5, 8110 + 0.5};
    std::array<int, 2> sprite_size = {32, 32};
    std::array<int, 2> active_sprite = {3, 2};

    Double_counter cycle_timer = {0, 150};
    Int_counter sprite_counter = {0, 4};

    states state = IDLE;
    directions direction_moving = SOUTH;
    directions direction_facing = SOUTH;

    Enemy(std::array<double, 2> position, uint image) {
        this->position = position;
        spritesheet = image;
    }
    
    void render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size) {
        draw_tile(shader, spritesheet, {float((position[0] - camera_pos[0] + visual_offset[0]) * scale), float((position[1] - camera_pos[1] + visual_offset[1]) * scale), visual_size[0] * scale, visual_size[1] * scale}, {active_sprite[0], active_sprite[1], 1, 1, 4, 8}, window_size, (position[1] - 0.125 - reference_y) * 0.01 + 0.1);
    }

    void tick(uint delta) {
        if(state != IDLE) {
            double change_x = 0.0;
            double change_y = 0.0;

            active_sprite[1] = direction_facing;
            
            switch(direction_moving) {
                case NORTH:
                    change_y = 0.006 * delta;
                    break;
                case NORTHEAST:
                    change_x = 0.004243 * delta;
                    change_y = 0.004243 * delta;
                    break;
                case EAST:
                    change_x = 0.006 * delta;
                    break;
                case SOUTHEAST:
                    change_x = 0.004243 * delta;
                    change_y = -0.004243 * delta;
                    break;
                case SOUTH:
                    change_y = -0.006 * delta;
                    break;
                case SOUTHWEST:
                    change_x = -0.004243 * delta;
                    change_y = -0.004243 * delta;
                    break;
                case WEST:
                    change_x = -0.006 * delta;
                    break;
                case NORTHWEST:
                    change_x = -0.004243 * delta;
                    change_y = 0.004243 * delta;
                    break;
            }

            if(state == WALKING) {
                if(cycle_timer.increment(delta)) {
                    sprite_counter.increment();
                }
                position[0] += change_x * 0.8;
                position[1] += change_y * 0.8;
            } else if(state == RUNNING) {
                if(cycle_timer.increment(delta * 1.3)) {
                    sprite_counter.increment();
                }
                position[0] += change_x * 1.28;
                position[1] += change_y * 1.28;
            }

            active_sprite[0] = sprite_counter.value;
        } else {
            sprite_counter.reset();
            cycle_timer.reset();
            active_sprite[0] = 3;
        }
    }

    void pathfind(Player target) {
        directions dir = Astar_pathfinding({7, 7}, {int(target.position[0] - position[0]) + 7, int(target.position[1] - position[1]) + 7}, empty_9x9_array);
        if(dir != -1) {
            state = WALKING;
            direction_moving = dir;
            switch(dir) {
                case NORTH:
                    direction_facing = NORTH;
                    break;

                case NORTHEAST:
                case SOUTHEAST:
                case EAST:
                    direction_facing = EAST;
                    break;
                
                case SOUTH:
                    direction_facing = SOUTH;
                    break;

                case NORTHWEST:
                case SOUTHWEST:
                case WEST:
                    direction_facing = WEST;
                    break;
            }
        } else {
            state = IDLE;
        }
    }
};

void set_player_state(std::unordered_map<uint, Chunk_data*> &loaded_chunks, Player* player) {
    std::vector<tile_ID> tiles_stepped_on = get_blocks_around(loaded_chunks, {player->walk_box[0] + player->position[0], player->walk_box[1] + player->position[1], player->walk_box[2], player->walk_box[3]});
    if(std::count(tiles_stepped_on.begin(), tiles_stepped_on.end(), WATER) == tiles_stepped_on.size()) {
        player->state = SWIMMING;
    } else if(key_down_array[GLFW_KEY_SPACE]) {
        player->state = RUNNING;
    } else {
        player->state = WALKING;
    }
}

void set_player_enums(std::unordered_map<uint, Chunk_data*> &loaded_chunks, Player* player) {
    bool check_last_key_pressed = true;
    if(key_down_array[GLFW_KEY_W]) {
        if(key_down_array[GLFW_KEY_D]) {
            player->direction_moving = NORTHEAST;
            player->direction_facing = EAST;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        } else if(key_down_array[GLFW_KEY_S]) {
            player->keys_pressed = false;
            if(player->speed.value == 0.0) player->state = IDLE;
            else {
                set_player_state(loaded_chunks, player);
            }
        } else if(key_down_array[GLFW_KEY_A]) {
            player->direction_moving = NORTHWEST;
            player->direction_facing = WEST;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        } else {
            player->direction_moving = NORTH;
            player->direction_facing = NORTH;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        }
    } else if(key_down_array[GLFW_KEY_D]) {
        if(key_down_array[GLFW_KEY_S]) {
            player->direction_moving = SOUTHEAST;
            player->direction_facing = EAST;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        } else if(key_down_array[GLFW_KEY_A]) {
            player->keys_pressed = false;
            if(player->speed.value == 0.0) player->state = IDLE;
            else {
                set_player_state(loaded_chunks, player);
            }
        } else {
            player->direction_moving = EAST;
            player->direction_facing = EAST;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        }
    } else if(key_down_array[GLFW_KEY_S]) {
        if(key_down_array[GLFW_KEY_A]) {
            player->direction_moving = SOUTHWEST;
            player->direction_facing = WEST;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        } else {
            player->direction_moving = SOUTH;
            player->direction_facing = SOUTH;
            player->keys_pressed = true;
            set_player_state(loaded_chunks, player);
        }
    } else if(key_down_array[GLFW_KEY_A]) {
        player->direction_moving = WEST;
        player->direction_facing = WEST;
        player->keys_pressed = true;
        set_player_state(loaded_chunks, player);
    } else {
        player->keys_pressed = false;
        if(player->speed.value == 0.0) player->state = IDLE;
        else {
            set_player_state(loaded_chunks, player);
        }
    }
}

//

std::unordered_map<int, float> offset_values = {
    {VOIDTILE, 0.0f},
    {TALL_GRASS, 0.0625f}
};

std::vector<std::string> connection_input_collector;
void process_input(std::string input, Player& player, std::list<text_struct>& chat_list, netwk::TCP_client& connection) {
    if(input[0] == '*') {
        // command
        if(strcmp(input.substr(1, 4).data(), "tpm ") == 0) {
            std::string x = "";
            std::string y = "";
            int pos = 5;
            for(char c : input.substr(5)) {
                pos++;
                if(std::find(valid_numbers.begin(), valid_numbers.end(), c) == valid_numbers.end()) {
                    break;
                } else {
                    x += c;
                }
            }
            for(char c : input.substr(pos)) {
                if(std::find(valid_numbers.begin(), valid_numbers.end(), c) == valid_numbers.end()) {
                    break;
                } else {
                    y += c;
                }
            }
            int x_val = strtoul(x.data(), nullptr, 10);
            int y_val = strtoul(y.data(), nullptr, 10);
            if(x_val < 0 || x_val >= world_size[0] || y_val < 0 || y_val >= world_size[1]) {
                text_struct message;
                message.set_values("Invalid parameters. (*tpm)", 600, 2);
                message.get_len();
                chat_list.emplace_back(message);
                total_chat_lines += message.lines;
                if(chat_list.size() >= 25) {
                    total_chat_lines -= chat_list.front().lines;
                    chat_list.pop_front();
                }
            } else {
                player.position = {x_val + 0.5, y_val + 0.25};
                text_struct message;
                message.set_values("Teleported " + player.name.str + "\\c000 to coordinates: " + std::to_string(x_val) + " " + std::to_string(y_val), 600, 2);
                message.get_len();
                chat_list.emplace_back(message);
                total_chat_lines += message.lines;
                if(chat_list.size() >= 25) {
                    total_chat_lines -= chat_list.front().lines;
                    chat_list.pop_front();
                }
            }
        } else {
            text_struct message;
            message.set_values("Invalid command.", 600, 2);
            message.get_len();
            chat_list.emplace_back(message);
            total_chat_lines += message.lines;
            if(chat_list.size() >= 25) {
                total_chat_lines -= chat_list.front().lines;
                chat_list.pop_front();
            }
        }
    } else {
        // chat
        if(input.size() != 0) {
            std::vector<uint8_t> body(input.size());
            memcpy(body.data(), input.data(), input.size());

            connection.send(1, body);
        }
    }
}

void chunk_gen_thread(bool &game_running, std::unordered_map<uint, Chunk_data*> &loaded_chunks, std::array<uint, total_loaded_chunks> &active_chunks, std::array<int, 2> world_size_chunks, uint &current_chunk, Worldgen &worldgen, Player &player) {
    while(game_running) {
        std::vector<uint> update_chunk_queue;
        if(check_if_moved_chunk(player.position)) {
            std::array<uint, total_loaded_chunks> new_active_chunks;
            std::array<int, 2> current_chunk_pos = {current_chunk % world_size_chunks[0], current_chunk / world_size_chunks[0]};
            for(int x = 0; x <= chunk_load_x * 2; ++x) {
                for(int y = 0; y <= chunk_load_y * 2; ++y) {
                    uint chunk_key = current_chunk + (x - chunk_load_x) + world_size_chunks[0] * (y - chunk_load_y);
                    if(insert_chunk(loaded_chunks, world_size_chunks, chunk_key, &worldgen)) {
                        update_chunk_queue.push_back(chunk_key);
                    }
                    new_active_chunks[x + y * (chunk_load_x * 2 + 1)] = chunk_key;
                }
            }

            for(uint chunk_key : update_chunk_queue) {
                update_chunk_water(loaded_chunks, world_size_chunks, chunk_key);
            }
            update_chunk_queue.clear();

            active_chunks = new_active_chunks;
        }
    }
}

void str_thread(bool &game_running, std::string &str) {
    while(game_running) {
        std::string getline_string;
        getline(std::cin, getline_string);
        str = getline_string;
    }
}

int main() {
    netwk::TCP_client connection(0xa50e);
    time_t time_storage_frames = clock();

    if(!glfwInit()) {
        std::cout << "glfw failure (init)\n";
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1000, 600, "test", NULL, NULL);
    if(!window) {
        std::cout << "glfw failure (window)\n";
        return 1;
    }
    glfwSetCharCallback(window, char_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);

    gladLoadGL();

    glfwSwapInterval(0);

    uint square_shader_program = create_shader(vertex_shader, fragment_shader);
    uint text_shader_program = create_shader(vertex_shader_text, fragment_shader_text);
    uint chunk_shader_program = create_shader(vertex_shader_chunk, fragment_shader_chunk);
    uint chunk_shader_program_depth = create_shader(vertex_shader_chunk_depth, fragment_shader_chunk);
    uint solid_shader_program = create_shader(vertex_shader_color, fragment_shader_color);

    uint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1); 

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    std::array<int, 3> floor_tileset_info, text_img_info, player_spritesheet_info, bandit0_spritesheet_info, bandit1_spritesheet_info;
    uint floor_tileset = generate_texture("res\\tiles\\floor_tileset.png", floor_tileset_info);
    player_spritesheet = generate_texture("res\\entities\\Player\\avergreen_spritesheet.png", player_spritesheet_info);
    bandit0_spritesheet = generate_texture("res\\entities\\Bandit\\bandit0_spritesheet.png", bandit0_spritesheet_info);
    bandit1_spritesheet = generate_texture("res\\entities\\Bandit\\bandit1_spritesheet.png", bandit1_spritesheet_info);
    text_img = generate_texture("res\\gui\\textsource.png", text_img_info);

    int frames = 0;

    Worldgen worldgen;
    worldgen.construct_3(0.7707326, 6, 3, 7, 1.5, 4, 4, 3, 1.5, 4, 4, 4, 1.3);

    //Player player1;
    
    std::string player_name;
    std::getline(std::cin, player_name);

    Player player({23472 + 0.5, 10384 + 0.5}, player_spritesheet, player_name);

    bool c = true;

    Double_counter water_cycle_timer;
    Int_counter water_sprite_counter;
    water_cycle_timer = {0, 800};
    water_sprite_counter = {0, 4};

    bool game_running = true;

    std::thread chunk_thread(
        [&game_running, &worldgen, &player]() {
            chunk_gen_thread(game_running, loaded_chunks, active_chunks, world_size_chunks, current_chunk, worldgen, player);
        }
    );

    std::thread process_connection_input_thread(
        [&game_running]() {
            while(game_running) {
                if(connection_input_collector.size() > 0) {
                    std::string input = connection_input_collector.back();
                    connection_input_collector.pop_back();
                    if(input.size() != 0) {
                        text_struct message;
                        message.set_values(input, 600, 2);
                        message.get_len();
                        total_chat_lines += message.lines;
                        chat_list.emplace_back(message);
                    }
                    if(chat_list.size() >= 25) {
                        total_chat_lines -= chat_list.front().lines;
                        chat_list.pop_front();
                    }
                    if(current_activity == PLAY) {
                        chat_line = std::max(total_chat_lines - 12, 0);
                    }
                }
            }
        }
    );

    std::unordered_map<uint64_t, Entity> player_map;

    text_struct version = {"\\c44fThe Simulation \\c000pre-alpha \\cf440.0.\\x1", 2, 1000};
    connection.start(connection_input_collector, player_map, game_running);
    
        
    std::array<char, 64> player_name_array;
    memcpy(player_name_array.data(), player_name.data(), player_name.size() + 1);
    
    netwk::player_join_packet_toserv join_packet{player_name_array, {player.position, player.direction_facing}};
    std::vector<uint8_t> msg_body = netwk::to_byte_vector(join_packet);
    connection.send(0, msg_body);

    time_t packet_time_container = clock();
    time_t time_storage_delta = clock();

    while(game_running) {
        // packets
        time_t current_time = clock();
        if(current_time - packet_time_container >= 80) {
            packet_time_container = current_time;
            netwk::entity_movement_packet_toserv send_packet{player.position, player.direction_facing};
            std::vector<uint8_t> msg_body = to_byte_vector(send_packet);
            connection.send(2, msg_body);
        }

        // window

        glfwGetFramebufferSize(window, &width, &height);
        width = width + 1 * (width % 2 == 1);
        height = height + 1 * (height % 2 == 1);
        glViewport(0, 0, width, height);

        glfwPollEvents();

        //keys

        set_player_enums(loaded_chunks, &player);

        //math
        uint delta_time = clock() - time_storage_delta;
        time_storage_delta = clock();

        if(enter_pressed) {
            process_input(char_callback_string, player, chat_list, connection);
            char_callback_string = "";
            current_activity = PLAY;
            enter_pressed = false;
            type_pos = 0;
            type_start = 0;
            type_len = 0;
            chat_line = std::max(total_chat_lines - 12, 0);
        }
            
        player.tick(delta_time);

        for(auto& [key, entity] : player_map) {
            entity.tick(delta_time);
        }

        if(water_cycle_timer.increment(delta_time)) water_sprite_counter.increment();

        camera_pos = {player.position[0], player.position[1] + 0.875};

        //rendering

        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int reference_y = int(player.position[1] / 16) * 16 - 32;

        for(int i = 0; i < total_loaded_chunks; ++i) {
            uint key = active_chunks[i];
            
            if(loaded_chunks.contains(key)) {
                Chunk_data* chunk = loaded_chunks[key];
                
                double x_corner = (chunk->corner[0] - camera_pos[0]) * scale;
                double y_corner = (chunk->corner[1] - camera_pos[1]) * scale;

                if(rect_collision({x_corner, y_corner, 16.0 * scale, 16.0 * scale}, {-width * 0.5, -height * 0.5, double(width), double(height)})) {
                    glBindTexture(GL_TEXTURE_2D, floor_tileset);

                    glUseProgram(chunk_shader_program_depth);
                    glUniform2f(0, width, height);
                    glUniform2f(4, scale, scale);
                    glUniform2f(6, 8.0, 16.0);
                    glUniform2f(2, x_corner, y_corner);
                    glUniform1f(8, round((chunk->corner[1] - reference_y)) * 0.01 + 0.1);

                    for(int j = 0; j < 256; ++j) {
                        glUniform1i(9 + j, chunk->object_tiles[j].texture);
                        glUniform1f(265 + j, offset_values[chunk->object_tiles[j].id]);
                    }

                    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 256);

                    // objects
                    glUseProgram(chunk_shader_program);
                    glUniform2f(0, width, height);
                    glUniform2f(4, scale, scale);
                    glUniform2f(6, 8.0, 16.0);
                    glUniform2f(2, x_corner, y_corner);

                    for(int j = 0; j < 256; ++j) {
                        int floor_tex = chunk->floor_tiles[j].texture;
                        if(floor_tex >= 56) floor_tex += water_sprite_counter.value;
                        glUniform1i(8 + j, floor_tex);
                    }

                    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 256);
                }
            }
        }

        player.render(reference_y, camera_pos, scale, square_shader_program, {width, height});

        for(auto& [key, entity] : player_map) {
            entity.render(reference_y, camera_pos, scale, square_shader_program, {width, height});
        }

        std::string hex_coords = to_hex_string(player.position[0]) + " " + to_hex_string(player.position[1]);
        std::string dec_coords = std::to_string(int(player.position[0])) + " " + std::to_string(int(player.position[1]));
        std::string str_length = to_hex_string(char_callback_string.size());

        render_text(text_shader_program, text_img, text_img_info, {width, height}, {float(width / 2 - 10 - (version.get_len() * 2)), float(height / 2 - 64)}, version.str, 2, 1000);
        render_text(text_shader_program, text_img, text_img_info, {width, height}, {float(width / 2 - 10 - ((hex_coords.size() - 4) * 7 - 3) * 2), float(height / 2 - 90)}, hex_coords, 2, 500);
        render_text(text_shader_program, text_img, text_img_info, {width, height}, {float(width / 2 - 10 - (dec_coords.size() * 7 - 3) * 2), float(height / 2 - 116)}, dec_coords, 2, 500);
        if(current_activity == CHAT) {
            render_text_type(text_shader_program, text_img, text_img_info, {width, height}, {float(-width / 2 + 10), float(height / 2 - 34)}, char_callback_string.substr(type_start), 2, type_pos - type_start, width);
            
            glUseProgram(solid_shader_program);
            glUniform1f(0, 0.001f);
            glUniform2f(1, width, height);
            glUniform4f(3, -width / 2, height / 2 - 37, width, 37);
            glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        int text_scale = 0.0625 * (scale - 16);
        render_text(text_shader_program, text_img, text_img_info, {width, height}, {float(-player.name.get_len() / 2 * text_scale), float(0.875 * scale)}, player.name.str, text_scale, 1000);

        int text_y_pos = height / 2 - 64;
        int chat_line_counter = 0;
        for(text_struct message : chat_list) {
            if(chat_line_counter >= chat_line) {
                int lines_displayed = chat_line_counter - chat_line;
                if(lines_displayed < 12) {
                    int lines_passed = render_text(text_shader_program, text_img, text_img_info, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(0, (12 - lines_displayed < message.lines) ? message.first_chars[12 - lines_displayed] : message.str.length()), 2, 600);
                    text_y_pos -= lines_passed * 26;
                    chat_line_counter += lines_passed;
                }
            } else {
                chat_line_counter += message.lines;
                if(chat_line_counter > chat_line) {
                    int num = message.lines - (chat_line_counter - chat_line);
                    int lines_passed = render_text(text_shader_program, text_img, text_img_info, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(message.first_chars[num]), 2, 600);
                    text_y_pos -= lines_passed * 26;
                }
            }
        }

        if(chat_list.size() != 0) {
            glUseProgram(solid_shader_program);
            glUniform1f(0, 0.001f);
            glUniform2f(1, width, height);
            glUniform4f(3, -width / 2, text_y_pos + 26 - 5, 620, height / 2 - text_y_pos - 56);
            glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //

        glfwSwapBuffers(window);
        frames++;
        
        time_t time_frames = clock();
        if(frames % 100 == 0) {
            printf("%f\n", (double)frames / (time_frames - time_storage_frames) * 1000);
            time_storage_frames = time_frames;
            frames = 0;
            std::cout << "\n";
        }

        game_running = !glfwWindowShouldClose(window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    chunk_thread.join();

    for(auto& [chunk_key, chunk_ptr] : loaded_chunks) {
        delete chunk_ptr;
        chunk_ptr = nullptr;
    }
    return 0;
}