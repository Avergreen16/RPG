#pragma once
#define _USE_MATH_DEFINES
#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>
#include <cmath>
#include <vector>

const std::string dark_fragment_shader = R"""(#version 330 core
#ifdef GL_ES
precision mediump float;
#endif
 
uniform vec2 u_resolution;
uniform vec2 u_shapeSize;
uniform vec2 u_shapePos;
uniform vec3 u_color;
uniform float u_lightAngle;

void main() {
    float pi = 3.14159265358979324;
    vec2 relativeCoord = vec2(gl_FragCoord.x - u_shapePos.x, gl_FragCoord.y * -1 + u_resolution.y - u_shapePos.y);
    vec2 st = relativeCoord/u_shapeSize;

    float py = st.y * 2 - 1;
    float px = st.x * 2 - 1;

    if(px * px + py * py >= 1) {
        discard;
    }

    gl_FragColor = vec4(u_color / 4, 1.0);
})""";

const std::string shadow_fragment_shader = R"""(#version 330 core
#ifdef GL_ES
precision mediump float;
#endif
 
uniform vec2 u_resolution;
uniform vec2 u_shapeSize;
uniform vec2 u_shapePos;
uniform vec3 u_color;
uniform float u_lightAngle;

void main() {
    float pi = 3.14159265358979324;
    vec2 relativeCoord = vec2(gl_FragCoord.x - u_shapePos.x, gl_FragCoord.y * -1 + u_resolution.y - u_shapePos.y);
    vec2 st = relativeCoord/u_shapeSize;

    float py = st.y * 2 - 1;
    float px = st.x * 2 - 1;

    if(px * px + py * py >= 1) {
        discard;
    }
    
    float width_at_height2 = sqrt(1 - py * py);
    float px2 = asin(px / width_at_height2) * 2/pi;
    float lightAngle = (u_lightAngle + 90) / 360;
    if(lightAngle >= 1) {
        lightAngle -= 1;
    }
    float light_xCoord = (lightAngle) * 4 - 2;
    float shadow1_xCoord = light_xCoord - 1;
    float shadow2_xCoord = light_xCoord + 1;

    if(px2 > shadow1_xCoord && px2 < shadow2_xCoord) {
        gl_FragColor = vec4(u_color / 3, 1.0);
    } else {
        gl_FragColor = vec4(u_color, 1.0);
    }
})""";

const std::string star_fragment_shader = R"""(#version 330 core
#ifdef GL_ES
precision mediump float;
#endif
 
uniform vec2 u_resolution;
uniform vec2 u_shapeSize;
uniform vec2 u_shapePos;
uniform vec3 u_color;

void main() {
    float pi = 3.14159265358979324;
    vec2 relativeCoord = vec2(gl_FragCoord.x - u_shapePos.x, gl_FragCoord.y * -1 + u_resolution.y - u_shapePos.y);
    vec2 st = relativeCoord/u_shapeSize;

    float py = st.y * 2 - 1;
    float px = st.x * 2 - 1;

    if(px * px + py * py >= 1) {
        discard;
    }

    gl_FragColor = vec4(u_color, 1.0);
})""";

const std::string texture_fragment_shader = R"""(#version 330 core
#ifdef GL_ES
precision mediump float;
#endif
 
uniform vec2 u_resolution;
uniform vec2 u_shapeSize;
uniform vec2 u_shapePos;
uniform float u_time;
uniform sampler2D u_texture;
uniform float u_axialTilt;
uniform float u_secondsToRotate;
uniform float u_lightAngle;

void main() {
    float pi = 3.14159265358979324;
    vec2 relativeCoord = vec2(gl_FragCoord.x - u_shapePos.x, gl_FragCoord.y * -1 + u_resolution.y - u_shapePos.y);
    vec2 st = relativeCoord/u_shapeSize;

    float py = st.y * 2 - 1;
    float px = st.x * 2 - 1;

    float angle_in_radians = u_axialTilt * pi / 180;
    float py_rot = py * cos(angle_in_radians) + px * sin(angle_in_radians);
    float px_rot = px * cos(angle_in_radians) - py * sin(angle_in_radians);

    if(px_rot * px_rot + py_rot * py_rot >= 1) {
        discard;
    }
    
    float width_at_height = sqrt(1 - py_rot * py_rot);
    float width_at_height2 = sqrt(1 - py * py);
    float py2 = asin(py_rot) * 2/pi;
    float px2 = asin(px_rot / width_at_height) * 2/pi;
    float px3 = asin(px / width_at_height2) * 2/pi;

    float xCoord = (px2 + 1) / 4 + u_time / (1000 * u_secondsToRotate);
    float tex_x = xCoord - floor(xCoord);
    float tex_y = (py2 + 1) / 2;

    vec4 color = texture(u_texture, vec2(tex_x, tex_y));

    float lightAngle = (u_lightAngle + 90) / 360;
    if(lightAngle >= 1) {
        lightAngle -= 1;
    }
    float light_xCoord = (lightAngle) * 4 - 2;
    float shadow1_xCoord = light_xCoord - 1;
    float shadow2_xCoord = light_xCoord + 1;

    if(px3 > shadow1_xCoord && px3 < shadow2_xCoord) {
        gl_FragColor = vec4(color.xyz / 4, 1.0);
    } else {
        gl_FragColor = vec4(color.xyz, 1.0);
    }
})""";

const std::string circle_fragment_shader = R"""(#version 330 core
#ifdef GL_ES
precision mediump float;
#endif
 
uniform vec2 u_resolution;
uniform vec2 u_shapeSize;
uniform vec2 u_shapePos;
uniform vec4 u_color;

void main() {
    float pi = 3.14159265358979324;
    vec2 relativeCoord = vec2(gl_FragCoord.x - u_shapePos.x, gl_FragCoord.y * -1 + u_resolution.y - u_shapePos.y);
    vec2 st = relativeCoord/u_shapeSize;

    float py = st.y * 2 - 1;
    float px = st.x * 2 - 1;

    if(px * px + py * py >= 1) {
        discard;
    }

    gl_FragColor = u_color;
})""";

sf::Shader shadow_shader;
sf::Shader star_shader;
sf::Shader dark_shader;
sf::Shader circle_shader;
sf::Shader texture_shader;

void draw_circle(sf::Glsl::Vec4 color, sf::Vector2f center, float radius, sf::RenderWindow* window) {
    sf::RectangleShape rect;
    rect.setSize(sf::Vector2f(radius * 2, radius * 2));
    rect.setPosition(center - sf::Vector2f(radius, radius));
    circle_shader.setUniform("u_resolution", (sf::Glsl::Vec2)window->getSize());
    circle_shader.setUniform("u_shapeSize", sf::Glsl::Vec2(radius * 2, radius * 2));
    circle_shader.setUniform("u_shapePos", (sf::Glsl::Vec2)rect.getPosition());
    circle_shader.setUniform("u_color", color);

    window->draw(rect, &circle_shader);
}