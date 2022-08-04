#pragma once
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
#include <chrono>
#include <memory>

#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>

#include "entities.cpp"
#include "asio_packets.cpp"
#include "worldgen.cpp"
#include "render.cpp"
#include "pathfinding.cpp"
#include "text.cpp"


// asio client
using asio::ip::tcp;

struct Setting;

namespace netwk {
    struct TCP_client {
        asio::io_context io_context;
        tcp::resolver resolver;
        tcp::socket socket;
        std::thread recieve_thread;

        TCP_client(unsigned short int target_port);

        void receive_loop(Setting& setting, bool& game_running);

        void start(Setting& setting, bool& game_running);

        void send(std::vector<uint8_t> packet);
    };
}


/// client setting

const uint chunk_load_x = 2;
const uint chunk_load_y = 2;
const uint total_loaded_chunks = (chunk_load_x * 2 + 1) * (chunk_load_y * 2 + 1);

enum game_activity{PLAY, CHAT};

bool rect_collision(std::array<double, 4> a, std::array<double, 4> b) {
    if(a[0] < b[0] + b[2] && a[0] + a[2] > b[0] && a[1] < b[1] + b[3] && a[1] + a[3] > b[1])
        return true;
    return false;
}

struct Approach {
    double value = 0.0;
    double target;

    void modify(double change) {
        if(value < target) value = std::min(value + change, target);
        else if(value > target) value = std::max(value - change, target);
    }

    void modify(double rising_change, double falling_change) {
        if(value < target) value = std::min(value + rising_change, target);
        else if(value > target) value = std::max(value - falling_change, target);
    }
};

struct Player {
    uint texture_id;

    std::array<float, 2> visual_size = {2, 2};
    std::array<float, 2> visual_offset = {-1, -0.125};
    std::array<double, 2> position;
    std::array<int, 2> sprite_size = {32, 32};
    std::array<int, 2> active_sprite = {3, 2};
    std::array<double, 4> walk_box = {-0.375, -0.125, 0.75, 0.25};

    Counter cycle_timer = {0, 150000000};
    Counter sprite_counter = {3, 4};
    Approach speed;

    states state = IDLE;
    states texture_version = WALKING;
    directions direction_moving = SOUTH;
    directions direction_facing = SOUTH;
    bool keep_moving = false;

    text_struct name;

    Player() = default;
    Player(std::array<double, 2> position, uint texture_id, std::string name);
    
    void render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size);

    void tick(uint delta);
};

struct texture_object {
    uint id;
    std::array<int, 3> data;
};

struct Setting {
    netwk::TCP_client& connection;

    Player player;
    std::unordered_map<uint64_t, Entity> entity_map;

    // screen parameters & render
    GLFWwindow* window;
    int width = 1000, height = 1000;

    std::array<double, 2> camera_pos;
    int scale = 48;

    Counter water_cycle_timer = {0, 800000000};
    Counter water_sprite_counter = {0, 4};

    text_struct version = {"\\c44fThe Simulation \\cfffpre-alpha \\cf660.0.\\x1", 2, 1000};

    // chunks
    std::unordered_map<uint, Chunk_data> loaded_chunks;
    std::array<uint, total_loaded_chunks> active_chunk_keys;    
    uint current_chunk = 0xffffffffu;

    bool check_if_moved_chunk();

    // chat
    game_activity current_activity = PLAY;

    std::string char_callback_string = "";
    std::list<text_struct> chat_list;
    int type_pos = 0; // position in string of cursor
    int type_start = 0; // position in string of leftmost displayed char
    int type_len = 0; // length of string in pixels
    int total_chat_lines = 0; // number of chat lines (rows of text) currently in the chat list
    int chat_line = 0; // topmost row displayed of chat messages

    int get_chat_list_lines();

    void process_input(std::string input, Player& player, std::list<text_struct>& chat_list, netwk::TCP_client& connection);

    void insert_chat_message(std::string input);

    /* textures & render
    texture ids in map:

    0 - tiles
    1 - text
    2 - avergreen
    3 - bald bandit
    4 - bandana bandit

    shader ids in map:

    0 - single square texture
    1 - text
    2 - chunk
    3 - chunk w/ depth (object tiles)
    4 - solid color
    */
    std::unordered_map<uint, texture_object> texture_map;
    std::unordered_map<uint, uint> shader_map;

    void load_textures_into_map();

    void create_shaders();

    std::unordered_map<int, float> object_tile_offset_values = {
        {VOIDTILE, 0.0f},
        {TALL_GRASS, 0.0625f}
    };

    // player
    std::map<int, bool> key_down_array = {
        {GLFW_KEY_W, false},
        {GLFW_KEY_S, false},
        {GLFW_KEY_A, false},
        {GLFW_KEY_D, false},
        {GLFW_KEY_SPACE, false},
        {GLFW_KEY_LEFT_SHIFT, false} // left shift makes keys sticky
    };

    bool send_chat_message = false; // sticky enter key

    std::array<int, 2> get_player_current_chunk();

    void set_player_enums(directions dir_moving, directions dir_facing);

    void set_player_enums();

    void process_key_inputs();

    // callbacks
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    static void char_callback(GLFWwindow* window, uint codepoint);

    Setting(GLFWwindow* window, netwk::TCP_client& connection_ref, std::array<double, 2> player_start_pos, std::string name);

    // game loop

    void send_join_packet();

    void send_positon_packet();

    void game_math(uint delta);

    void render();
};


// asio client

namespace netwk {
    TCP_client::TCP_client(unsigned short int target_port) : resolver(io_context), socket(io_context) { // connects the socket to the server
        auto endpoint = resolver.resolve("127.0.0.1", std::to_string(target_port));
        asio::connect(socket, endpoint);
    }

    void TCP_client::receive_loop(Setting& setting, bool& game_running) { // recieves incoming packets and does things with them based on header contents
        if(!game_running) socket.close();
        else {
            std::shared_ptr<std::array<char, sizeof(packet_header)>> buffer(new std::array<char, sizeof(packet_header)>);
            socket.async_read_some(asio::buffer(buffer->data(), buffer->size()),
                [this, buffer, &setting, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                    if(error_code) {
                        std::cout << "Error: " << error_code.message() << "\n";
                    } else {
                        packet_header header;
                        memcpy(&header, buffer->data(), bytes_transferred);

                        std::shared_ptr<std::vector<uint8_t>> buffer_body(new std::vector<uint8_t>(header.size_bytes));
                        socket.async_read_some(asio::buffer(buffer_body->data(), buffer_body->size()), 
                            [this, header_id = header.id, buffer_body, &setting, &game_running](const asio::error_code& error_code, size_t bytes_transferred) {
                                if(error_code) {
                                    std::cout << "Error: " << error_code.message() << "\n";
                                } else { // insert text into input collector to be processed by client     
                                    receive_loop(setting, game_running); // primes next recieve, this does NOT stall while waiting for new data from the server

                                    switch(header_id) {
                                        case 0: { // insert text into input collector to be processed by client
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (chat message)\n";
                                            std::string message = make_string(reinterpret_cast<char*>(buffer_body->data()), bytes_transferred);
                                            setting.insert_chat_message(message);

                                            break;
                                        }
                                        case 1: { // player joined
                                            std::cout << "Recieved " << bytes_transferred << " bytes from the server. (player joined)\n";
                                            player_join_packet_toclient recv_packet = from_byte_vector<player_join_packet_toclient>(buffer_body->data());

                                            text_struct txt_struct;
                                            txt_struct.set_values(make_string_array(recv_packet.name.data(), 256), 0x10000, 1);
                                            setting.entity_map.emplace(recv_packet.entity_packet.entity_id, Entity(recv_packet.entity_packet.entity_id, setting.texture_map[2].id, txt_struct, recv_packet.entity_packet.position, recv_packet.entity_packet.direction, setting.loaded_chunks));

                                            break;
                                        }
                                        case 2: { // player position packets
                                            //std::cout << "Recieved " << bytes_transferred << " bytes from the server. (player position)\n";

                                            entity_movement_packet_toclient recv_packet = from_byte_vector<entity_movement_packet_toclient>(buffer_body->data());
                                            if(setting.entity_map.contains(recv_packet.entity_id)) setting.entity_map.at(recv_packet.entity_id).insert_position(recv_packet);
                                            else {
                                                setting.entity_map.emplace(recv_packet.entity_id, Entity(recv_packet.entity_id, setting.texture_map[2].id, text_struct{}, recv_packet.position, recv_packet.direction, setting.loaded_chunks));
                                            }

                                            break;
                                        }
                                        case 3: {
                                            
                                        }
                                        default: { // the packet recieved did not have a valid header type if it reaches the default statement
                                            std::cout << "Recieved a packet with an invalid header type.\n";
                                        }
                                    }
                                }
                            }
                        );
                    }
                }
            );
        }
    }

    void TCP_client::start(Setting& setting, bool& game_running) { // starts the recieve loop
        recieve_thread = std::thread(
            [this, &setting, &game_running]() {
                this->receive_loop(setting, game_running);
                io_context.run();
            }
        );
    }

    void TCP_client::send(std::vector<uint8_t> packet) { // sends packet to server
        socket.async_write_some(asio::buffer(packet.data(), packet.size()), 
            [](const asio::error_code& error_code, size_t bytes) mutable {
                if(error_code) {
                    std::cout << "Failed to send message to the server.\n";
                }
            }
        );
    };
}


// client setting

Player::Player(std::array<double, 2> position, uint texture_id, std::string name) {
    this->position = position;
    this->texture_id = texture_id;
    speed.target = 0.0;
    this->name.set_values(name, 0x10000, 1);
}

void Player::render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size) {
    draw_tile(shader, texture_id, {float((position[0] - camera_pos[0] + visual_offset[0]) * scale), float((position[1] - camera_pos[1] + visual_offset[1]) * scale), visual_size[0] * scale, visual_size[1] * scale}, {active_sprite[0], active_sprite[1], 1, 1, 4, 8}, window_size, (position[1] - 0.125 - reference_y) * 0.01 + 0.1);
}

void Player::tick(uint delta) { // delta is in nanoseconds
    if(state != IDLE) {
        double change_x = 0.0;
        double change_y = 0.0;

        active_sprite[1] = direction_facing;
        
        switch(direction_moving) {
            case NORTH:
                change_y = 0.000000006 * delta;
                break;
            case NORTHEAST:
                change_x = 0.000000004243 * delta;
                change_y = 0.000000004243 * delta;
                break;
            case EAST:
                change_x = 0.000000006 * delta;
                break;
            case SOUTHEAST:
                change_x = 0.000000004243 * delta;
                change_y = -0.000000004243 * delta;
                break;
            case SOUTH:
                change_y = -0.000000006 * delta;
                break;
            case SOUTHWEST:
                change_x = -0.000000004243 * delta;
                change_y = -0.000000004243 * delta;
                break;
            case WEST:
                change_x = -0.000000006 * delta;
                break;
            case NORTHWEST:
                change_x = -0.000000004243 * delta;
                change_y = 0.000000004243 * delta;
                break;
        }
        
        switch(state) {
            case WALKING:
                texture_version = WALKING;
                sprite_counter.limit = 4;
                if(keep_moving) {
                    speed.target = 1.0;
                    speed.modify(0.00000001 * delta);
                } else {
                    speed.target = 0.0;
                    speed.modify(0.00000002 * delta);
                }
                if(cycle_timer.increment_by(delta)) {
                    sprite_counter.increment();
                }
                
                position[0] += change_x * speed.value;
                position[1] += change_y * speed.value;
                break;

            case RUNNING:
                texture_version = WALKING;
                sprite_counter.limit = 4;
                if(keep_moving) {
                    speed.target = 1.6;
                    speed.modify(0.00000001 * delta);
                } else {
                    speed.target = 0.0;
                    speed.modify(0.00000002 * delta);
                }
                if(cycle_timer.increment_by(delta * 1.3)) {
                    sprite_counter.increment();
                }

                position[0] += change_x * speed.value;
                position[1] += change_y * speed.value;
                break;

            case SWIMMING:
                texture_version = SWIMMING;
                sprite_counter.limit = 2;
                if(sprite_counter.value > 1) sprite_counter.value = 0;
                active_sprite[1] = direction_facing + 4;
                if(keep_moving) {
                    speed.target = 0.5;
                    speed.modify(0.000000005 * delta, 0.00000004 * delta);
                } else {
                    speed.target = 0.0;
                    speed.modify(0.00000004 * delta);
                }
                if(cycle_timer.increment_by(delta * 0.6)) {
                    sprite_counter.increment();
                }

                position[0] += change_x * speed.value;
                position[1] += change_y * speed.value;
                break;
        }

        active_sprite[0] = sprite_counter.value;
    } else {
        sprite_counter.value = 0;
        cycle_timer.value = 0;
        if(texture_version == WALKING) active_sprite[0] = 3;
        else if(texture_version == SWIMMING) active_sprite[0] = 1;
    }
}

bool Setting::check_if_moved_chunk() {
    uint chunk_key = (uint)(player.position[0] / 16) + (uint)(player.position[1] / 16) * world_size_chunks[0];
    if(chunk_key != current_chunk) {
        current_chunk = chunk_key;
        return true;
    }
    return false;
}

int Setting::get_chat_list_lines() {
    int lines = 0;
    for(text_struct message : chat_list) {
        lines += message.lines;
    }
    return lines;
}

void Setting::process_input(std::string input, Player& player, std::list<text_struct>& chat_list, netwk::TCP_client& connection) {
    if(input[0] == '*') { // command
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
                message.generate_data();
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
                message.generate_data();
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
            message.generate_data();
            chat_list.emplace_back(message);
            total_chat_lines += message.lines;
            if(chat_list.size() >= 25) {
                total_chat_lines -= chat_list.front().lines;
                chat_list.pop_front();
            }
        }
    } else { // chat
        if(input.size() != 0) {
            connection.send(netwk::make_packet(1, (void*)input.data(), input.size()));
        }
    }
}

void Setting::insert_chat_message(std::string input) {
    text_struct message;
    message.set_values(input, 600, 2);
    message.generate_data();
    total_chat_lines += message.lines;
    chat_list.emplace_back(message);

    if(chat_list.size() >= 25) {
        total_chat_lines -= chat_list.front().lines;
        chat_list.pop_front();
    }
    if(current_activity == PLAY) {
        chat_line = std::max(total_chat_lines - 12, 0);
    }
}

void Setting::load_textures_into_map() {
    std::vector<std::string> addresses = {
        "res\\tiles\\floor_tileset.png",
        "res\\gui\\textsource.png",
        "res\\entities\\Player\\avergreen_spritesheet.png",
        "res\\entities\\Bandit\\bandit0_spritesheet.png",
        "res\\entities\\Bandit\\bandit1_spritesheet.png"
    };

    for(uint i = 0; i < addresses.size(); ++i) {
        std::array<int, 3> tex_data;
        texture_map.emplace(i, texture_object{load_texture(addresses[i].data(), tex_data), tex_data});
    }
}

void Setting::create_shaders() {
    shader_map = {
        {0, create_shader(vertex_shader, fragment_shader)},
        {1, create_shader(vertex_shader_text, fragment_shader_text)},
        {2, create_shader(vertex_shader_chunk, fragment_shader_chunk)},
        {3, create_shader(vertex_shader_chunk_depth, fragment_shader_chunk)},
        {4, create_shader(vertex_shader_color, fragment_shader_color)}
    };
}

std::array<int, 2> Setting::get_player_current_chunk() {
    return {int(player.position[0] / 16), int(player.position[1] / 16)};
}

void Setting::set_player_enums(directions dir_moving, directions dir_facing) {
    player.direction_moving = dir_moving;
    player.direction_facing = dir_facing;
    player.keep_moving = true;

    std::vector<tile_ID> tiles_stepped_on = get_tiles_under(loaded_chunks, {player.walk_box[0] + player.position[0], player.walk_box[1] + player.position[1], player.walk_box[2], player.walk_box[3]});
    if(std::count(tiles_stepped_on.begin(), tiles_stepped_on.end(), WATER) == tiles_stepped_on.size()) 
        player.state = SWIMMING;
    else if(key_down_array[GLFW_KEY_SPACE]) 
        player.state = RUNNING;
    else 
        player.state = WALKING;
}

void Setting::set_player_enums() {
    player.keep_moving = false;

    if(player.speed.value == 0.0) 
        player.state = IDLE;
    else {
        std::vector<tile_ID> tiles_stepped_on = get_tiles_under(loaded_chunks, {player.walk_box[0] + player.position[0], player.walk_box[1] + player.position[1], player.walk_box[2], player.walk_box[3]});
        if(std::count(tiles_stepped_on.begin(), tiles_stepped_on.end(), WATER) == tiles_stepped_on.size()) 
            player.state = SWIMMING;
        else if(key_down_array[GLFW_KEY_SPACE]) 
            player.state = RUNNING;
        else 
            player.state = WALKING;
    }
}

void Setting::process_key_inputs() {
    bool check_last_key_pressed = true;
    if(key_down_array[GLFW_KEY_W]) {
        if(key_down_array[GLFW_KEY_D]) 
            set_player_enums(NORTHEAST, EAST);
        else if(key_down_array[GLFW_KEY_S]) 
            set_player_enums();
        else if(key_down_array[GLFW_KEY_A]) 
            set_player_enums(NORTHWEST, WEST);
        else
            set_player_enums(NORTH, NORTH);
    } else if(key_down_array[GLFW_KEY_D]) {
        if(key_down_array[GLFW_KEY_S])
            set_player_enums(SOUTHEAST, EAST);
        else if(key_down_array[GLFW_KEY_A])
            set_player_enums();
        else
            set_player_enums(EAST, EAST);
    } else if(key_down_array[GLFW_KEY_S]) {
        if(key_down_array[GLFW_KEY_A])
            set_player_enums(SOUTHWEST, WEST);
        else
            set_player_enums(SOUTH, SOUTH);
    } else if(key_down_array[GLFW_KEY_A])
        set_player_enums(WEST, WEST);
    else
        set_player_enums();
}

void Setting::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Setting* setting = (Setting*)(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS) {
        if(setting->current_activity == PLAY) {
            if(setting->key_down_array.contains(key)) {
                setting->key_down_array[key] = true;
                //if(key == GLFW_KEY_W || key == GLFW_KEY_D || key == GLFW_KEY_S || key == GLFW_KEY_A) last_direction_key_pressed = key;
            }
            else if(key == GLFW_KEY_R) {
                setting->chat_line = 0;
                setting->total_chat_lines = 0;
                setting->chat_list.clear();
            } else if(key == GLFW_KEY_N) {
                setting->loaded_chunks.clear();
                setting->current_chunk = 0xffffffffu;
            } else if(key == GLFW_KEY_T) {
                setting->current_activity = CHAT;
                setting->char_callback_string = "";
            }
        } else if(setting->current_activity == CHAT) {
            if(key == GLFW_KEY_BACKSPACE) {
                if(setting->char_callback_string.size() != 0 && setting->type_pos != 0) {
                    int char_pos = setting->type_pos - 1;
                    if(standard_chars.contains(setting->char_callback_string[char_pos])) setting->type_len -= standard_chars[setting->char_callback_string[char_pos]][1] * 2 + 2;
                    setting->char_callback_string.erase(setting->char_callback_string.begin() + char_pos);
                    --setting->type_pos;
                    if(setting->type_pos < setting->type_start) {
                        --setting->type_start;
                        setting->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_ESCAPE) {
                setting->current_activity = PLAY;
                setting->type_pos = 0;
                setting->type_start = 0;
                setting->type_len = 0;
                setting->chat_line = std::max(setting->total_chat_lines - 12, 0);
                setting->char_callback_string = "";
            } else if(key == GLFW_KEY_ENTER) {
                setting->send_chat_message = true;
            } else if(key == GLFW_KEY_LEFT) {
                if(setting->char_callback_string.size() != 0 && setting->type_pos != 0) {
                    --setting->type_pos;
                    if(standard_chars.contains(setting->char_callback_string[setting->type_pos])) setting->type_len -= standard_chars[setting->char_callback_string[setting->type_pos]][1] * 2 + 2;
                    if(setting->type_pos < setting->type_start) {
                        --setting->type_start;
                        setting->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_RIGHT) {
                if(setting->char_callback_string.size() != 0 && setting->type_pos < setting->char_callback_string.size()) {
                    if(standard_chars.contains(setting->char_callback_string[setting->type_pos])) setting->type_len += standard_chars[setting->char_callback_string[setting->type_pos]][1] * 2 + 2;
                    ++setting->type_pos;
                    while(setting->type_len >= setting->width - 20) {
                        if(standard_chars.contains(setting->char_callback_string[setting->type_start])) setting->type_len -= standard_chars[setting->char_callback_string[setting->type_start]][1] * 2 + 2;
                        ++setting->type_start;
                    }
                }
            }
        }
    } else if(action == GLFW_RELEASE) {
        if(setting->current_activity == PLAY && (!setting->key_down_array[GLFW_KEY_LEFT_SHIFT] || key == GLFW_KEY_LEFT_SHIFT)) {
            if(setting->key_down_array.find(key) != setting->key_down_array.end()) setting->key_down_array[key] = false;
        }
    } else if(action == GLFW_REPEAT) {
        if(setting->current_activity == CHAT) {
            if(key == GLFW_KEY_BACKSPACE) {
                if(setting->char_callback_string.size() != 0 && setting->type_pos != 0) {
                    int char_pos = setting->type_pos - 1;
                    if(standard_chars.contains(setting->char_callback_string[char_pos])) setting->type_len -= standard_chars[setting->char_callback_string[char_pos]][1] * 2 + 2;
                    setting->char_callback_string.erase(setting->char_callback_string.begin() + char_pos);
                    --setting->type_pos;
                    if(setting->type_pos < setting->type_start) {
                        --setting->type_start;
                        setting->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_LEFT) {
                if(setting->char_callback_string.size() != 0 && setting->type_pos != 0) {
                    --setting->type_pos;
                    if(standard_chars.contains(setting->char_callback_string[setting->type_pos])) setting->type_len -= standard_chars[setting->char_callback_string[setting->type_pos]][1] * 2 + 2;
                    if(setting->type_pos < setting->type_start) {
                        --setting->type_start;
                        setting->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_RIGHT) {
                if(setting->char_callback_string.size() != 0 && setting->type_pos < setting->char_callback_string.size()) {
                    if(standard_chars.contains(setting->char_callback_string[setting->type_pos])) setting->type_len += standard_chars[setting->char_callback_string[setting->type_pos]][1] * 2 + 2;
                    ++setting->type_pos;
                    while(setting->type_len >= setting->width - 20) {
                        if(standard_chars.contains(setting->char_callback_string[setting->type_start])) setting->type_len -= standard_chars[setting->char_callback_string[setting->type_start]][1] * 2 + 2;
                        ++setting->type_start;
                    }
                }
            }
        }
    }
}

void Setting::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Setting* setting = (Setting*)(glfwGetWindowUserPointer(window));
    if(setting->current_activity == PLAY) {
        if(yoffset > 0) {
            if(setting->scale < 64) setting->scale += 16;
        } else {
            if(setting->scale > 16) setting->scale -= 16;
        }
    } else if(setting->current_activity == CHAT) {
        if(yoffset > 0) {
            if(setting->chat_line >= 4) setting->chat_line -= 4;
            else setting->chat_line = 0;
        } else {
            if(setting->chat_line <= setting->get_chat_list_lines() - 16) setting->chat_line += 4;
            else setting->chat_line = setting->get_chat_list_lines() - 12;
        }
    }
}

void Setting::char_callback(GLFWwindow* window, uint codepoint) {
    Setting* setting = (Setting*)(glfwGetWindowUserPointer(window));
    if(setting->current_activity == CHAT) {
        if(standard_chars.contains(codepoint)) setting->type_len += standard_chars[codepoint][1] * 2 + 2;
        setting->char_callback_string.insert(setting->char_callback_string.begin() + setting->type_pos, codepoint);
        ++setting->type_pos;
        while(setting->type_len >= setting->width - 20) {
            if(standard_chars.contains(setting->char_callback_string[setting->type_start])) setting->type_len -= standard_chars[setting->char_callback_string[setting->type_start]][1] * 2 + 2;
            ++setting->type_start;
        }
    }
}

Setting::Setting(GLFWwindow* window, netwk::TCP_client& connection_ref, std::array<double, 2> player_start_pos, std::string name) : connection(connection_ref) {
    load_textures_into_map();
    create_shaders();
    player = Player(player_start_pos, texture_map[2].id, name);
    version.generate_data();
    this->window = window;
}

// game loop

void Setting::send_join_packet() {
    std::array<char, 64> player_name_array;
    memcpy(player_name_array.data(), player.name.str.data(), player.name.str.size() + 1);
    netwk::player_join_packet_toserv join_packet{player_name_array, {player.position, player.direction_facing}};
    connection.send(netwk::make_packet(0, (void*)&join_packet, sizeof(join_packet)));
}

void Setting::send_positon_packet() {
    netwk::entity_movement_packet_toserv send_packet{player.position, player.direction_facing};
    connection.send(netwk::make_packet(2, (void*)&send_packet, sizeof(send_packet)));
}

void Setting::game_math(uint delta) {
    glfwGetFramebufferSize(window, &width, &height);
    width = width + 1 * (width % 2 == 1);
    height = height + 1 * (height % 2 == 1);
    glViewport(0, 0, width, height);

    glfwPollEvents();

    process_key_inputs();

    if(send_chat_message) {
        process_input(char_callback_string, player, chat_list, connection);
        char_callback_string = "";
        current_activity = PLAY;
        send_chat_message = false;
        type_pos = 0;
        type_start = 0;
        type_len = 0;
        chat_line = std::max(total_chat_lines - 12, 0);
    }

    player.tick(delta);

    for(auto& [key, entity] : entity_map) {
        entity.tick(delta);
    }

    camera_pos = {player.position[0], player.position[1] + 0.875};

    if(water_cycle_timer.increment_by(delta)) water_sprite_counter.increment();
}

void Setting::render() {
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int reference_y = int(player.position[1] / 16) * 16 - 32;

    for(int i = 0; i < total_loaded_chunks; ++i) {
        uint key = active_chunk_keys[i];
        
        if(loaded_chunks.contains(key)) {
            Chunk_data& chunk = loaded_chunks[key];
            
            double x_corner = (chunk.corner[0] - camera_pos[0]) * scale;
            double y_corner = (chunk.corner[1] - camera_pos[1]) * scale;

            if(rect_collision({x_corner, y_corner, 16.0 * scale, 16.0 * scale}, {-width * 0.5, -height * 0.5, double(width), double(height)})) {
                glBindTexture(GL_TEXTURE_2D, texture_map[0].id);

                glUseProgram(shader_map[3]);
                glUniform2f(0, width, height);
                glUniform2f(4, scale, scale);
                glUniform2f(6, 8.0, 16.0);
                glUniform2f(2, x_corner, y_corner);
                glUniform1f(8, round((chunk.corner[1] - reference_y)) * 0.01 + 0.1);

                for(int j = 0; j < 256; ++j) {
                    glUniform1i(9 + j, chunk.object_tiles[j].texture);
                    glUniform1f(265 + j, object_tile_offset_values[chunk.object_tiles[j].id]);
                }

                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 256);

                // objects
                glUseProgram(shader_map[2]);
                glUniform2f(0, width, height);
                glUniform2f(4, scale, scale);
                glUniform2f(6, 8.0, 16.0);
                glUniform2f(2, x_corner, y_corner);

                for(int j = 0; j < 256; ++j) {
                    int floor_tex = chunk.floor_tiles[j].texture;
                    if(floor_tex >= 56) floor_tex += water_sprite_counter.value;
                    glUniform1i(8 + j, floor_tex);
                }

                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 256);
            }
        }
    }

    player.render(reference_y, camera_pos, scale, shader_map[0], {width, height});

    for(auto& [key, entity] : entity_map) {
        entity.render(reference_y, camera_pos, scale, shader_map[0], {width, height});
    }

    // render coords & version
    std::string hex_coords = to_hex_string(player.position[0]) + " " + to_hex_string(player.position[1]);
    std::string dec_coords = std::to_string(int(player.position[0])) + " " + std::to_string(int(player.position[1]));

    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(width / 2 - 10 - (version.generate_data() * 2)), float(height / 2 - 64)}, version.str, 2, 1000);
    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(width / 2 - 10 - ((hex_coords.size() - 4) * 7 - 3) * 2), float(height / 2 - 90)}, hex_coords, 2, 500);
    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(width / 2 - 10 - (dec_coords.size() * 7 - 3) * 2), float(height / 2 - 116)}, dec_coords, 2, 500);

    if(current_activity == CHAT) {
        render_text_type(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(height / 2 - 34)}, char_callback_string.substr(type_start), 2, type_pos - type_start, width);
        
        glUseProgram(shader_map[4]);
        glUniform1f(0, 0.001f);
        glUniform2f(1, width, height);
        glUniform4f(3, -width / 2, height / 2 - 37, width, 37);
        glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    int text_scale = 0.0625 * (scale - 16);
    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-player.name.generate_data() / 2 * text_scale), float(0.875 * scale)}, player.name.str, text_scale, 1000);

    int text_y_pos = height / 2 - 64;
    int chat_line_counter = 0;
    for(text_struct message : chat_list) {
        if(chat_line_counter >= chat_line) {
            int lines_displayed = chat_line_counter - chat_line;
            if(lines_displayed < 12) {
                int lines_passed = render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(0, (12 - lines_displayed < message.lines) ? message.first_chars[12 - lines_displayed] : message.str.length()), 2, 600);
                text_y_pos -= lines_passed * 26;
                chat_line_counter += lines_passed;
            }
        } else {
            chat_line_counter += message.lines;
            if(chat_line_counter > chat_line) {
                int num = message.lines - (chat_line_counter - chat_line);
                int lines_passed = render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(message.first_chars[num]), 2, 600);
                text_y_pos -= lines_passed * 26;
            }
        }
    }

    if(chat_list.size() != 0) {
        glUseProgram(shader_map[4]);
        glUniform1f(0, 0.001f);
        glUniform2f(1, width, height);
        glUniform4f(3, -width / 2, text_y_pos + 26 - 5, 620, height / 2 - text_y_pos - 56);
        glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glfwSwapBuffers(window);
}