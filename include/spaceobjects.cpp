#pragma once
#define _USE_MATH_DEFINES
#include <SFML-2.5.1\include\SFML\Graphics.hpp>
#include <cmath>
#include <vector>

#include "spacefunctions.cpp"

double G = 0.0001;

struct Planemo;
struct Barycenter;
struct Ring;

struct Planemo {
    std::string name;
    std::string type;
    int mk_class;
    
    sf::Vector2<double> position;
    sf::Vector2<double> view_position;

    bool is_star = false;

    Planemo* light_source;
    Planemo* parent = NULL;

    sf::RenderWindow* window;
    sf::Shader* shader = NULL;
    sf::Clock* clock;
    sf::Texture* tex = NULL;
    float axial_tilt = 0;
    float rotation_time = 0;
    double start_angle = 0;
    double current_angle = 0;
    double orbital_radius;
    double orbital_period;

    double density;
    double radius;
    double mass;
    sf::Vector3f color;
    sf::Color sfcolor;

    bool retrograde = false;
    bool small_crosshair = false;

    sf::RectangleShape draw_rect;

    Planemo(sf::RenderWindow* p_window, sf::Clock* p_clock, std::string p_name);

    void planet(sf::Shader* p_shader, Planemo* p_parent, bool p_retrograde, Planemo* p_light_source, double p_mass, double p_density, sf::Vector2<double> p_position, sf::Vector3f p_color, std::string p_type, bool p_scrosshair);
    void star(sf::Shader* p_shader, Planemo* p_parent, bool p_retrograde, double p_mass, double p_density, sf::Vector2<double> p_position, sf::Vector3f p_color, std::string p_type, int p_class, bool p_scrosshair);
    void render(sf::Vector2<double> camera_pos, double scale);

    void update_position();
};

void Planemo::update_position() {
    current_angle = ((double(clock->getElapsedTime().asMicroseconds()) / 1000000) / orbital_period * 360 + start_angle) * (M_PI / 180);
    position = sf::Vector2<double>(cos(current_angle), sin(current_angle)) * orbital_radius + parent->position;
}

Planemo::Planemo(sf::RenderWindow* p_window, sf::Clock* p_clock, std::string p_name) {
    window = p_window;
    clock = p_clock;

    name = p_name;
}

void Planemo::planet(sf::Shader* p_shader, Planemo* p_parent, bool p_retrograde, Planemo* p_light_source, double p_mass, double p_density, sf::Vector2<double> p_position, sf::Vector3f p_color, std::string p_type, bool p_scrosshair) {
    shader = p_shader;
    light_source = p_light_source;
    parent = p_parent;
    retrograde = p_retrograde;
    
    density = p_density;
    radius = cbrt((3 * p_mass) / (4 * p_density * M_PI)) * 20;
    mass = p_mass;

    color = p_color;
    sfcolor = sf::Color(color.x * 255, color.y * 255, color.z * 255);
    type = p_type;
    small_crosshair = p_scrosshair;

    position = p_position;
    view_position = p_position;
}

void Planemo::star(sf::Shader* p_shader, Planemo* p_parent, bool p_retrograde, double p_mass, double p_density, sf::Vector2<double> p_position, sf::Vector3f p_color, std::string p_type, int p_class, bool p_scrosshair) {
    shader = p_shader;
    parent = p_parent;
    retrograde = p_retrograde;

    density = p_density;
    radius = cbrt((3 * p_mass) / (4 * p_density * M_PI)) * 20;
    mass = p_mass;

    is_star = true;
    mk_class = p_class;
    color = p_color;
    sfcolor = sf::Color(color.x * 255, color.y * 255, color.z * 255);
    type = p_type;
    small_crosshair = p_scrosshair;

    position = p_position;
    view_position = p_position;
}

void Planemo::render(sf::Vector2<double> camera_pos, double scale) {
    draw_rect.setSize(sf::Vector2f(radius * 2 * scale, radius * 2 * scale));
    if(draw_rect.getSize().x < 1.5) {
        if(small_crosshair) {
            draw_rect.setSize(sf::Vector2f(7, 7));
            draw_rect.setPosition((sf::Vector2f)((-camera_pos + view_position) * scale + ((sf::Vector2<double>)window->getSize() / (double)2 - sf::Vector2<double>(3.5, 3.5))));
            if(collision(draw_rect, sf::RectangleShape((sf::Vector2f)window->getSize()))) {
                sf::Vector2f pos = draw_rect.getPosition();
                pos = sf::Vector2f(round(pos.x), round(pos.y));
                sf::Vertex line0[] {
                    sf::Vertex(sf::Vector2f(pos.x, pos.y), sfcolor),
                    sf::Vertex(sf::Vector2f(pos.x + 7, pos.y + 7), sfcolor)
                };
                sf::Vertex line1[] {
                    sf::Vertex(sf::Vector2f(pos.x + 7, pos.y), sfcolor),
                    sf::Vertex(sf::Vector2f(pos.x, pos.y + 7), sfcolor)
                };
                window->draw(line0, 2, sf::Lines);
                window->draw(line1, 2, sf::Lines);
            }
        } else {
            draw_rect.setSize(sf::Vector2f(15, 15));
            draw_rect.setPosition((sf::Vector2f)((-camera_pos + view_position) * scale + ((sf::Vector2<double>)window->getSize() / (double)2 - sf::Vector2<double>(7.5, 7.5))));
            if(collision(draw_rect, sf::RectangleShape((sf::Vector2f)window->getSize()))) {
                sf::Vector2f pos = draw_rect.getPosition();
                pos = sf::Vector2f(round(pos.x), round(pos.y));
                sf::Vertex line0[] {
                    sf::Vertex(sf::Vector2f(pos.x, pos.y), sfcolor),
                    sf::Vertex(sf::Vector2f(pos.x + 15, pos.y + 15), sfcolor)
                };
                sf::Vertex line1[] {
                    sf::Vertex(sf::Vector2f(pos.x + 15, pos.y), sfcolor),
                    sf::Vertex(sf::Vector2f(pos.x, pos.y + 15), sfcolor)
                };
                window->draw(line0, 2, sf::Lines);
                window->draw(line1, 2, sf::Lines);
            }
        }
    } else {
        draw_rect.setPosition((sf::Vector2f)((-camera_pos + view_position) * scale + ((sf::Vector2<double>)window->getSize() / (double)2 - sf::Vector2<double>(radius * scale, radius * scale))));
        if(collision(draw_rect, sf::RectangleShape((sf::Vector2f)window->getSize()))) {
            if(is_star) {
                shader->setUniform("u_resolution", (sf::Glsl::Vec2)window->getSize());
                shader->setUniform("u_shapeSize", (sf::Glsl::Vec2)draw_rect.getSize());
                shader->setUniform("u_shapePos", (sf::Glsl::Vec2)draw_rect.getPosition());
                shader->setUniform("u_color", (sf::Glsl::Vec3)color);

                window->draw(draw_rect, shader);
            } else {
                if(light_source != NULL) {
                    sf::Vector2<double> difference = light_source->position - position;
                    float lightAngle = atan2(-difference.y, difference.x) * 180 / M_PI;
                    if(lightAngle < 0) {
                        lightAngle = 360 + lightAngle;
                    }

                    shader->setUniform("u_resolution", (sf::Glsl::Vec2)window->getSize());
                    shader->setUniform("u_shapeSize", (sf::Glsl::Vec2)draw_rect.getSize());
                    shader->setUniform("u_shapePos", (sf::Glsl::Vec2)draw_rect.getPosition());
                    shader->setUniform("u_lightAngle", lightAngle);

                    if(tex != NULL) {
                        shader->setUniform("u_texture", *tex);
                        shader->setUniform("u_time", (float)((*clock).getElapsedTime().asMilliseconds()));
                        shader->setUniform("u_axialTilt", axial_tilt);
                        shader->setUniform("u_secondsToRotate", rotation_time);
                    } else {
                        shader->setUniform("u_color", (sf::Glsl::Vec3)color);
                    }

                    window->draw(draw_rect, shader);
                } else {
                    shader->setUniform("u_resolution", (sf::Glsl::Vec2)window->getSize());
                    shader->setUniform("u_shapeSize", (sf::Glsl::Vec2)draw_rect.getSize());
                    shader->setUniform("u_shapePos", (sf::Glsl::Vec2)draw_rect.getPosition());
                    shader->setUniform("u_color", (sf::Glsl::Vec3)color);

                    window->draw(draw_rect, shader);
                }
            } 
        }
    }
}

//struct Ring;