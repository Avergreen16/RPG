#pragma once
#define _USE_MATH_DEFINES

#include "asio_client.hpp"
#include "client_core.hpp"

/*template<typename type0, template type1>
bool check_point_inclusion(std::array<type0, 4> intrect, std::array<type1, 2> point) {
    if(point[0] >= intrect[0] && point[0] < intrect[0] + intrect[2] && point[1] >= intrect[1] && point[1] < intrect[1] + intrect[3]) return true;
    return false;
}*/

Player::Player(Core* core_ptr, std::array<double, 2> position, uint texture_id, std::string name) {
    this->core = core_ptr;
    this->position = position;
    this->texture_id = texture_id;
    velocity[0].target = 0.0;
    velocity[1].target = 0.0;
    this->name.set_values(name, 0x10000, 1);
}

void Player::render(int reference_y, glm::vec2 camera_pos, float scale, uint shader, std::array<int, 2> window_size) {
    draw_tile(shader, texture_id, {float((position[0] - camera_pos.x + visual_offset[0]) * scale), float((position[1] - camera_pos.y + visual_offset[1]) * scale), visual_size[0] * scale, visual_size[1] * scale}, {active_sprite[0], active_sprite[1], 1, 1, 4, 8}, window_size, (position[1] - 0.125 - reference_y) * 0.01 + 0.1);
}

void Player::tick(uint delta) { // delta is in nanoseconds
    std::vector<tile_ID> tiles_stepped_on = get_tiles_under(core->loaded_chunks, {walk_box[0] + position[0], walk_box[1] + position[1], walk_box[2], walk_box[3]});
    if(std::count(tiles_stepped_on.begin(), tiles_stepped_on.end(), WATER) == tiles_stepped_on.size()) {
        texture_version = SWIMMING;
        sprite_counter.set_limit(2);
        if(state != IDLE)
            state = SWIMMING;
    } else if(core->key_down_array[GLFW_KEY_SPACE]) {
        texture_version = WALKING;
        sprite_counter.set_limit(4);
        if(state != IDLE)
            state = RUNNING;
    } else {
        texture_version = WALKING;
        sprite_counter.set_limit(4);
        if(state != IDLE)
            state = WALKING;
    }
    
    
    double change_x = 0.0;
    double change_y = 0.0;

    if(state != IDLE) {
        switch(direction_moving) {
            case NORTH:
                change_y = 1.0;
                break;
            case NORTHEAST:
                change_x = M_SQRT1_2;
                change_y = M_SQRT1_2;
                break;
            case EAST:
                change_x = 1.0;
                break;
            case SOUTHEAST:
                change_x = M_SQRT1_2;
                change_y = -M_SQRT1_2;
                break;
            case SOUTH:
                change_y = -1.0;
                break;
            case SOUTHWEST:
                change_x = -M_SQRT1_2;
                change_y = -M_SQRT1_2;
                break;
            case WEST:
                change_x = -1.0;
                break;
            case NORTHWEST:
                change_x = -M_SQRT1_2;
                change_y = M_SQRT1_2;
                break;
        }
    }
        
    switch(state) {
        case IDLE: {
            cycle_timer.value = cycle_timer.limit - 1;
            if(texture_version == WALKING) sprite_counter.value = 3;
            else if(texture_version == SWIMMING) sprite_counter.value = 1;
            break;
        }
        case WALKING: {
            active_sprite[1] = direction_facing;
            if(keep_moving) {
                velocity[0].target = change_x;
                velocity[1].target = change_y;
                velocity[0].modify(0.000000005 * delta);
                velocity[1].modify(0.000000005 * delta);
            } else {
                velocity[0].target = 0.0;
                velocity[1].target = 0.0;
                velocity[0].modify(0.00000001 * delta);
                velocity[1].modify(0.00000001 * delta);
            }
            if(cycle_timer.increment_by(delta)) {
                sprite_counter.increment();
            }
            
            position[0] += 0.000000006 * delta * velocity[0].value;
            position[1] += 0.000000006 * delta * velocity[1].value;
            break;
        }
        case RUNNING: {
            active_sprite[1] = direction_facing;
            if(keep_moving) {
                velocity[0].target = change_x;
                velocity[1].target = change_y;
                velocity[0].modify(0.000000005 * delta);
                velocity[1].modify(0.000000005 * delta);
            } else {
                velocity[0].target = 0.0;
                velocity[1].target = 0.0;
                velocity[0].modify(0.00000001 * delta);
                velocity[1].modify(0.00000001 * delta);
            }
            if(cycle_timer.increment_by(delta * 1.3)) {
                sprite_counter.increment();
            }
            
            position[0] += 0.00000001 * delta * velocity[0].value;
            position[1] += 0.00000001 * delta * velocity[1].value;
            break;
        }
        case SWIMMING: {
            active_sprite[1] = direction_facing + 4;
            if(keep_moving) {
                velocity[0].target = change_x;
                velocity[1].target = change_y;
                velocity[0].modify(0.000000005 * delta);
                velocity[1].modify(0.000000005 * delta);
            } else {
                velocity[0].target = 0.0;
                velocity[1].target = 0.0;
                velocity[0].modify(0.00000001 * delta);
                velocity[1].modify(0.00000001 * delta);
            }
            if(cycle_timer.increment_by(delta * 0.6)) {
                sprite_counter.increment();
            }
            
            position[0] += 0.000000003 * delta * velocity[0].value;
            position[1] += 0.000000003 * delta * velocity[1].value;
            break;
        }
    }

    active_sprite[0] = sprite_counter.value;
}

void Player::health_change(uint change) {
    health += change;
    if(health > max_health) health = max_health;
    else if(health < 0) health = 0;
}

bool Core::check_if_moved_chunk() {
    uint chunk_key = (uint)(player.position[0] / 16) + (uint)(player.position[1] / 16) * world_size_chunks[0];
    if(chunk_key != current_chunk) {
        current_chunk = chunk_key;
        return true;
    }
    return false;
}

int Core::get_chat_list_lines() {
    int lines = 0;
    for(text_struct message : chat_list) {
        lines += message.lines;
    }
    return lines;
}

void Core::process_input(std::string input, Player& player, std::list<text_struct>& chat_list, netwk::TCP_client& connection) {
    if(input[0] == '*') { // command
        if(strcmp(input.substr(1, 4).data(), "tpm ") == 0) {
            std::string x = "";
            std::string y = "";
            
            int x_rad = 10;
            int y_rad = 10;
            bool x_digits = false;
            bool y_digits = false;

            int pos = 5;
            if(strcmp(input.substr(5, 2).data(), "\\x") == 0) {
                x_rad = 16;
                pos += 2;
                for(char c : input.substr(pos)) {
                    pos++;
                    if(std::find(valid_hex_numbers.begin(), valid_hex_numbers.end(), c) == valid_hex_numbers.end()) {
                        break;
                    } else {
                        x_digits = true;
                        x += c;
                    }
                }
            } else {
                for(char c : input.substr(pos)) {
                    pos++;
                    if(std::find(valid_numbers.begin(), valid_numbers.end(), c) == valid_numbers.end()) {
                        break;
                    } else {
                        x_digits = true;
                        x += c;
                    }
                }
            }
            if(strcmp(input.substr(pos, 2).data(), "\\x") == 0) {
                y_rad = 16;
                pos += 2;
                for(char c : input.substr(pos)) {
                    if(std::find(valid_hex_numbers.begin(), valid_hex_numbers.end(), c) == valid_hex_numbers.end()) {
                        break;
                    } else {
                        y_digits = true;
                        y += c;
                    }
                }
            } else {
                for(char c : input.substr(pos)) {
                    pos++;
                    if(std::find(valid_numbers.begin(), valid_numbers.end(), c) == valid_numbers.end()) {
                        break;
                    } else {
                        y_digits = true;
                        y += c;
                    }
                }
            }

            int x_val = strtoul(x.data(), nullptr, x_rad);
            int y_val = strtoul(y.data(), nullptr, y_rad);
            if(x_val < 0 || x_val >= world_size[0] || y_val < 0 || y_val >= world_size[1] || !x_digits || !y_digits) {
                insert_chat_message("Invalid parameters. (*tpm)");
            } else {
                player.position = {x_val + 0.5, y_val + 0.25};
                insert_chat_message("Teleported " + player.name.str + "\\cfff to coordinates: " + std::to_string(x_val) + " " + std::to_string(y_val));
            }
        } else if(strcmp(input.substr(1, 8).data(), "hchange ") == 0) {
            std::string change = "";
            bool digits = false;
            uint rad = 10;

            uint pos = 9;
            if(strcmp(input.substr(9, 2).data(), "\\x") == 0) {
                rad = 16;
                pos += 2;
                for(char c : input.substr(pos)) {
                    pos++;
                    if(std::find(valid_hex_numbers.begin(), valid_hex_numbers.end(), c) == valid_hex_numbers.end()) {
                        break;
                    } else {
                        digits = true;
                        change += c;
                    }
                }
            } else {
                for(char c : input.substr(pos)) {
                    pos++;
                    if(std::find(valid_numbers.begin(), valid_numbers.end(), c) == valid_numbers.end()) {
                        break;
                    } else {
                        digits = true;
                        change += c;
                    }
                }
            }

            if(digits) {
                int change_val = strtol(change.data(), nullptr, rad);
                player.health_change(change_val);
                if(change_val < 0) {
                    insert_chat_message(player.name.str + " \\cffftook " + std::to_string(change_val * -1) + " damage.");
                } else {
                    insert_chat_message(player.name.str + " \\cfffgained " + std::to_string(change_val) + " health.");
                }
            } else {
                insert_chat_message("Invalid parameters. (*hchange)");
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

void Core::insert_chat_message(std::string input) {
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

void Core::load_textures_into_map() {
    std::vector<std::string> addresses = {
        "res\\tiles\\floor_tileset.png",
        "res\\gui\\textsource.png",
        "res\\entities\\Player\\avergreen_spritesheet.png",
        "res\\entities\\Bandit\\bandit0_spritesheet.png",
        "res\\entities\\Bandit\\bandit1_spritesheet.png",
        "res\\gui\\display.png"
    };

    for(uint i = 0; i < addresses.size(); ++i) {
        std::array<int, 3> tex_data;
        texture_map.emplace(i, texture_object{load_texture(addresses[i].data(), tex_data), tex_data});
    }
}

void Core::create_shaders() {
    shader_map = {
        {0, create_shader(vertex_shader, fragment_shader)},
        {1, create_shader(vertex_shader_text, fragment_shader_text)},
        {2, create_shader(vertex_shader_chunk, fragment_shader_chunk)},
        {3, create_shader(vertex_shader_chunk_depth, fragment_shader_chunk)},
        {4, create_shader(vertex_shader_color, fragment_shader_color)},
        {5, create_shader(v_src, f_src)}
    };
}

std::array<int, 2> Core::get_player_current_chunk() {
    return {int(player.position[0] / 16), int(player.position[1] / 16)};
}

void Core::update_movement(directions dir_moving, directions dir_facing) {
    player.direction_moving = dir_moving;
    player.direction_facing = dir_facing;
    player.keep_moving = true;

    player.state = WALKING;
}

void Core::update_movement() {
    player.keep_moving = false;

    if(player.velocity[0].value == 0.0 && player.velocity[1].value == 0.0) 
        player.state = IDLE;
}

void Core::process_key_inputs() {
    bool check_last_key_pressed = true;
    if(key_down_array[GLFW_KEY_W]) {
        if(key_down_array[GLFW_KEY_D]) 
            update_movement(NORTHEAST, EAST);
        else if(key_down_array[GLFW_KEY_S]) 
            update_movement();
        else if(key_down_array[GLFW_KEY_A]) 
            update_movement(NORTHWEST, WEST);
        else
            update_movement(NORTH, NORTH);
    } else if(key_down_array[GLFW_KEY_D]) {
        if(key_down_array[GLFW_KEY_S])
            update_movement(SOUTHEAST, EAST);
        else if(key_down_array[GLFW_KEY_A])
            update_movement();
        else
            update_movement(EAST, EAST);
    } else if(key_down_array[GLFW_KEY_S]) {
        if(key_down_array[GLFW_KEY_A])
            update_movement(SOUTHWEST, WEST);
        else
            update_movement(SOUTH, SOUTH);
    } else if(key_down_array[GLFW_KEY_A])
        update_movement(WEST, WEST);
    else
        update_movement();
}

void Core::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Core* core = (Core*)(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS) {
        if(core->current_activity == PLAY) {
            if(core->key_down_array.contains(key)) {
                core->key_down_array[key] = true;
                //if(key == GLFW_KEY_W || key == GLFW_KEY_D || key == GLFW_KEY_S || key == GLFW_KEY_A) last_direction_key_pressed = key;
            }
            else if(key == GLFW_KEY_R) {
                core->chat_line = 0;
                core->total_chat_lines = 0;
                core->chat_list.clear();
            } else if(key == GLFW_KEY_N) {
                core->loaded_chunks.clear();
                core->current_chunk = 0xffffffffu;
            } else if(key == GLFW_KEY_T) {
                core->current_activity = CHAT;
                core->chat_input_string = "";
            }
        } else if(core->current_activity == CHAT) {
            if(key == GLFW_KEY_BACKSPACE) {
                if(core->chat_input_string.size() != 0 && core->type_pos != 0) {
                    int char_pos = core->type_pos - 1;
                    if(standard_chars.contains(core->chat_input_string[char_pos])) core->type_len -= 6 * 2 + 2;
                    core->chat_input_string.erase(core->chat_input_string.begin() + char_pos);
                    --core->type_pos;
                    if(core->type_pos < core->type_start) {
                        --core->type_start;
                        core->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_ESCAPE) {
                core->current_activity = PLAY;
                core->type_pos = 0;
                core->type_start = 0;
                core->type_len = 0;
                core->chat_line = std::max(core->total_chat_lines - 12, 0);
                core->chat_input_string = "";
            } else if(key == GLFW_KEY_ENTER) {
                core->send_chat_message = true;
            } else if(key == GLFW_KEY_LEFT) {
                if(core->chat_input_string.size() != 0 && core->type_pos != 0) {
                    --core->type_pos;
                    if(standard_chars.contains(core->chat_input_string[core->type_pos])) core->type_len -= 6 * 2 + 2;
                    if(core->type_pos < core->type_start) {
                        --core->type_start;
                        core->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_RIGHT) {
                if(core->chat_input_string.size() != 0 && core->type_pos < core->chat_input_string.size()) {
                    if(standard_chars.contains(core->chat_input_string[core->type_pos])) core->type_len += 6 * 2 + 2;
                    ++core->type_pos;
                    while(core->type_len >= core->width - 20) {
                        if(standard_chars.contains(core->chat_input_string[core->type_start])) core->type_len -= 6 * 2 + 2;
                        ++core->type_start;
                    }
                }
            }
        }
    } else if(action == GLFW_RELEASE) {
        if(core->current_activity == PLAY && (!core->key_down_array[GLFW_KEY_LEFT_SHIFT] || key == GLFW_KEY_LEFT_SHIFT)) {
            if(core->key_down_array.find(key) != core->key_down_array.end()) core->key_down_array[key] = false;
        }
    } else if(action == GLFW_REPEAT) {
        if(core->current_activity == CHAT) {
            if(key == GLFW_KEY_BACKSPACE) {
                if(core->chat_input_string.size() != 0 && core->type_pos != 0) {
                    int char_pos = core->type_pos - 1;
                    if(standard_chars.contains(core->chat_input_string[char_pos])) core->type_len -= 6 * 2 + 2;
                    core->chat_input_string.erase(core->chat_input_string.begin() + char_pos);
                    --core->type_pos;
                    if(core->type_pos < core->type_start) {
                        --core->type_start;
                        core->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_LEFT) {
                if(core->chat_input_string.size() != 0 && core->type_pos != 0) {
                    --core->type_pos;
                    if(standard_chars.contains(core->chat_input_string[core->type_pos])) core->type_len -= 6 * 2 + 2;
                    if(core->type_pos < core->type_start) {
                        --core->type_start;
                        core->type_len = 0;
                    }
                }
            } else if(key == GLFW_KEY_RIGHT) {
                if(core->chat_input_string.size() != 0 && core->type_pos < core->chat_input_string.size()) {
                    if(standard_chars.contains(core->chat_input_string[core->type_pos])) core->type_len += 6 * 2 + 2;
                    ++core->type_pos;
                    while(core->type_len >= core->width - 20) {
                        if(standard_chars.contains(core->chat_input_string[core->type_start])) core->type_len -= 6 * 2 + 2;
                        ++core->type_start;
                    }
                }
            }
        }
    }
}

void Core::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Core* core = (Core*)(glfwGetWindowUserPointer(window));
    if(core->current_activity == PLAY) {
        if(yoffset > 0) {
            if(core->scale < 64) core->scale += 16;
        } else {
            if(core->scale > 16) core->scale -= 16;
        }
    } else if(core->current_activity == CHAT) {
        if(yoffset > 0) {
            if(core->chat_line >= 4) core->chat_line -= 4;
            else core->chat_line = 0;
        } else {
            if(core->chat_line <= core->get_chat_list_lines() - 16) core->chat_line += 4;
            else core->chat_line = core->get_chat_list_lines() - 12;
        }
    }
}

void Core::char_callback(GLFWwindow* window, uint codepoint) {
    Core* core = (Core*)(glfwGetWindowUserPointer(window));
    if(core->current_activity == CHAT) {
        if(standard_chars.contains(codepoint)) core->type_len += 6 * 2 + 2;
        core->chat_input_string.insert(core->chat_input_string.begin() + core->type_pos, codepoint);
        ++core->type_pos;
        while(core->type_len >= core->width - 20) {
            if(standard_chars.contains(core->chat_input_string[core->type_start])) core->type_len -= 6 * 2 + 2;
            ++core->type_start;
        }
    }
}

Core::Core(GLFWwindow* window, netwk::TCP_client& connection_ref, std::array<double, 2> player_start_pos, std::string name, bool& game_running) : connection(connection_ref) {
    stbi_set_flip_vertically_on_load(1);
    load_textures_into_map();
    create_shaders();
    player = Player(this, player_start_pos, texture_map[2].id, name);
    version.generate_data();
    this->window = window;
    this->game_running = game_running;

    chunk_mesh_gen_thread = std::thread(
        [this]() {
            while(this->game_running) {
                if(chunk_mesh_gen_queue.size() != 0) {
                    uint key = chunk_mesh_gen_queue.front();
                    chunk_mesh_gen_queue.pop();
                    
                    loaded_chunks.at(key).generate_mesh();
                }
            }
        }
    );
}

// game loop

void Core::send_join_packet() {
    std::array<char, 64> player_name_array;
    memcpy(player_name_array.data(), player.name.str.data(), player.name.str.size() + 1);
    netwk::player_join_packet_toserv join_packet{player_name_array, {player.position, player.direction_facing}};
    connection.send(netwk::make_packet(0, (void*)&join_packet, sizeof(join_packet)));
}

void Core::send_positon_packet() {
    netwk::entity_movement_packet_toserv send_packet{player.position, player.direction_facing};
    connection.send(netwk::make_packet(2, (void*)&send_packet, sizeof(send_packet)));
}

void Core::game_math(uint delta) {
    glfwGetFramebufferSize(window, &width, &height);
    width = width + 1 * (width % 2 == 1);
    height = height + 1 * (height % 2 == 1);
    glViewport(0, 0, width, height);

    glfwPollEvents();

    process_key_inputs();

    if(send_chat_message) {
        process_input(chat_input_string, player, chat_list, connection);
        chat_input_string = "";
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

    camera_pos = glm::vec2(player.position[0], player.position[1] + 0.875);

    if(water_cycle_timer.increment_by(delta)) water_sprite_counter.increment();
}

void Core::render(GLuint v_array) {
    glm::vec3 camera_position(camera_pos, 0.0f);
    glm::vec3 look_at = camera_position + glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up(0.0f, -1.0f, 0.0f);
    glm::mat4 pv_matrix = glm::ortho(-float(width / 2) / scale, float(width / 2) / scale, float(height / 2) / scale, -float(height / 2) / scale); // perspective matrix
    pv_matrix *= glm::lookAt(camera_position, look_at, up); // view matrix


    int reference_y = int(player.position[1] / 16) * 16 - 32;

    std::array<uint, total_loaded_chunks> active_chunk_keys_copy = active_chunk_keys;
    for(int i = 0; i < total_loaded_chunks; ++i) {
        uint key = active_chunk_keys_copy[i];
        
        if(loaded_chunks.contains(key)) {
            Chunk_data& chunk = loaded_chunks[key];
            
            if(chunk.mesh_generated == 0) chunk_mesh_gen_queue.push(key);
            else if(chunk.mesh_generated == 2) {
                if(!chunk.vertices_loaded) chunk.load_vertices();
                chunk.render(shader_map[5], texture_map[0].id, pv_matrix);
            }
            
            /*double x_corner = (chunk.corner[0] - camera_pos[0]) * scale;
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
            }*/
        }
    }

    glBindVertexArray(v_array);

    player.render(reference_y, camera_pos, scale, shader_map[0], {width, height});

    for(auto& [key, entity] : entity_map) {
        entity.render(reference_y, camera_pos, scale, shader_map[0], {width, height});
    }
}

void Core::render_gui() {
    std::string hex_coords = to_hex_string(player.position[0]) + " " + to_hex_string(player.position[1]);
    std::string dec_coords = std::to_string(int(player.position[0])) + " " + std::to_string(int(player.position[1]));

    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(width / 2 - 8 - ((version.str.size() - 15) * 12 - 2)), float(height / 2 - 52)}, version.str, 2, 1000);
    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(width / 2 - 8 - ((hex_coords.size() - 4) * 12 - 2)), float(height / 2 - 78)}, hex_coords, 2, 500);
    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(width / 2 - 8 - (dec_coords.size() * 12 - 2)), float(height / 2 - 104)}, dec_coords, 2, 500);

    if(current_activity == CHAT) {
        render_text_type(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-width / 2 + 6), float(height / 2 - 26)}, chat_input_string.substr(type_start), 2, type_pos - type_start, width);
        
        glUseProgram(shader_map[4]);
        glUniform1f(0, 0.001f);
        glUniform2f(1, width, height);
        glUniform4f(3, -width / 2, height / 2 - 32, width, 32);
        glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    int text_scale = 0.0625 * (scale - 16);
    render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-player.name.generate_data() / 2 * text_scale), float(0.875 * scale)}, player.name.str, text_scale, 1000);

    int text_y_pos = height / 2 - 52;
    int chat_line_counter = 0;
    for(text_struct message : chat_list) {
        if(chat_line_counter >= chat_line) {
            int lines_displayed = chat_line_counter - chat_line;
            if(lines_displayed < 12) {
                int lines_passed = render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(0, (12 - lines_displayed < message.lines) ? message.first_chars[12 - lines_displayed] : message.str.length()), 2, 600);
                text_y_pos -= lines_passed * 22;
                chat_line_counter += lines_passed;
            }
        } else {
            chat_line_counter += message.lines;
            if(chat_line_counter > chat_line) {
                int num = message.lines - (chat_line_counter - chat_line);
                int lines_passed = render_text(shader_map[1], texture_map[1].id, texture_map[1].data, {width, height}, {float(-width / 2 + 10), float(text_y_pos)}, message.str.substr(message.first_chars[num]), 2, 600);
                text_y_pos -= lines_passed * 22;
            }
        }
    }

    if(chat_list.size() != 0) {
        glUseProgram(shader_map[4]);
        glUniform1f(0, 0.001f);
        glUniform2f(1, width, height);
        glUniform4f(3, -width / 2, text_y_pos + 22 - 6, 620, height / 2 - (text_y_pos + 22 - 6) - 26);
        glUniform4f(7, 0.0f, 0.0f, 0.0f, 0.125f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    bool dead_heart = false;
    for(int i = 0; i < 8; i++) {
        if(i >= player.health) dead_heart = true;
        draw_tile(shader_map[0], texture_map[5].id, {12.0f + i * 57 - width / 2, 12.0f - height / 2, 48.0f, 42.0f}, {16 * dead_heart, 1, 16, 14, 128, 128}, {width, height}, 0.0005f);
        //draw_tile(shader_map[0], texture_map[5].id, {12.0f + i * 33 - width / 2, 12.0f - height / 2, 27.0f, 24.0f}, {32 + 9 * dead_heart, 0, 9, 8, 128, 128}, {width, height}, 0.0005f);
    }
}