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
layout(location = 0) in vec3 inpos;
layout(location = 1) in vec2 in_texcoord;

uniform mat4 trans_mat;
uniform mat4 proj_mat;
uniform mat4 view_mat;

out vec2 f_texcoord;

void main() {
    gl_Position = proj_mat * view_mat * (trans_mat * vec4(inpos, 1.0));
    f_texcoord = in_texcoord;
}
)""";

const char* f_src = R"""(
#version 460 core

in vec2 f_texcoord;

uniform sampler2D input_texture;

out vec4 FragColor;

void main() {
    FragColor = texture(input_texture, f_texcoord);
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

float dist = 200.0f;
float xdeg = 0.0f;
float zdeg = 0.0f;
glm::vec3 camera_dir(1.0f, 0.0f, 0.0f);
glm::vec3 camera_pos(-20.0f, 0.0f, 0.0f);
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
};

std::vector<Vertex> vertex_vector;
std::vector<uint32_t> index_vector;// = {0, 1, 2, 1, 2, 3, 0, 2, 4, 2, 4, 6, 0, 1, 4, 1, 4, 5, 1, 3, 5, 3, 5, 7, 2, 3, 6, 3, 6, 7, 4, 5, 6, 5, 6, 7};

unsigned int load_texture(char address[], std::array<int, 3> &info) {
    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    unsigned char *data = stbi_load(address, &info[0], &info[1], &info[2], 0); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info[0], info[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return texture_id;
}

void generate_sphere(std::vector<Vertex>& vertex_vector, std::vector<uint32_t>& index_vector, float radius, int div, siv::PerlinNoise& noise) {
    //vertex_vector.resize((side_split + 1) * (side_split + 1));
    //index_vector.resize(side_split * side_split * 6);
    /*for(float j = 0; j < div_vert + 1; j++) {
        float j_radians = glm::radians(j / div_vert * 180 - 90);
        float width_at_height = cos(j_radians);
        for(float i = 0; i < div_around + 1; i++) {
            float i_radians = glm::radians(i / div_around * 360);
            glm::vec3 position(cos(i_radians) * width_at_height, sin(i_radians) * width_at_height, sin(j_radians));

            vertex_vector.push_back({position * radius, glm::vec2(i / div_around, j / div_vert)});
        }        
    }

    for(int j = 0; j < div_vert; j++) {
        for(int i = 0; i < div_around; i++) {
            int current_index = i + j * (div_around + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div_around + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div_around + 2);
            index_vector.push_back(current_index + div_around + 1);
        }
    }*/
    int vert_per_face = (div + 1) * (div + 1);

    // top +z
    for(float y = 0; y < div + 1; y++) {
        for(float x = 0; x < div + 1; x++) {
            glm::vec3 position(-1.0f + x / div * 2.0f, -1.0f + y / div * 2.0f, 1.0f);
            position = glm::normalize(position);
            
            position *= 1 + 0.2 * noise.normalizedOctave3D_01(position.x, position.y, position.z, 2);
            position.z *= 0.65;
            
            vertex_vector.push_back({position * radius, {atan2f(position.y, position.x) / (2 * M_PI), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}});
        }
    }
    for(int y = 0; y < div; y++) {
        for(int x = 0; x < div; x++) {
            uint32_t current_index = x + y * (div + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div + 2);
            index_vector.push_back(current_index + div + 1);
        }
    }

    // side +y
    for(float z = 0; z < div + 1; z++) {
        for(float x = 0; x < div + 1; x++) {
            glm::vec3 position(-1.0f + x / div * 2.0f, 1.0f, -1.0f + z / div * 2.0f);
            position = glm::normalize(position);
            
            position *= 1 + 0.2 * noise.normalizedOctave3D_01(position.x, position.y, position.z, 2);
            position.z *= 0.65;
            
            vertex_vector.push_back({position * radius, {atan2f(position.y, position.x) / (2 * M_PI), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}});
        }
    }
    for(int z = 0; z < div; z++) {
        for(int x = 0; x < div; x++) {
            uint32_t current_index = vert_per_face + x + z * (div + 1);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + div + 2);
            index_vector.push_back(current_index + 1);
        }
    }

    // side +x
    for(float z = 0; z < div + 1; z++) {
        for(float y = 0; y < div + 1; y++) {
            glm::vec3 position(1.0f, -1.0f + y / div * 2.0f, -1.0f + z / div * 2.0f);
            position = glm::normalize(position);
            
            position *= 1 + 0.2 * noise.normalizedOctave3D_01(position.x, position.y, position.z, 2);
            position.z *= 0.65;
            
            vertex_vector.push_back({position * radius, {atan2f(position.y, position.x) / (2 * M_PI), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}});
        }
    }
    for(int z = 0; z < div; z++) {
        for(int y = 0; y < div; y++) {
            uint32_t current_index = vert_per_face * 2 + y + z * (div + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div + 2);
            index_vector.push_back(current_index + div + 1);
        }
    }

    // bottom -z
    for(float y = 0; y < div + 1; y++) {
        for(float x = 0; x < div + 1; x++) {
            glm::vec3 position(-1.0f + x / div * 2.0f, -1.0f + y / div * 2.0f, -1.0f);
            position = glm::normalize(position);
            
            position *= 1 + 0.2 * noise.normalizedOctave3D_01(position.x, position.y, position.z, 2);
            position.z *= 0.65;
            
            vertex_vector.push_back({position * radius, {atan2f(position.y, position.x) / (2 * M_PI), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}});
        }
    }
    for(int y = 0; y < div; y++) {
        for(int x = 0; x < div; x++) {
            uint32_t current_index = vert_per_face * 3 + x + y * (div + 1);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + div + 2);
            index_vector.push_back(current_index + 1);
        }
    }

    // side -y
    for(float z = 0; z < div + 1; z++) {
        for(float x = 0; x < div + 1; x++) {
            glm::vec3 position(-1.0f + x / div * 2.0f, -1.0f, -1.0f + z / div * 2.0f);
            position = glm::normalize(position);
            
            position *= 1 + 0.2 * noise.normalizedOctave3D_01(position.x, position.y, position.z, 2);
            position.z *= 0.65;
            
            vertex_vector.push_back({position * radius, {atan2f(position.y, position.x) / (2 * M_PI), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}});
        }
    }
    for(int z = 0; z < div; z++) {
        for(int x = 0; x < div; x++) {
            uint32_t current_index = vert_per_face * 4 + x + z * (div + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index + div + 2);
            index_vector.push_back(current_index + div + 1);
        }
    }

    // side -x
    for(float z = 0; z < div + 1; z++) {
        for(float y = 0; y < div + 1; y++) {
            glm::vec3 position(-1.0f, -1.0f + y / div * 2.0f, -1.0f + z / div * 2.0f);
            position = glm::normalize(position);
            
            position *= 1 + 0.2 * noise.normalizedOctave3D_01(position.x, position.y, position.z, 2);
            position.z *= 0.65;
            
            vertex_vector.push_back({position * radius, {atan2f(position.y, position.x) / (2 * M_PI), atanf(position.z / hypot(position.x, position.y)) / M_PI + 0.5f}});
        }
    }
    for(int z = 0; z < div; z++) {
        for(int y = 0; y < div; y++) {
            uint32_t current_index = vert_per_face * 5 + y + z * (div + 1);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + 1);
            index_vector.push_back(current_index);
            index_vector.push_back(current_index + div + 1);
            index_vector.push_back(current_index + div + 2);
            index_vector.push_back(current_index + 1);
        }
    }
}

int main() {
    int width = 1000, height = 600;
    glfwInit();

    /*vertex_vector.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}});
    vertex_vector.push_back({{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}});
    vertex_vector.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}});
    vertex_vector.push_back({{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}});
    vertex_vector.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}});
    vertex_vector.push_back({{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}});
    vertex_vector.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}});
    vertex_vector.push_back({{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}});*/
    siv::PerlinNoise noise(4097);
    generate_sphere(vertex_vector, index_vector, 16.0f, 100, noise);

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

    GLuint ebo_id;
    glCreateBuffers(1, &ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_vector.size() * sizeof(uint32_t), index_vector.data(), GL_STATIC_DRAW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glm::vec3 scale(1.0f, 1.0f, 1.0f);
    float rotation = 0.0f;
    glm::vec3 position(0.0f, 0.0f, 0.0f);

    stbi_set_flip_vertically_on_load(1);

    glPolygonMode(GL_BACK, GL_LINE);

    std::array<int, 3> data_array;
    GLuint planet_texture = load_texture("res\\planets\\terra.png", data_array);

    bool should_close = false;
    while(!should_close) {
        glfwGetFramebufferSize(window, &width, &height);
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

        camera_pos += glm::vec3(move_vector, 0.0f);

        if(user_input_array[GLFW_KEY_SPACE]) {
            camera_pos.z += 0.1f;
        }
        if(user_input_array[GLFW_KEY_LEFT_SHIFT]) {
            camera_pos.z -= 0.1f;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glBindVertexArray(vao_id);

        glm::mat4 transform = glm::identity<glm::mat4>();
        transform = glm::scale(transform, scale);
        transform = glm::rotate(transform, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::translate(transform, position);

        glm::vec3 center = camera_pos + camera_dir;
        glm::vec3 up(0.0f, 0.0f, 1.0f);
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
        float far = 2048.0f;

        glm::mat4 projection = glm::perspective(45.0f, float(width) / height, near, far);//glm::ortho(left, right, bottom, top, near, far);

        glUseProgram(program);

        glBindTexture(GL_TEXTURE_2D, planet_texture);

        glUniformMatrix4fv(2, 1, GL_FALSE, &transform[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(0, 1, GL_FALSE, &projection[0][0]);

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