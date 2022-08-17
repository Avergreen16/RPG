#define _USE_MATH_DEFINES
#include <vector>
#include <array>
#include <iostream>
#include <map>
#include <chrono>

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
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;

layout(location = 0) uniform mat4 matrix;

out vec2 f_texcoord;
out vec3 f_normal;

void main() {
    gl_Position = matrix * vec4(in_pos, 1.0);
    f_texcoord = in_texcoord;
    f_normal = in_normal;
}
)""";

const char* f_src = R"""(
#version 460 core
layout(location = 1) uniform vec3 light_source_rel_pos;

in vec2 f_texcoord;
in vec3 f_normal;

uniform sampler2D input_texture;

out vec4 FragColor;

void main() {
    float dotproduct = dot(f_normal, normalize(light_source_rel_pos));
    FragColor = vec4(texture(input_texture, f_texcoord).xyz * min(max((dotproduct * 12.0), 0.125), 1.0), 1.0);
}
)""";

const char* f_src2 = R"""(
#version 460 core

in vec2 f_texcoord;

uniform sampler2D input_texture;

out vec4 FragColor;

void main() {
    FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
)""";

time_t __attribute__((always_inline)) get_time() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

GLuint create_shader(const char* vertex_shader, const char* fragment_shader) {
    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v_shader, 1, &vertex_shader, NULL);
    glShaderSource(f_shader, 1, &fragment_shader, NULL);
    glCompileShader(v_shader);
    glCompileShader(f_shader);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, v_shader);
    glAttachShader(shader, f_shader);
    glLinkProgram(shader);

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return shader;
}

glm::vec3 up(0.0f, 0.0f, 1.0f);
glm::mat4 identity = glm::identity<glm::mat4>();

float xdeg = 0.0f;
float zdeg = 0.0f;
glm::vec3 camera_dir(1.0f, 0.0f, 0.0f);
glm::vec3 camera_pos(40000.0f, 0.0f, 300.0f);
glm::vec2 move_dir(0.0f, -1.0f);
glm::vec3 up_dir = glm::normalize(camera_pos);

std::map<int, bool> user_input_array = {
    {GLFW_KEY_W, false},
    {GLFW_KEY_A, false},
    {GLFW_KEY_S, false},
    {GLFW_KEY_D, false},
    {GLFW_KEY_LEFT_CONTROL, false},
    {GLFW_KEY_LEFT_SHIFT, false},
    {GLFW_MOUSE_BUTTON_LEFT, false}
};

void update_camera_dir() {
    float temp_x = cos(glm::radians(xdeg));
    float temp_y = sin(glm::radians(xdeg));
    float width_at_z = cos(glm::radians(zdeg));
    float z = sin(glm::radians(zdeg));
    camera_dir = glm::vec3(temp_x * width_at_z, temp_y * width_at_z, z);

    move_dir = glm::vec2(temp_x, temp_y);
}

std::array<double, 2> mouse_pos;
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if(user_input_array[GLFW_MOUSE_BUTTON_LEFT]) {
        double mouse_pos_change_x = xpos - mouse_pos[0];
        double mouse_pos_change_y = ypos - mouse_pos[1];

        xdeg -= mouse_pos_change_x / 2;
        if(xdeg >= 360) xdeg -= 360;
        else if(xdeg < 0) xdeg += 360;
        zdeg = std::max(std::min(89.9, zdeg - mouse_pos_change_y / 2), -89.9);

        update_camera_dir();
        std::cout << move_dir[0] << " " << move_dir[1] << "\n";
    }
    mouse_pos = {xpos, ypos};
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
        if(action == GLFW_PRESS) {
            user_input_array[GLFW_MOUSE_BUTTON_LEFT] = true;
        } else if(action == GLFW_RELEASE) {
            user_input_array[GLFW_MOUSE_BUTTON_LEFT] = false;
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        if(user_input_array.contains(key)) {
            user_input_array[key] = true;
        }
    } else if(action == GLFW_RELEASE) {
       if(user_input_array.find(key) != user_input_array.end()) user_input_array[key] = false;
    }
}

float speed = 48.0f;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(yoffset > 0) {
        speed *= 1.5;
    } else if(yoffset < 0) {
        speed /= 1.5;
    }
}

struct Vertex {
    glm::vec3 position;
    glm::vec2 color;
    glm::vec3 normal;
};

std::vector<Vertex> vertex_vector;
std::vector<uint32_t> index_vector;

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

void generate_sphere(std::vector<Vertex>& vertex_vector, std::vector<uint32_t>& index_vector, float radius, const int div, siv::PerlinNoise& noise) {
    int vert_per_face = (div + 1) * (div + 1);

    int correction_vertices = 0;
    for(float y = 0; y < div + 1; y++) {
        for(float x = 0; x < div + 1; x++) {
            glm::vec3 position(-1.0f + x / div * 2.0f, -1.0f + y / div * 2.0f, 0.0f);
            position.z += 1.0f - std::max(abs(x - div / 2), abs(y - div / 2)) / div * 2;
            position = glm::normalize(position);

            //position *= 1 + noise.normalizedOctave3D(position[0] * 1.1 + 0.798, position[1] * 1.1 - 0.332, position[2] * 1.1, 7, 0.6);
            
            vertex_vector.push_back({position * radius, {fmod(atan2f(position.y, position.x) / (2 * M_PI) + 1.0, 1.0), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}, position});
        }
    }
    for(int i = div / 2 + 1; i < div + 1; i++) {
        vertex_vector.push_back({vertex_vector[i + (div / 2) * (div + 1)].position, {1.0f, vertex_vector[i + (div / 2) * (div + 1)].color.y}, vertex_vector[i + (div / 2) * (div + 1)].normal});
        correction_vertices++;
    }
    for(float i = 0; i < 8; i++) {
        if(i != 0) {
            vertex_vector.push_back({vertex_vector[(div / 2) + (div / 2) * (div + 1)].position, {i / 8, 1.0f}, vertex_vector[(div / 2) + (div / 2) * (div + 1)].normal});
            correction_vertices++;
        }
    }
    std::cout << vertex_vector[(div / 2) + (div / 2) * (div + 1)].color.x << "\n";
    for(int y = 0; y < div; y++) {
        for(int x = 0; x < div; x++) {
            uint32_t current_index = x + y * (div + 1);
            if((y == div / 2 || y == div / 2 - 1) && (x == div / 2 || x == div / 2 - 1)) {
                if(y == div / 2) {
                    if(x == div / 2) {
                        index_vector.push_back(current_index);
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(current_index + div + 2);
                        index_vector.push_back(vert_per_face + correction_vertices - 7);
                        index_vector.push_back(current_index + div + 2);
                        index_vector.push_back(current_index + div + 1);
                    } else {
                        index_vector.push_back(current_index);
                        index_vector.push_back(vert_per_face + correction_vertices - 5);
                        index_vector.push_back(current_index + div + 1);
                        index_vector.push_back(vert_per_face + correction_vertices - 6);
                        index_vector.push_back(current_index + div + 2);
                        index_vector.push_back(current_index + div + 1);
                    }
                } else {
                    if(x == div / 2) {
                        index_vector.push_back(current_index);
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(vert_per_face + correction_vertices - 2);
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(vert_per_face);
                        index_vector.push_back(vert_per_face + correction_vertices - 1);
                    } else {
                        index_vector.push_back(current_index);
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(vert_per_face + correction_vertices - 3);
                        index_vector.push_back(current_index);
                        index_vector.push_back(vert_per_face + correction_vertices - 4);
                        index_vector.push_back(current_index + div + 1);
                    }
                }
            } else if(y == (div / 2) - 1 && x > div / 2) {
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(vert_per_face + (x - div / 2 - 1));
                index_vector.push_back(current_index + 1);
                index_vector.push_back(vert_per_face + (x - div / 2));
                index_vector.push_back(vert_per_face + (x - div / 2 - 1));
            } else if(x == y) {
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index + div + 2);
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + div + 2);
                index_vector.push_back(current_index + div + 1);
            } else {
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index + div + 1);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index + div + 2);
                index_vector.push_back(current_index + div + 1);
            }
        }
    }

    for(float y = 0; y < div + 1; y++) {
        for(float x = 0; x < div + 1; x++) {
            glm::vec3 position(-1.0f + x / div * 2.0f, -1.0f + y / div * 2.0f, 0.0f);
            position.z -= 1.0f - std::max(abs(x - div / 2), abs(y - div / 2)) / div * 2;
            position = glm::normalize(position);

            //position *= 1 + noise.normalizedOctave3D(position[0] * 1.1 + 0.798, position[1] * 1.1 - 0.332, position[2] * 1.1, 7, 0.6);

            vertex_vector.push_back({position * radius, {fmod(atan2f(position.y, position.x) / (2 * M_PI) + 1.0, 1.0), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}, position});
        }
    }
    int correction_vertices_2 = 0;
    for(int i = div / 2 + 1; i < div + 1; i++) {
        vertex_vector.push_back({vertex_vector[vert_per_face + correction_vertices + i + (div / 2) * (div + 1)].position, {1.0f, vertex_vector[vert_per_face + correction_vertices + i + (div / 2) * (div + 1)].color.y}, vertex_vector[vert_per_face + correction_vertices + i + (div / 2) * (div + 1)].normal});
        correction_vertices_2++;
    }
    for(float i = 0; i < 8; i++) {
        if(i != 0) {
            vertex_vector.push_back({vertex_vector[vert_per_face + correction_vertices + (div / 2) + (div / 2) * (div + 1)].position, {i / 8, 0.0f}, vertex_vector[vert_per_face + correction_vertices + (div / 2) + (div / 2) * (div + 1)].normal});
            correction_vertices_2++;
        }
    }
    for(int y = 0; y < div; y++) {
        for(int x = 0; x < div; x++) {
            uint32_t current_index = vert_per_face + correction_vertices + x + y * (div + 1);
            if((y == div / 2 || y == div / 2 - 1) && (x == div / 2 || x == div / 2 - 1)) {
                if(y == div / 2) {
                    if(x == div / 2) {
                        index_vector.push_back(current_index);
                        index_vector.push_back(current_index + div + 2);
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 7);
                        index_vector.push_back(current_index + div + 1);
                        index_vector.push_back(current_index + div + 2);
                    } else {
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 5);
                        index_vector.push_back(current_index);
                        index_vector.push_back(current_index + div + 1);
                        index_vector.push_back(current_index + div + 2);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 6);
                        index_vector.push_back(current_index + div + 1);
                    }
                } else {
                    if(x == div / 2) {
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(current_index);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 2);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices);
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 1);
                    } else {
                        index_vector.push_back(current_index + 1);
                        index_vector.push_back(current_index);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 3);
                        index_vector.push_back(vert_per_face * 2 + correction_vertices + correction_vertices_2 - 4);
                        index_vector.push_back(current_index);
                        index_vector.push_back(current_index + div + 1);
                    }
                }
            } else if(y == (div / 2) - 1 && x > div / 2) {
                index_vector.push_back(current_index);
                index_vector.push_back(vert_per_face * 2 + correction_vertices + (x - div / 2 - 1));
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(vert_per_face * 2 + correction_vertices + (x - div / 2 - 1));
                index_vector.push_back(vert_per_face * 2 + correction_vertices + (x - div / 2));
            } else if(x == y) {
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + div + 2);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + div + 1);
                index_vector.push_back(current_index + div + 2);
            } else {
                index_vector.push_back(current_index);
                index_vector.push_back(current_index + div + 1);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index + 1);
                index_vector.push_back(current_index + div + 1);
                index_vector.push_back(current_index + div + 2);
            }
        }
    }
}

struct Sphere {
    glm::vec3 pos;
    float radius;
    GLuint surface_texture;
    glm::vec3 light_source;
    
    Sphere(glm::vec3 pos, float radius, GLuint surface_texture, glm::vec3 light_source) {
        this->pos = pos;
        this->radius = radius;
        this->surface_texture = surface_texture;
        this->light_source = light_source;
    }

    void render(glm::mat4 pers_view_mat, GLuint program) {
        glm::mat4 trans_mat = identity;
        trans_mat = glm::translate(trans_mat, pos);

        glm::vec3 scale(radius, radius, radius);
        trans_mat = glm::scale(trans_mat, scale);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, surface_texture);
        
        glm::mat4 render_matrix = pers_view_mat * trans_mat;
        glm::vec3 rel_light_source = (light_source - pos);
        glUniformMatrix4fv(0, 1, GL_FALSE, &render_matrix[0][0]);
        glUniform3fv(1, 1, &rel_light_source[0]);

        glDrawElements(GL_TRIANGLES, index_vector.size(), GL_UNSIGNED_INT, 0);
    }
};

int main() {
    int width = 1000, height = 600;
    glfwInit();

    siv::PerlinNoise noise(4097);
    generate_sphere(vertex_vector, index_vector, 1.0f, 32, noise);

    GLFWwindow* window = glfwCreateWindow(width, height, "test", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL();

    glfwSwapInterval(0);
    
    GLuint vao_id;
    glCreateVertexArrays(1, &vao_id);
    glBindVertexArray(vao_id);

    GLuint array_buffer_id;
    glCreateBuffers(1, &array_buffer_id);
    glBindBuffer(GL_ARRAY_BUFFER, array_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, vertex_vector.size() * sizeof(Vertex), vertex_vector.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 5));
    glEnableVertexAttribArray(2);

    GLuint ebo_id;
    glCreateBuffers(1, &ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_vector.size() * sizeof(uint32_t), index_vector.data(), GL_STATIC_DRAW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &v_src, 0);
    glCompileShader(v_shader);

    GLint is_compiled = 0;
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &is_compiled);
    if(is_compiled == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(v_shader, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetShaderInfoLog(v_shader, max_length, &max_length, info_log.data());

        printf("Vertex shader failed to compile: \n%s", info_log.data());
    }

    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &f_src, 0);
    glCompileShader(f_shader);

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

    GLuint outline_shader = create_shader(v_src, f_src2);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    stbi_set_flip_vertically_on_load(1);

    std::array<int, 3> data_array;
    GLuint terra_texture = load_texture("res\\planets\\terra.png", data_array);
    GLuint gas_giant0_texture = load_texture("res\\planets\\gasgiant0.png", data_array);
    GLuint sun_texture = load_texture("res\\planets\\sun.png", data_array);
    GLuint shafur_texture = load_texture("res\\planets\\shafur.png", data_array);
    GLuint gas_giant1_texture = load_texture("res\\planets\\gasgiant1.png", data_array);

    glPolygonMode(GL_BACK, GL_LINE);

    Sphere sun({0.0f, 0.0f, 0.0f}, 1400.0f, sun_texture, {20400.0f, 400.0f, 195.0f});
    Sphere gas_giant({40000.0f, 200.0f, 200.0f}, 120.0f, gas_giant0_texture, sun.pos);
    Sphere terra({40400.0f, 400.0f, 195.0f}, 12.0f, terra_texture, sun.pos);
    Sphere ice_giant({-1000.0f, 3000.0f, 100.0f}, 150.0f, gas_giant1_texture, sun.pos);
    Sphere shafur({40600.0f, -300.0f, 205.0f}, 10.0f, shafur_texture, sun.pos);

    time_t start_time = get_time();

    bool should_close = false;
    while(!should_close) {
        time_t current_time = get_time();
        uint32_t delta_time = current_time - start_time;
        start_time = current_time;

        glfwGetFramebufferSize(window, &width, &height);
        if(width == 0 || height == 0) {
            width = 16;
            height = 16;
        }
        glViewport(0, 0, width, height);

        glfwPollEvents();

        glm::vec2 move_vector(0.0f, 0.0f);
    
        if(user_input_array[GLFW_KEY_W]) {
            move_vector.x += move_dir.x;
            move_vector.y += move_dir.y;
        } 
        if(user_input_array[GLFW_KEY_A]) {
            move_vector.x -= move_dir.y;
            move_vector.y += move_dir.x;
        } 
        if(user_input_array[GLFW_KEY_S]) {
            move_vector.x -= move_dir.x;
            move_vector.y -= move_dir.y;
        }
        if(user_input_array[GLFW_KEY_D]) {
            move_vector.x += move_dir.y;
            move_vector.y -= move_dir.x;
        }

        if(!(move_vector.x == 0.0f && move_vector.y == 0.0f)) move_vector = glm::normalize(move_vector);
        float movement_factor = speed * (double(delta_time) / 1000000000);
        if(user_input_array[GLFW_KEY_LEFT_CONTROL]) {
            move_vector *= movement_factor;
        } else {
            move_vector *= movement_factor;
        }

        camera_pos += glm::vec3(move_vector, 0.0f);

        if(user_input_array[GLFW_KEY_SPACE]) {
            camera_pos.z += movement_factor;
        }
        if(user_input_array[GLFW_KEY_LEFT_SHIFT]) {
            camera_pos.z -= movement_factor;
        }
        
        //glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(vao_id);

        glm::vec3 center = camera_pos + camera_dir;
        glm::mat4 view_matrix = glm::lookAt(camera_pos, center, up);

        /*float projection_width = width;
        float projection_height = height;
        float aspect_ratio = projection_height / projection_width;
        float w = 100;
        float h = w * aspect_ratio;

        float left = -w / 2.0f;
        float right = w / 2.0f;
        float bottom = -h / 2.0f;
        float top = h / 2.0f;*/
        float near = 0.5f;
        float far = 100000.0f;

        glm::mat4 projection_matrix = glm::perspective(45.0f, float(width) / height, near, far);//glm::ortho(left, right, bottom, top, near, far);

        glm::mat4 proj_view_mat = projection_matrix * view_matrix;

        sun.light_source = camera_pos;

        sun.render(proj_view_mat, program);
        gas_giant.render(proj_view_mat, program);
        ice_giant.render(proj_view_mat, program);
        shafur.render(proj_view_mat, program);
        terra.render(proj_view_mat, program);

        glfwSwapBuffers(window);

        should_close = glfwWindowShouldClose(window);
    }

    glDeleteBuffers(1, &array_buffer_id);
    glDeleteBuffers(1, &ebo_id);
    glDeleteVertexArrays(1, &vao_id);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}