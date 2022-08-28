#define _USE_MATH_DEFINES
#include <vector>
#include <array>
#include <iostream>
#include <map>
#include <chrono>
#include <unordered_map>
#include <thread>

#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>
#include <glm-master\glm\glm.hpp>
#include <glm-master\glm\gtc\matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "PerlinNoise-master\PerlinNoise.hpp"

const char* v_src = R"""(
#version 460 core
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) uniform mat4 proj_view_mat;
layout(location = 1) uniform mat4 transform_mat;
layout(location = 2) uniform vec2 tex_offset;

out vec2 tex_coord;

void main() {
    gl_Position = proj_view_mat * (transform_mat * vec4(in_pos, 1.0));
    tex_coord = in_tex_coord + tex_offset;
}
)""";

const char* f_src = R"""(
#version 460 core

in vec2 tex_coord;

uniform sampler2D input_texture;

out vec4 FragColor;

void main() {
    vec4 color = texture(input_texture, tex_coord);
    if(color.w == 0.0) discard;
    FragColor = color;
}
)""";

const int chunk_size = 16;
const float player_height = 2;

//__attribute__((always_inline))
time_t get_time() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

GLuint create_shader_program(const char* vertex_shader, const char* fragment_shader) {
    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v_shader, 1, &vertex_shader, NULL);
    glShaderSource(f_shader, 1, &fragment_shader, NULL);
    glCompileShader(v_shader);
    glCompileShader(f_shader);

    GLint is_compiled = 0;
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &is_compiled);
    if(is_compiled == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(v_shader, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetShaderInfoLog(v_shader, max_length, &max_length, info_log.data());

        printf("Vertex shader failed to compile: \n%s", info_log.data());
    }

    is_compiled = 0;
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &is_compiled);
    if(is_compiled == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(f_shader, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetShaderInfoLog(f_shader, max_length, &max_length, info_log.data());

        printf("Fragment shader failed to compile: \n%s", info_log.data());
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);
    glLinkProgram(program);

    GLint is_linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &is_compiled);
    if(is_compiled == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetProgramInfoLog(program, max_length, &max_length, info_log.data());

        printf("Shader failed to link: \n%s", info_log.data());
    }

    int num_uniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uniforms);

    int max_char_len;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_char_len);
    if(num_uniforms > 0 && max_char_len > 0) {
        std::vector<char> char_buffer(max_char_len);
        for(int i = 0; i < num_uniforms; i++) {
            int length, size;
            GLenum data_type;
            glGetActiveUniform(program, i, max_char_len, &length, &size, &data_type, char_buffer.data());
            GLint var_location = glGetUniformLocation(program, char_buffer.data());
            printf("Uniform %s has location %i\n", char_buffer.data(), var_location);
        }
    }

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return program;
}

unsigned int load_texture(char address[], std::array<int, 3> &info) {
    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    unsigned char *data = stbi_load(address, &info[0], &info[1], &info[2], 0); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info[0], info[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return texture_id;
}

struct Vertex {
    glm::vec3 position;
    glm::vec2 color;
};

std::vector<Vertex> vertex_vector;
std::vector<uint32_t> index_vector;

std::vector<Vertex> vertex_vector1 = {
    {{0.0f, player_height / 2, player_height}, {0.0f, 0.125f}},
    {{0.0f, player_height / 2, 0.0f}, {0.0f, 0.0f}},
    {{0.0f, -player_height / 2, player_height}, {0.25f, 0.125f}},
    {{0.0f, -player_height / 2, player_height}, {0.25f, 0.125f}},
    {{0.0f, player_height / 2, 0.0f}, {0.0f, 0.0f}},
    {{0.0f, -player_height / 2, 0.0f}, {0.25f, 0.0f}}
};

std::array<Vertex, 24> cube_vertices = {
    Vertex{{0.0f, 1.0f, 1.0f}, {0.0f, 0.5f}},
    Vertex{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // -x
    Vertex{{0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
    Vertex{{0.0f, 0.0f, 0.0f}, {0.5f, 0.0f}},

    Vertex{{1.0f, 0.0f, 1.0f}, {0.0f, 0.5f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // +x
    Vertex{{1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
    Vertex{{1.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},

    Vertex{{0.0f, 0.0f, 1.0f}, {0.0f, 0.5f}},
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // -y
    Vertex{{1.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}},
    
    Vertex{{1.0f, 1.0f, 1.0f}, {0.0f, 0.5f}},
    Vertex{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // +y
    Vertex{{0.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
    Vertex{{0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},

    Vertex{{1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {0.0f, 0.5f}}, // -z
    Vertex{{0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}},
    Vertex{{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}},

    Vertex{{0.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
    Vertex{{0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}}, // +z
    Vertex{{1.0f, 1.0f, 1.0f}, {1.0f, 0.5f}},
    Vertex{{1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
};

std::array<uint32_t, 36> cube_indices = {
    0, 1, 2, 2, 1, 3,
    4, 5, 6, 6, 5, 7,
    8, 9, 10, 10, 9, 11,
    12, 13, 14, 14, 13, 15,
    16, 17, 18, 18, 17, 19,
    20, 21, 22, 22, 21, 23
};

struct Counter {
    unsigned int value;
    unsigned int limit;

    bool increment() {
        value++;
        if(value >= limit) {
            value = 0;
            return true;
        }
        return false;
    }

    bool increment_by(unsigned int change) {
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
};

glm::mat4 identity_mat4 = glm::identity<glm::mat4>();
glm::vec2 zero_vec2(0.0f, 0.0f);

struct Camera {
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 1.0f);

    float xdeg = 0.0f;
    float zdeg = 0.0f;
    glm::vec3 look_dir;
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec2 move_dir = glm::vec2(0.0f, -1.0f);

    float near;
    float far;
    float view_angle;

    Camera(glm::vec3 pos, glm::vec3 look_at, float near, float far, float view_angle) {
        glm::vec3 norm_look_at = glm::normalize(look_at);
        glm::vec2 look_at_xy = glm::vec2(norm_look_at);
        float length = glm::length(look_at_xy);
        
        this->pos = pos;
        this->look_dir = norm_look_at;

        // getting xdeg and zdeg
        this->xdeg = atan2(look_at_xy.y, look_at_xy.x);
        this->zdeg = atan2(norm_look_at.z, length);

        this->move_dir = glm::normalize(look_at_xy);

        this->near = near;
        this->far = far;
        this->view_angle = view_angle;
    }
    Camera() = default;

    void update_camera_dir() {
        float temp_x = cos(glm::radians(xdeg));
        float temp_y = sin(glm::radians(xdeg));
        float width_at_z = cos(glm::radians(zdeg));
        float z = sin(glm::radians(zdeg));

        look_dir = glm::vec3(temp_x * width_at_z, temp_y * width_at_z, z);
        move_dir = glm::vec2(temp_x, temp_y);
    }

    glm::mat4 get_pv_matrix(int window_width, int window_height) {
        glm::mat4 matrix = glm::perspective(view_angle, float(window_width) / window_height, near, far); // projection matrix
        matrix *= glm::lookAt(pos, pos + look_dir, up); // view matrix

        return matrix;
    }
};

struct Core {
    std::map<int, bool> user_input_array = {
        {GLFW_KEY_W, false},
        {GLFW_KEY_A, false},
        {GLFW_KEY_S, false},
        {GLFW_KEY_D, false},
        {GLFW_KEY_LEFT_CONTROL, false},
        {GLFW_KEY_LEFT_SHIFT, false},
        {GLFW_MOUSE_BUTTON_LEFT, false}
    };

    float speed = 10.0f;
    std::array<double, 2> mouse_pos;
    glm::ivec3 current_chunk_location = glm::ivec3(0x7fffffff, 0x7fffffff, 0x7fffffff);
    glm::ivec3 render_distance = glm::ivec3(4, 4, 3);

    Camera world_camera;
    Camera space_camera;

    Core(Camera world_camera) {
        this->world_camera = world_camera;
    }
};

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Core* core = (Core*)glfwGetWindowUserPointer(window);
    if(core->user_input_array[GLFW_MOUSE_BUTTON_LEFT]) {
        double mouse_pos_change_x = xpos - core->mouse_pos[0];
        double mouse_pos_change_y = ypos - core->mouse_pos[1];

        core->world_camera.xdeg -= mouse_pos_change_x / 2;
        if(core->world_camera.xdeg >= 360) core->world_camera.xdeg -= 360;
        else if(core->world_camera.xdeg < 0) core->world_camera.xdeg += 360;
        core->world_camera.zdeg = std::max(std::min(89.9, core->world_camera.zdeg - mouse_pos_change_y / 2), -89.9);
    }
    core->mouse_pos = {xpos, ypos};
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Core* core = (Core*)glfwGetWindowUserPointer(window);
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
        if(action == GLFW_PRESS) {
            core->user_input_array[GLFW_MOUSE_BUTTON_LEFT] = true;
        } else if(action == GLFW_RELEASE) {
            core->user_input_array[GLFW_MOUSE_BUTTON_LEFT] = false;
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Core* core = (Core*)glfwGetWindowUserPointer(window);
    if(action == GLFW_PRESS) {
        if(core->user_input_array.contains(key)) {
            core->user_input_array[key] = true;
        }
    } else if(action == GLFW_RELEASE) {
       if(core->user_input_array.find(key) != core->user_input_array.end()) core->user_input_array[key] = false;
    }
}

/*void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(yoffset > 0) {
        speed *= 1.5;
    } else if(yoffset < 0) {
        speed /= 1.5;
    }
}*/

uint64_t convert_to_chunk_key(glm::ivec3 location) {
    uint64_t key = 0;
    
    key += location.x + 0x800;
    key += (location.y + 0x800) * 0x1000;
    key += uint64_t(location.z + 0x800) * 0x1000000;

    return key;
}

typedef std::array<std::array<std::array<uint16_t, 16>, 16>, 16> block_id_array;

void load_chunk_vertices(block_id_array& chunk, std::vector<Vertex>& vertex_vector, std::vector<uint32_t>& index_vector, uint64_t key) {
    
}

struct Chunk_data;

std::vector<uint64_t> loaded_chunk_keys;
std::unordered_map<uint64_t, Chunk_data> chunk_map;

struct Chunk_data {
    GLuint vertex_array;
    GLuint array_buffer;
    GLuint element_buffer;
    glm::mat4 transform_mat = identity_mat4;
    int index_count;

    bool mesh_generated = false;
    bool vertices_loaded = false;

    std::vector<Vertex> vertex_vector;
    std::vector<uint32_t> index_vector;

    block_id_array blocks;
    glm::ivec3 location;
    uint64_t key;

    Chunk_data(glm::ivec3 location) {
        this->location = location;
        transform_mat = glm::translate(transform_mat, glm::vec3(location * 16));
    }

    void generate_blocks(siv::PerlinNoise& noise) {
        glm::vec3 corner = location * 16;
        for(float z = 0; z < 16; z++) {
            for(float y = 0; y < 16; y++) {
                for(float x = 0; x < 16; x++) {
                    float val = noise.normalizedOctave3D((corner.x + x) / 16, (corner.y + y) / 16, (corner.z + z) / 16, 4);
                    blocks[x][y][z] = floor(val);
                }
            }
        }

        key = convert_to_chunk_key(location);
    }

    void generate_mesh() {
        int count = 0;
        for(int z = 0; z < 16; z++) {
            for(int y = 0; y < 16; y++) {
                for(int x = 0; x < 16; x++) {
                    if(blocks[x][y][z] != 0) {
                        if((x == 0 && chunk_map.at(key - 1).blocks[15][y][z] == 0) || (x != 0 && blocks[x - 1][y][z] == 0)) {
                            for(int i = 0; i < 4; i++) {
                                Vertex v = cube_vertices[i];
                                v.position += glm::vec3(x, y, z);
                                vertex_vector.push_back(v);
                            }
                            for(int i = 0; i < 6; i++) {
                                index_vector.push_back(cube_indices[i] + count * 4);
                            }
                            count++;
                        }
                        if((x == 15 && chunk_map.at(key + 1).blocks[0][y][z] == 0) || (x != 15 && blocks[x + 1][y][z] == 0)) {
                            for(int i = 4; i < 8; i++) {
                                Vertex v = cube_vertices[i];
                                v.position += glm::vec3(x, y, z);
                                vertex_vector.push_back(v);
                            }
                            for(int i = 0; i < 6; i++) {
                                index_vector.push_back(cube_indices[i] + count * 4);
                            }
                            count++;
                        }
                        if((y == 0 && chunk_map.at(key - 0x1000).blocks[x][15][z] == 0) || (y != 0 && blocks[x][y - 1][z] == 0)) {
                            for(int i = 8; i < 12; i++) {
                                Vertex v = cube_vertices[i];
                                v.position += glm::vec3(x, y, z);
                                vertex_vector.push_back(v);
                            }
                            for(int i = 0; i < 6; i++) {
                                index_vector.push_back(cube_indices[i] + count * 4);
                            }
                            count++;
                        }
                        if((y == 15 && chunk_map.at(key + 0x1000).blocks[x][0][z] == 0) || (y != 15 && blocks[x][y + 1][z] == 0)) {
                            for(int i = 12; i < 16; i++) {
                                Vertex v = cube_vertices[i];
                                v.position += glm::vec3(x, y, z);
                                vertex_vector.push_back(v);
                            }
                            for(int i = 0; i < 6; i++) {
                                index_vector.push_back(cube_indices[i] + count * 4);
                            }
                            count++;
                        }
                        if((z == 0 && chunk_map.at(key - 0x1000000).blocks[x][y][15] == 0) || (z != 0 && blocks[x][y][z - 1] == 0)) {
                            for(int i = 16; i < 20; i++) {
                                Vertex v = cube_vertices[i];
                                v.position += glm::vec3(x, y, z);
                                vertex_vector.push_back(v);
                            }
                            for(int i = 0; i < 6; i++) {
                                index_vector.push_back(cube_indices[i] + count * 4);
                            }
                            count++;
                        }
                        if((z == 15 && chunk_map.at(key + 0x1000000).blocks[x][y][0] == 0) || (z != 15 && blocks[x][y][z + 1] == 0)) {
                            for(int i = 20; i < 24; i++) {
                                Vertex v = cube_vertices[i];
                                v.position += glm::vec3(x, y, z);
                                vertex_vector.push_back(v);
                            }
                            for(int i = 0; i < 6; i++) {
                                index_vector.push_back(cube_indices[i] + count * 4);
                            }
                            count++;
                        }
                    }
                }
            }
        }

        index_count = index_vector.size();

        mesh_generated = true;
    }

    void load_vertices() {
        // create and bind buffers
        glCreateVertexArrays(1, &vertex_array);
        glBindVertexArray(vertex_array);

        glCreateBuffers(1, &array_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, array_buffer);

        glCreateBuffers(1, &element_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);

        // load buffers with data
        glBufferData(GL_ARRAY_BUFFER, vertex_vector.size() * sizeof(Vertex), vertex_vector.data(), GL_STATIC_DRAW); // array buffer (vertices)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
        glEnableVertexAttribArray(1);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_vector.size() * sizeof(uint32_t), index_vector.data(), GL_STATIC_DRAW); // element buffer (indices)

        vertex_vector.clear();
        index_vector.clear();

        vertices_loaded = true;
    }

    void render(GLuint shader_program, GLuint texture, glm::mat4& pv_mat) {
        glBindVertexArray(vertex_array);

        glUseProgram(shader_program);
        glBindTexture(GL_TEXTURE_2D, texture);        
        glUniformMatrix4fv(0, 1, GL_FALSE, &pv_mat[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &transform_mat[0][0]);
        glUniform2fv(2, 1, &zero_vec2[0]);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }

    void delete_buffers() {
        glDeleteBuffers(1, &array_buffer);
        glDeleteBuffers(1, &element_buffer);
        glDeleteVertexArrays(1, &vertex_array);
    }

    static void generate_chunk(glm::ivec3 location, siv::PerlinNoise& noise) {
        time_t start_time = get_time();

        uint64_t key = convert_to_chunk_key(location);

        if(!chunk_map.contains(key)) {
            chunk_map.emplace(key, Chunk_data(location));
            chunk_map.at(key).generate_blocks(noise);
        }

        if(!chunk_map.contains(key - 1)) {
            chunk_map.emplace(key - 1, Chunk_data(location + glm::ivec3(-1, 0, 0)));
            chunk_map.at(key - 1).generate_blocks(noise);
        }
        if(!chunk_map.contains(key + 1)) {
            chunk_map.emplace(key + 1, Chunk_data(location + glm::ivec3(1, 0, 0)));
            chunk_map.at(key + 1).generate_blocks(noise);
        }
        if(!chunk_map.contains(key - 0x1000)) {
            chunk_map.emplace(key - 0x1000, Chunk_data(location + glm::ivec3(0, -1, 0)));
            chunk_map.at(key - 0x1000).generate_blocks(noise);
        }
        if(!chunk_map.contains(key + 0x1000)) {
            chunk_map.emplace(key + 0x1000, Chunk_data(location + glm::ivec3(0, 1, 0)));
            chunk_map.at(key + 0x1000).generate_blocks(noise);
        }
        if(!chunk_map.contains(key - 0x1000000)) {
            chunk_map.emplace(key - 0x1000000, Chunk_data(location + glm::ivec3(0, 0, -1)));
            chunk_map.at(key - 0x1000000).generate_blocks(noise);
        }
        if(!chunk_map.contains(key + 0x1000000)) {
            chunk_map.emplace(key + 0x1000000, Chunk_data(location + glm::ivec3(0, 0, 1)));
            chunk_map.at(key + 0x1000000).generate_blocks(noise);
        }

        chunk_map.at(key).generate_mesh();

        std::cout << double(get_time() - start_time) / 1000000000 << "\n";
    }
};

void load_chunk_loop(bool& game_running, Core& core, siv::PerlinNoise& noise, std::vector<uint64_t>& loaded_chunk_keys) {
    while(game_running) {
        glm::ivec3 camera_chunk_location = glm::floor(core.world_camera.pos / 16.0f);
        if(camera_chunk_location != core.current_chunk_location) {

            std::cout << std::dec << camera_chunk_location.x << " " << camera_chunk_location.y << " " << camera_chunk_location.z << "\n" << std::hex << convert_to_chunk_key(camera_chunk_location) << "\n";
            core.current_chunk_location = camera_chunk_location;
            std::vector<uint64_t> chunk_keys;
            for(int z = -core.render_distance.z; z <= core.render_distance.z; z++) {
                for(int y = -core.render_distance.y; y <= core.render_distance.y; y++) {
                    for(int x = -core.render_distance.x; x <= core.render_distance.x; x++) {
                        glm::ivec3 chunk_loc(x + core.current_chunk_location.x, y + core.current_chunk_location.y, z + core.current_chunk_location.z);
                        uint64_t key = convert_to_chunk_key(chunk_loc);

                        if(!chunk_map.contains(key) || !chunk_map.at(key).mesh_generated) {
                            Chunk_data::generate_chunk(chunk_loc, noise);
                        }

                        chunk_keys.push_back(key);
                    }
                }
            }
            loaded_chunk_keys = chunk_keys;
        }
    }
}

int main() {
    // load world vertices
    siv::PerlinNoise noise(5009);

    glfwInit();

    // set up window attributes and callbacks
    int width = 1000, height = 600;
    GLFWwindow* window = glfwCreateWindow(width, height, "test", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    Core core(Camera({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, 0.0625f, 1024.0f, 45.0f));
    glfwSetWindowUserPointer(window, (void*)&core);
    glfwSwapInterval(0);

    gladLoadGL();

    // create and bind buffers for character
    GLuint vertex_array1;
    glCreateVertexArrays(1, &vertex_array1);
    glBindVertexArray(vertex_array1);
    GLuint array_buffer1;
    glCreateBuffers(1, &array_buffer1);
    glBindBuffer(GL_ARRAY_BUFFER, array_buffer1);
    // load vertices
    glBufferData(GL_ARRAY_BUFFER, vertex_vector1.size() * sizeof(Vertex), vertex_vector1.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    // settings (depth testing, see triangles)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    //glPolygonMode(GL_BACK, GL_LINE);

    // shaders
    GLuint program = create_shader_program(v_src, f_src);

    // load textures
    stbi_set_flip_vertically_on_load(1);
    std::array<int, 3> info;
    GLuint texture = load_texture("res\\tiles\\3d_tileset_16.png", info);
    GLuint player_texture = load_texture("res\\entities\\Player\\avergreen_spritesheet.png", info);

    // counters for walk cycle
    Counter frame_switch = {0, 150000000};
    Counter walk_cycle = {3, 4};

    bool game_running = true;

    std::thread chunk_load_thread(
        [&game_running, &core, &noise]() {
            load_chunk_loop(game_running, core, noise, loaded_chunk_keys);
        }
    );

    //chunk_load_thread.join();

    time_t start_time = get_time();
    while(game_running) {
        // time calculations (get delta_time)
        time_t current_time = get_time();
        uint32_t delta_time = current_time - start_time;
        start_time = current_time;

        // update window size
        glfwGetFramebufferSize(window, &width, &height);
        if(width == 0 && height == 0) {
            width = 16;
            height = 16;
        }
        glViewport(0, 0, width, height);

        // get key inputs
        glfwPollEvents();
        core.world_camera.update_camera_dir();

        // move the camera
        glm::vec3 move_vector(0.0f, 0.0f, 0.0f);
        if(core.user_input_array[GLFW_KEY_W]) {
            move_vector.x += core.world_camera.move_dir.x;
            move_vector.y += core.world_camera.move_dir.y;
        } 
        if(core.user_input_array[GLFW_KEY_A]) {
            move_vector.x -= core.world_camera.move_dir.y;
            move_vector.y += core.world_camera.move_dir.x;
        } 
        if(core.user_input_array[GLFW_KEY_S]) {
            move_vector.x -= core.world_camera.move_dir.x;
            move_vector.y -= core.world_camera.move_dir.y;
        }
        if(core.user_input_array[GLFW_KEY_D]) {
            move_vector.x += core.world_camera.move_dir.y;
            move_vector.y -= core.world_camera.move_dir.x;
        }
        // normalize x and y and scale move vector based on delta_time and key inputs
        if(!(move_vector.x == 0.0f && move_vector.y == 0.0f)) {
            move_vector = glm::normalize(move_vector);
        } 
        if(core.user_input_array[GLFW_KEY_SPACE]) {
            move_vector.z += 1.0f;
        }
        if(core.user_input_array[GLFW_KEY_LEFT_SHIFT]) {
            move_vector.z -= 1.0f;
        }
        float move_factor = core.speed * (double(delta_time) / 1000000000) * ((core.user_input_array[GLFW_KEY_LEFT_CONTROL]) ? 2.0f : 1.0f);
        core.world_camera.pos += move_vector * move_factor;

        // character walk cycle
        if(frame_switch.increment_by(delta_time)) {
            walk_cycle.increment();
        }
        
        // clear window and reset screen buffers
        glClearColor(0.4, 0.6, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get projection and view matrix
        glm::mat4 pv_mat = core.world_camera.get_pv_matrix(width, height);

        // bind world vertices and draw world
        std::vector<uint64_t> temp_keys = loaded_chunk_keys;
        for(uint64_t key : temp_keys) {
            Chunk_data& c = chunk_map.at(key);
            if(!c.vertices_loaded) {
                c.load_vertices();
            }
            c.render(program, texture, pv_mat);
        }

        // translate character for billboard effect
        glm::vec2 diff = glm::vec2(core.world_camera.pos) - glm::vec2(7.0f, 3.0f); 
        glm::vec3 move(7.0f, 3.0f, 3.0f);
        float angle = atan2(-diff.y, -diff.x);
        glm::mat4 transform_mat_billboard = identity_mat4;
        transform_mat_billboard = glm::translate(transform_mat_billboard, move);
        transform_mat_billboard = glm::rotate(transform_mat_billboard, angle, core.world_camera.up);
        // change sprite shown based on rotation angle
        glm::vec2 direction_offset_vector(0.25f * walk_cycle.value, 0.25f);
        float look_angle = fmod(angle - M_PI / 2 + M_PI * 2, M_PI * 2);
        if(!(look_angle < M_PI * 0.25 || look_angle > M_PI * 1.75)) {
            if(look_angle < M_PI * 0.75) {
                direction_offset_vector.y = 0.375f;
            } else if(look_angle < M_PI * 1.25) {
                direction_offset_vector.y = 0.0f;
            } else {
                direction_offset_vector.y = 0.125f;
            }
        }
        
        // bind vertices and draw character
        glBindVertexArray(vertex_array1);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, player_texture);
        glUniformMatrix4fv(0, 1, GL_FALSE, &pv_mat[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &transform_mat_billboard[0][0]);
        glUniform2fv(2, 1, &direction_offset_vector[0]);
        glDrawArrays(GL_TRIANGLES, 0, vertex_vector1.size());

        glfwSwapBuffers(window);

        game_running = !glfwWindowShouldClose(window);
    }

    // delete buffers
    for(auto& [key, c] : chunk_map) {
        c.delete_buffers();
    }
    glDeleteBuffers(1, &array_buffer1);
    glDeleteVertexArrays(1, &vertex_array1);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}