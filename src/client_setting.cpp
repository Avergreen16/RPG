#pragma once
#include "client_setting.hpp"

/*template<typename type0, template type1>
bool check_point_inclusion(std::array<type0, 4> intrect, std::array<type1, 2> point) {
    if(point[0] >= intrect[0] && point[0] < intrect[0] + intrect[2] && point[1] >= intrect[1] && point[1] < intrect[1] + intrect[3]) return true;
    return false;
}*/

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
                    speed.modify(0.01 * delta);
                } else {
                    speed.target = 0.0;
                    speed.modify(0.02 * delta);
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
                    speed.modify(0.01 * delta);
                } else {
                    speed.target = 0.0;
                    speed.modify(0.02 * delta);
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
                    speed.modify(0.005 * delta, 0.04 * delta);
                } else {
                    speed.target = 0.0;
                    speed.modify(0.04 * delta);
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
        texture_map.emplace(i, texture_object{load_texture(addresses[0].data(), tex_data), tex_data});
    }
}

void Setting::create_shaders() {
    shader_map = {
        {1, create_shader(vertex_shader, fragment_shader)},
        {2, create_shader(vertex_shader_text, fragment_shader_text)},
        {3, create_shader(vertex_shader_chunk, fragment_shader_chunk)},
        {4, create_shader(vertex_shader_chunk_depth, fragment_shader_chunk)},
        {5, create_shader(vertex_shader_color, fragment_shader_color)}
    };
}

std::array<int, 2> Setting::get_player_current_chunk() {
    return {int(player.position[0] / 16), int(player.position[1] / 16)};
}

void Setting::set_player_enums(directions dir_moving, directions dir_facing) {
    player.direction_moving = NORTHEAST;
    player.direction_facing = EAST;
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

static void Setting::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

static void Setting::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
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

static void Setting::char_callback(GLFWwindow* window, uint codepoint) {
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

Setting::Setting(GLFWwindow* window, netwk::TCP_client& connection_ref, std::array<double, 2> player_start_pos, std::string name) : player(player_start_pos, texture_map[2].id, name), connection(connection_ref) {
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

    render_text(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(width / 2 - 10 - (version.generate_data() * 2)), float(height / 2 - 64)}, version.str, 2, 1000);
    render_text(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(width / 2 - 10 - ((hex_coords.size() - 4) * 7 - 3) * 2), float(height / 2 - 90)}, hex_coords, 2, 500);
    render_text(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(width / 2 - 10 - (dec_coords.size() * 7 - 3) * 2), float(height / 2 - 116)}, dec_coords, 2, 500);

    if(current_activity == CHAT) {
        render_text_type(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(height / 2 - 34)}, char_callback_string.substr(type_start), 2, type_pos - type_start, width);
        
        glUseProgram(shader_map[4]);
        glUniform1f(0, 0.001f);
        glUniform2f(1, width, height);
        glUniform4f(3, -width / 2, height / 2 - 37, width, 37);
        glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    int text_scale = 0.0625 * (scale - 16);
    render_text(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(-player.name.generate_data() / 2 * text_scale), float(0.875 * scale)}, player.name.str, text_scale, 1000);

    int text_y_pos = height / 2 - 64;
    int chat_line_counter = 0;
    for(text_struct message : chat_list) {
        if(chat_line_counter >= chat_line) {
            int lines_displayed = chat_line_counter - chat_line;
            if(lines_displayed < 12) {
                int lines_passed = render_text(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(0, (12 - lines_displayed < message.lines) ? message.first_chars[12 - lines_displayed] : message.str.length()), 2, 600);
                text_y_pos -= lines_passed * 26;
                chat_line_counter += lines_passed;
            }
        } else {
            chat_line_counter += message.lines;
            if(chat_line_counter > chat_line) {
                int num = message.lines - (chat_line_counter - chat_line);
                int lines_passed = render_text(shader_map[1], text_img, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(message.first_chars[num]), 2, 600);
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