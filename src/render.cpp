#pragma once
#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>
#include <array>

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
    if(tex_color.w == 0.0) discard;
    gl_FragColor = color;
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

unsigned int create_shader(const char* vertex_shader, const char* fragment_shader) {
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
}

unsigned int generate_texture(char address[], std::array<int, 3> &info) {
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

void draw_tile(unsigned int shader, unsigned int texture, std::array<float, 4> display_rect, std::array<int, 6> texture_rect, std::array<int, 2> window_size, float z_value) {
    glUseProgram(shader);

    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1f(0, z_value);
    glUniform2f(1, window_size[0], window_size[1]);
    glUniform4f(3, display_rect[0], display_rect[1], display_rect[2], display_rect[3]);
    glUniform4f(7, (float)texture_rect[0] / texture_rect[4], (float)texture_rect[1] / texture_rect[5], (float)texture_rect[2] / texture_rect[4], (float)texture_rect[3] / texture_rect[5]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}