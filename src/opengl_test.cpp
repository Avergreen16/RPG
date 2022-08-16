#define _USE_MATH_DEFINES
#include <vector>
#include <array>
#include <iostream>
#include <map>

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

layout(location = 0) uniform mat4 trans_mat;
layout(location = 1) uniform mat4 view_mat;
layout(location = 2) uniform mat4 proj_mat;

out vec2 f_texcoord;
out vec3 f_normal;

void main() {
    gl_Position = proj_mat * view_mat * (trans_mat * vec4(in_pos, 1.0));
    f_texcoord = in_texcoord;
    f_normal = in_normal;
}
)""";

const char* f_src = R"""(
#version 460 core
layout(location = 3) uniform vec3 light_source_rel_pos;

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

float dist = 200.0f;
float xdeg = 0.0f;
float zdeg = 0.0f;
glm::vec3 camera_dir(1.0f, 0.0f, 0.0f);
glm::vec3 camera_pos(-20.0f, -20.0f, 0.0f);
glm::vec2 move_dir(0.0f, -1.0f);
glm::vec3 up_dir = glm::normalize(camera_pos);

glm::mat4 player_matrix = glm::rotate(identity, glm::radians(glm::acos(glm::dot(up_dir, up))), glm::cross(up_dir, up));

std::map<int, bool> user_input_array = {
    {GLFW_KEY_W, false},
    {GLFW_KEY_A, false},
    {GLFW_KEY_S, false},
    {GLFW_KEY_D, false},
    {GLFW_KEY_LEFT_CONTROL, false},
    {GLFW_KEY_LEFT_SHIFT, false},
    {GLFW_MOUSE_BUTTON_LEFT, false}
};

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(yoffset > 0) dist /= 1.2f;
    else dist *= 1.2f;
}

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

int main() {
    int width = 1000, height = 600;
    glfwInit();

    siv::PerlinNoise noise(4097);
    generate_sphere(vertex_vector, index_vector, 1.0f, 80, noise);

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

    glm::vec3 scale(16.0f, 16.0f, 16.0f);
    float rotation = 0.0f;
    glm::vec3 position(0.0f, 0.0f, 0.0f);
    glm::vec3 rot_axis(0.0f, 0.0f, 1.0f);

    glm::vec3 scale_2(190.0f, 190.0f, 190.0f);
    float rotation_2 = 180.0f;
    glm::vec3 position_2(800.0f, 200.0f, 20.0f);

    glm::vec3 scale_3(1400.0f, 1400.0f, 1400.0f);
    float rotation_3 = 0.0f;
    glm::vec3 position_3(30000.0f, 25000.0f, -600.0f);

    stbi_set_flip_vertically_on_load(1);

    std::array<int, 3> data_array;
    GLuint planet_texture = load_texture("res\\planets\\terra.png", data_array);
    GLuint gas_giant_texture = load_texture("res\\planets\\gasgiant.png", data_array);
    GLuint sun_texture = load_texture("res\\planets\\sun.png", data_array);

    glPolygonMode(GL_BACK, GL_LINE);

    bool should_close = false;
    while(!should_close) {
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
        if(user_input_array[GLFW_KEY_LEFT_CONTROL]) {
            move_vector *= 0.25;
        } else {
            move_vector *= 0.1f;
        }

        camera_pos += glm::vec3(glm::vec4(move_vector, 0.0f, 0.0f) * player_matrix);

        if(user_input_array[GLFW_KEY_SPACE]) {
            camera_pos += up_dir * 0.1f;
        }
        if(user_input_array[GLFW_KEY_LEFT_SHIFT]) {
            camera_pos -= up_dir * 0.1f;
        }

        up_dir = glm::normalize(camera_pos);

        player_matrix = glm::rotate(identity, glm::radians(glm::acos(glm::dot(up_dir, up))), glm::cross(up_dir, up));
        
        //glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(vao_id);

        glm::mat4 transform_matrix = glm::identity<glm::mat4>();
        transform_matrix = glm::translate(transform_matrix, position);
        transform_matrix = glm::rotate(transform_matrix, glm::radians(rotation), rot_axis);
        transform_matrix = glm::scale(transform_matrix, scale);

        glm::mat4 transform_matrix_2 = glm::identity<glm::mat4>();
        transform_matrix_2 = glm::translate(transform_matrix_2, position_2);
        transform_matrix_2 = glm::rotate(transform_matrix_2, glm::radians(rotation_2), rot_axis);
        transform_matrix_2 = glm::scale(transform_matrix_2, scale_2);

        glm::mat4 transform_matrix_3 = glm::identity<glm::mat4>();
        transform_matrix_3 = glm::translate(transform_matrix_3, position_3);
        transform_matrix_3 = glm::rotate(transform_matrix_3, glm::radians(rotation_3), rot_axis);
        transform_matrix_3 = glm::scale(transform_matrix_3, scale_3);

        glm::vec3 center = camera_pos + glm::vec3(glm::vec4(camera_dir, 1.0f) * player_matrix);
        glm::mat4 view_matrix = glm::lookAt(camera_pos, center, up_dir);

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

        glUseProgram(program);

        glBindTexture(GL_TEXTURE_2D, planet_texture);

        glUniformMatrix4fv(0, 1, GL_FALSE, &transform_matrix[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_FALSE, &projection_matrix[0][0]);
        glUniform3fv(3, 1, &position_3[0]);

        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glDrawElements(GL_TRIANGLES, index_vector.size(), GL_UNSIGNED_INT, 0);

        /*glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glUseProgram(outline_shader);

        glUniformMatrix4fv(2, 1, GL_FALSE, &transform_matrix[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(0, 1, GL_FALSE, &projection_matrix[0][0]);*/
        glm::mat4 rot = glm::rotate(identity, glm::radians(-rotation_2), rot_axis);
        glm::vec3 gas_giant_light_source = glm::vec4(position_3, 1.0) * glm::inverse(transform_matrix_2);
        glm::vec3 sun_light_source = -position_3;

        glBindTexture(GL_TEXTURE_2D, gas_giant_texture);

        glUniformMatrix4fv(0, 1, GL_FALSE, &transform_matrix_2[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_FALSE, &projection_matrix[0][0]);
        glUniform3fv(3, 1, &gas_giant_light_source[0]);
        
        glDrawElements(GL_TRIANGLES, index_vector.size(), GL_UNSIGNED_INT, 0);


        glBindTexture(GL_TEXTURE_2D, sun_texture);

        glUniformMatrix4fv(0, 1, GL_FALSE, &transform_matrix_3[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_FALSE, &projection_matrix[0][0]);
        glUniform3fv(3, 1, &sun_light_source[0]);
        
        glDrawElements(GL_TRIANGLES, index_vector.size(), GL_UNSIGNED_INT, 0);

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