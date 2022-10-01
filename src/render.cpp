#pragma once
#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>
#include <array>
#include <vector>
#include <iostream>

#include "global.cpp"

const char* vertex_shader = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_f;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;
layout(location = 7) uniform vec4 texture_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / (window_size / 2), z_value, 1.0);
    tex_coord_f = tex_coord * texture_rect.zw + texture_rect.xy;
}
)""";

const char* fragment_shader = R"""(
#version 460 core

in vec2 tex_coord_f;

layout(location = 11) uniform sampler2D input_texture;

void main() {
    gl_FragColor = texture(input_texture, tex_coord_f);
    if(gl_FragColor.w == 0.0) discard;
}
)""";

const char* vertex_shader_chunk = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_f;

layout(location = 0) uniform vec2 window_size;
layout(location = 2) uniform vec2 top_left;
layout(location = 4) uniform vec2 visual_size;

layout(location = 6) uniform vec2 texture_numbers;
layout(location = 8) uniform int textures[256];

void main() {
    int current_texture = textures[gl_InstanceID];
    gl_Position = vec4(((pos + vec2(gl_InstanceID % 16, gl_InstanceID / 16)) * visual_size + top_left) / (window_size / 2), 0.95, 1.0);
    tex_coord_f = (tex_coord + vec2(current_texture % int(texture_numbers.x), current_texture / int(texture_numbers.x))) / texture_numbers;
}
)""";

const char* vertex_shader_chunk_depth = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_f;

layout(location = 0) uniform vec2 window_size;
layout(location = 2) uniform vec2 top_left;
layout(location = 4) uniform vec2 visual_size;

layout(location = 6) uniform vec2 texture_numbers;
layout(location = 8) uniform float y_coordinate_chunk;
layout(location = 9) uniform int textures[256];
layout(location = 265) uniform float offsets[256];

void main() {
    int current_texture = textures[gl_InstanceID];
    gl_Position = vec4(((pos + vec2(gl_InstanceID % 16, gl_InstanceID / 16)) * visual_size + top_left) / (window_size / 2), (int(gl_InstanceID * 0.0625) + offsets[gl_InstanceID] * 0.0625) * 0.01 + y_coordinate_chunk, 1.0);
    tex_coord_f = (tex_coord + vec2(current_texture % int(texture_numbers.x), current_texture / int(texture_numbers.x))) / texture_numbers;
}
)""";

const char* fragment_shader_chunk = R"""(
#version 460 core

in vec2 tex_coord_f;

uniform sampler2D input_texture;

void main() {
    gl_FragColor = texture(input_texture, tex_coord_f);
    if(gl_FragColor.w == 0.0) discard;
}
)""";

const char* vertex_shader_text = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_f;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;
layout(location = 7) uniform vec4 texture_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / (window_size / 2), z_value, 1.0);
    tex_coord_f = tex_coord * texture_rect.zw + texture_rect.xy;
}
)""";

const char* fragment_shader_text = R"""(
#version 460 core

in vec2 tex_coord_f;

layout(location = 11) uniform vec4 color;
layout(location = 15) uniform sampler2D input_texture;

void main() {
    vec4 tex_color = texture(input_texture, tex_coord_f);
    gl_FragColor = tex_color;
}
)""";

const char* vertex_shader_color = R"""(
#version 460 core
layout(location = 0) in vec2 pos;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / (window_size / 2), z_value, 1.0);
}
)""";

const char* fragment_shader_color = R"""(
#version 460 core

layout(location = 7) uniform vec4 color;

void main() {
    gl_FragColor = color;
}
)""";

const char* v_src = R"""(
#version 460 core
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) uniform mat4 proj_view_mat;
layout(location = 1) uniform mat4 transform_mat;

out vec2 tex_coord;

void main() {
    gl_Position = (proj_view_mat * transform_mat) * vec4(in_pos, 1.0);
    tex_coord = in_tex_coord;
}
)""";

const char* f_src = R"""(
#version 460 core

in vec2 tex_coord;

uniform sampler2D input_texture;

out vec4 FragColor;

void main() {
    ivec2 coord = ivec2(floor(tex_coord));
    vec4 color = texelFetch(input_texture, coord, 0);
    //if(color.w == 0.0) FragColor = ivec4(1.0f, 0.0f, 1.0f, 1.0f);
    //else {
        if(coord.y == 112) {
            FragColor = ivec4(1.0f, 0.0f, 0.0f, 1.0f);
        } else {
            FragColor = color;
        }
    //}
}
)""";

/*unsigned int create_shader(const char* vertex_shader, const char* fragment_shader) {
    unsigned int v_shader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v_shader, 1, &vertex_shader, NULL);
    glShaderSource(f_shader, 1, &fragment_shader, NULL);
    glCompileShader(v_shader);
    glCompileShader(f_shader);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, v_shader);
    glAttachShader(shader, f_shader);
    glLinkProgram(shader);

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return shader;
}*/

void APIENTRY glDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;
    
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

GLuint create_shader(const char* vertex_shader, const char* fragment_shader) {
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

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return program;
}

unsigned int load_texture(char address[], std::array<int, 3> &info) {
    /*unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    unsigned char *data = stbi_load(address, &info[0], &info[1], &info[2], 4); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info[0], info[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return texture_id;*/

    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(address, &width, &height, &nrChannels, 0);
    if(data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return texture_id;
}

void draw_tile(unsigned int shader, unsigned int texture, std::array<float, 4> display_rect, std::array<int, 6> texture_rect, std::array<int, 2> window_size, float z_value) {
    glUseProgram(shader);

    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1f(0, z_value);
    glUniform2f(1, window_size[0], window_size[1]);
    glUniform4f(3, display_rect[0], display_rect[1], display_rect[2], display_rect[3]);
    glUniform4f(7, (float)texture_rect[0] / texture_rect[4], (float)texture_rect[1] / texture_rect[5], (float)texture_rect[2] / texture_rect[4], (float)texture_rect[3] / texture_rect[5]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
};

float vertices[] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f
};

Vertex floor_vertices[] = {
    Vertex{{0.0f, 0.0f, 0.5f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.5f}, {16.0f, 0.0f}},
    Vertex{{0.0f, 1.0f, 0.5f}, {0.0f, 16.0f}},
    Vertex{{1.0f, 1.0f, 0.5f}, {16.0f, 16.0f}}
};

Vertex object_vertices[] = {
    Vertex{{0.0f, 0.0f, 0.1f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.1f}, {16.0f, 0.0f}},
    Vertex{{0.0f, 1.0f, 0.5f}, {0.0f, 16.0f}},
    Vertex{{1.0f, 1.0f, 0.5f}, {16.0f, 16.0f}}
};