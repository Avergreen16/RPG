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
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) uniform mat4 proj_view_mat;
layout(location = 1) uniform mat4 trans_mat;

out vec2 tex_coord;

void main() {
    gl_Position = proj_view_mat * (trans_mat * vec4(in_pos, 1.0));
    tex_coord = in_tex_coord;
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

time_t __attribute__((always_inline)) get_time() {
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

glm::vec3 up(0.0f, 0.0f, 1.0f);
glm::mat4 identity = glm::identity<glm::mat4>();

float xdeg = 0.0f;
float zdeg = 0.0f;
glm::vec3 camera_dir(0.0f, 0.0f, -1.0f);
glm::vec3 camera_pos(0.0f, 0.0f, 1.0f);
glm::vec2 move_dir(0.0f, -1.0f);

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

float speed = 5.0f;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    /*if(yoffset > 0) {
        speed *= 1.5;
    } else if(yoffset < 0) {
        speed /= 1.5;
    }*/
}

struct Vertex {
    glm::vec3 position;
    glm::vec2 color;
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

void load_vertices() {
    vertex_vector = std::vector<Vertex>{
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}}, // 0, 0, -1
    {{0.0f, 1.0f, 0.0f}, {0.0f, 0.5f}}, {{1.0f, 1.0f, 0.0f}, {0.5f, 0.5f}},

    {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 1.0f, 0.0f}, {0.5f, 0.0f}}, // 0, 1, -1
    {{0.0f, 2.0f, 0.0f}, {0.0f, 0.5f}}, {{1.0f, 2.0f, 0.0f}, {0.5f, 0.5f}},
    
    {{0.0f, 2.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 2.0f, 0.0f}, {0.5f, 0.0f}}, // 0, 2, -1
    {{0.0f, 3.0f, 0.0f}, {0.0f, 0.5f}}, {{1.0f, 3.0f, 0.0f}, {0.5f, 0.5f}}, 

    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, {{2.0f, 1.0f, 0.0f}, {0.5f, 0.0f}}, // 1, 1, -1
    {{1.0f, 2.0f, 0.0f}, {0.0f, 0.5f}}, {{2.0f, 2.0f, 0.0f}, {0.5f, 0.5f}}, 

    {{1.0f, 2.0f, 0.0f}, {0.0f, 0.0f}}, {{2.0f, 2.0f, 0.0f}, {0.5f, 0.0f}}, // 1, 2, -1
    {{1.0f, 3.0f, 0.0f}, {0.0f, 0.5f}}, {{2.0f, 3.0f, 0.0f}, {0.5f, 0.5f}}, 

    {{2.0f, 2.0f, 0.0f}, {0.0f, 0.0f}}, {{3.0f, 2.0f, 0.0f}, {0.5f, 0.0f}}, // 2, 2, -1
    {{2.0f, 3.0f, 0.0f}, {0.0f, 0.5f}}, {{3.0f, 3.0f, 0.0f}, {0.5f, 0.5f}}, 

    {{1.0f, 0.0f, 1.0f}, {1.0f, 0.5f}}, {{1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}}, // 1, 0, 0 -x
    {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, {{1.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},

    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.5f}}, {{2.0f, 1.0f, 1.0f}, {0.5f, 0.5f}}, // 1, 0, 0 +y
    {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, {{2.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},

    {{2.0f, 1.0f, 1.0f}, {1.0f, 0.5f}}, {{2.0f, 2.0f, 1.0f}, {0.5f, 0.5f}}, // 2, 1, 0 -x
    {{2.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, {{2.0f, 2.0f, 0.0f}, {0.5f, 0.0f}},

    {{2.0f, 2.0f, 1.0f}, {1.0f, 0.5f}}, {{3.0f, 2.0f, 1.0f}, {0.5f, 0.5f}}, // 2, 1, 0 +y
    {{2.0f, 2.0f, 0.0f}, {1.0f, 0.0f}}, {{3.0f, 2.0f, 0.0f}, {0.5f, 0.0f}},

    {{1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, {{2.0f, 0.0f, 1.0f}, {0.5f, 0.0f}}, // 1, 0, 0 +z
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.5f}}, {{2.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

    {{2.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, {{3.0f, 0.0f, 1.0f}, {0.5f, 0.0f}}, // 2, 0, 0
    {{2.0f, 1.0f, 1.0f}, {0.0f, 0.5f}}, {{3.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

    {{2.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, {{3.0f, 1.0f, 1.0f}, {0.5f, 0.0f}}, // 2, 1, 0 +z
    {{2.0f, 2.0f, 1.0f}, {0.0f, 0.5f}}, {{3.0f, 2.0f, 1.0f}, {0.5f, 0.5f}}
    };

    for(int i = 0; i < 13; i++) {
        index_vector.push_back(i * 4);
        index_vector.push_back(i * 4 + 1);
        index_vector.push_back(i * 4 + 2);

        index_vector.push_back(i * 4 + 1);
        index_vector.push_back(i * 4 + 2);
        index_vector.push_back(i * 4 + 3);
    }
}

std::vector<Vertex> vertex_vector1 = {
    {{0.0f, 1.0f, 2.0f}, {0.25, 0.375}},
    {{0.0f, -1.0f, 2.0f}, {0.5, 0.375}},
    {{0.0f, 1.0f, 0.0f}, {0.25, 0.25}},
    {{0.0f, -1.0f, 2.0f}, {0.5, 0.375}},
    {{0.0f, 1.0f, 0.0f}, {0.25, 0.25}},
    {{0.0f, -1.0f, 0.0f}, {0.5, 0.25}}
};

int main() {
    load_vertices();

    int width = 1000, height = 600;
    glfwInit();

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
    /*glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 5));
    glEnableVertexAttribArray(2);*/

    GLuint ebo_id;
    glCreateBuffers(1, &ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_vector.size() * sizeof(uint32_t), index_vector.data(), GL_STATIC_DRAW);



    GLuint vao_id1;
    glCreateVertexArrays(1, &vao_id1);
    glBindVertexArray(vao_id1);

    GLuint array_buffer_id1;
    glCreateBuffers(1, &array_buffer_id1);
    glBindBuffer(GL_ARRAY_BUFFER, array_buffer_id1);
    glBufferData(GL_ARRAY_BUFFER, vertex_vector1.size() * sizeof(Vertex), vertex_vector1.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);




    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    GLuint program = create_shader_program(v_src, f_src);

    stbi_set_flip_vertically_on_load(1);
    std::array<int, 3> info;
    GLuint texture = load_texture("res\\tiles\\3d_tileset.png", info);
    GLuint player_texture = load_texture("res\\entities\\Player\\avergreen_spritesheet.png", info);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    //glPolygonMode(GL_BACK, GL_LINE);

    time_t start_time = get_time();

    bool should_close = false;

    std::cout << index_vector.size() << "\n";
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

        if(!(move_vector.x == 0.0f && move_vector.y == 0.0f)) {
            move_vector = glm::normalize(move_vector);
            std::cout << camera_pos.x << " " << camera_pos.y << " " << camera_pos.z << "\n";
        } 
        float movement_factor = speed * (double(delta_time) / 1000000000);

        if(user_input_array[GLFW_KEY_LEFT_CONTROL]) {
            move_vector *= movement_factor * 2.0f;
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
        
        glClearColor(0.2, 0.2, 0.2, 1.0);
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
        float near = 0.1f;
        float far = 100.0f;

        glm::mat4 projection_matrix = glm::perspective(45.0f, float(width) / height, near, far);//glm::ortho(left, right, bottom, top, near, far);

        glm::mat4 proj_view_mat = projection_matrix * view_matrix;

        glBindVertexArray(vao_id);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glUniformMatrix4fv(0, 1, GL_FALSE, &proj_view_mat[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &identity[0][0]);

        glDrawElements(GL_TRIANGLES, index_vector.size(), GL_UNSIGNED_INT, 0);

        glm::vec2 diff = glm::vec2(camera_pos) - glm::vec2(1.0f, 2.5f); 
        glm::vec3 move(1.0f, 2.5f, 0.0f);
        glm::mat4x4 trans_mat_billboard = identity;
        trans_mat_billboard = glm::translate(trans_mat_billboard, move);
        trans_mat_billboard = glm::rotate(trans_mat_billboard, atan2(diff.y, diff.x), up);

        glBindVertexArray(vao_id1);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, player_texture);
        
        glUniformMatrix4fv(0, 1, GL_FALSE, &proj_view_mat[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &trans_mat_billboard[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, vertex_vector1.size());



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