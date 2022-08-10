#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>

#include <vector>
#include <array>
#include <iostream>

const char* v_src = R"""(
#version 460 core
layout(location = 0) in vec3 inpos;
layout(location = 1) in vec3 incol;

out vec3 fcol;

void main() {
    gl_Position = vec4(inpos, 1.0);
    fcol = incol;
}
)""";

const char* f_src = R"""(
#version 460 core

in vec3 fcol;

void main() {
    gl_FragColor = vec4(fcol, 1.0);
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

struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

std::vector<Vertex> vertex_vector;
std::vector<uint32_t> index_vector = {0, 1, 2, 0, 2, 3};

int main() {
    glfwInit();

    vertex_vector.push_back({{0.5f, 0.7f, 0.5f}, {1.0f, 0.0f, 0.0f}});
    vertex_vector.push_back({{-0.5f, 0.6f, 0.5f}, {0.0f, 1.0f, 0.0f}});
    vertex_vector.push_back({{-0.7f, -0.9f, 0.5f}, {0.0f, 0.0f, 1.0f}});
    vertex_vector.push_back({{0.6f, -0.8f, 0.5f}, {0.6f, 0.0f, 1.0f}});

    GLFWwindow* window = glfwCreateWindow(1000, 600, "test", NULL, NULL);
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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    GLuint ebo_id;
    glCreateBuffers(1, &ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_vector.size() * sizeof(uint32_t), index_vector.data(), GL_STATIC_DRAW);


    GLuint shader = create_shader(v_src, f_src);

    bool should_close = false;
    while(!should_close) {
        glClear(GL_COLOR_BUFFER_BIT);
        
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glBindVertexArray(vao_id);

        glUseProgram(shader);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);

        should_close = glfwWindowShouldClose(window);
    }

    return 0;
}