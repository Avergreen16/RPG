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

#include "render.cpp"
#include "worldgen.cpp"
#include "pathfinding.cpp"
#include "text.cpp"
#include "entities.cpp"

namespace netwk {
    struct TCP_client;
}
struct Setting;

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
    Setting* setting;
    uint texture_id;

    std::array<double, 2> position;
    std::array<Approach, 2> velocity;

    std::array<float, 2> visual_size = {2, 2};
    std::array<float, 2> visual_offset = {-1, -0.125};
    std::array<int, 2> sprite_size = {32, 32};
    std::array<int, 2> active_sprite = {3, 2};
    std::array<double, 4> walk_box = {-0.375, -0.125, 0.75, 0.25};

    Counter cycle_timer = {0, 150000000};
    Counter sprite_counter = {3, 4};

    states state = IDLE;
    states texture_version = WALKING;
    directions direction_moving = SOUTH;
    directions direction_facing = SOUTH;
    bool keep_moving = false;

    int max_health = 8;
    int health = 8;

    text_struct name;

    Player() = default;
    Player(Setting* setting_ptr, std::array<double, 2> position, uint texture_id, std::string name);
    
    void render(int reference_y, std::array<double, 2> camera_pos, float scale, uint shader, std::array<int, 2> window_size);

    void tick(uint delta);

    void health_change(uint change);
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

    std::string chat_input_string = "";
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

    void update_movement(directions dir_moving, directions dir_facing);

    void update_movement();

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

    void render_gui();
};