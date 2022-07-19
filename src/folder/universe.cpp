#define _USE_MATH_DEFINES
#define GLFW_INCLUDE_NONE
#include <glfw\glfw3.h>
#include <glad\glad.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>

#include "Spaceobjects.cpp"
#include "shaders.cpp"

sf::Vector2<double> get_orbital_speed(double parent_mass, double orbital_radius, double angle) {
    double orbital_speed = sqrt(G * parent_mass / orbital_radius);
    return sf::Vector2<double>(orbital_speed * cos(angle), orbital_speed * sin(angle));
}

void add_satellite(sf::Shader* p_shader, Planemo* satellite, Planemo* parent, bool retrograde, Planemo* light_source, double orbital_radius, double angle, double p_mass, double p_density, sf::Vector3f color, std::string p_type, bool p_scrosshair, bool is_star = false, int p_class = 0) {
    //double radian_angle = angle * M_PI / 180;
    //sf::Vector2<double> position(parent->position.x + orbital_radius * cos(radian_angle), parent->position.y + orbital_radius * sin(radian_angle));
    sf::Vector2<double> position(0, 0);

    if(is_star) satellite->star(p_shader, parent, retrograde, p_mass, p_density, position, color, p_type, p_class, p_scrosshair);
    else satellite->planet(p_shader, parent, retrograde, light_source, p_mass, p_density, position, color, p_type, p_scrosshair);

    if(parent != NULL) {
        satellite->orbital_period = 2 * M_PI * sqrt(orbital_radius * orbital_radius * orbital_radius / (G * parent->mass));
        satellite->orbital_radius = orbital_radius;
        satellite->start_angle = angle;
    }
}

bool compare_planet_radius(Planemo* p1, Planemo* p2) {
    return p1->radius > p2->radius;
}

bool compare_planet_mass(Planemo* p1, Planemo* p2) {
    return p1->mass > p2->mass ? true : p1->mass == p2->mass ? p1->radius > p2->radius : false;
}

bool compare_planet_distance(Planemo* p1, Planemo* p2) { 
    return hypot(p1->position.x, p1->position.y) < hypot(p2->position.x, p2->position.y);
}

struct Solar_system {
    std::vector<Planemo*> spaceobjects;

    std::vector<Planemo*> stars;
};

bool is_planet_clicked(Planemo* p, sf::Vector2<double> camera_pos, sf::Vector2f mouse_pos, double scale) {
    sf::RectangleShape rect;
    rect.setSize(sf::Vector2f(p->radius * 2 * scale, p->radius * 2 * scale));
    if(rect.getSize().x < 1.5) {
        if(p->small_crosshair) {
            rect.setSize(sf::Vector2f(7, 7));
            rect.setPosition((sf::Vector2f)((-camera_pos + p->view_position) * scale + ((sf::Vector2<double>)p->window->getSize() / (double)2 - sf::Vector2<double>(3.5, 3.5))));
            if(collision(rect, sf::RectangleShape((sf::Vector2f)p->window->getSize())) && point_collision(rect, mouse_pos)) {
                return true;
            }
        } else {
            rect.setSize(sf::Vector2f(15, 15));
            rect.setPosition((sf::Vector2f)((-camera_pos + p->view_position) * scale + ((sf::Vector2<double>)p->window->getSize() / (double)2 - sf::Vector2<double>(7.5, 7.5))));
            if(collision(rect, sf::RectangleShape((sf::Vector2f)p->window->getSize())) && point_collision(rect, mouse_pos)) {
                return true;
            }
        }
    } else {
        rect.setPosition((sf::Vector2f)((-camera_pos + p->view_position) * scale + ((sf::Vector2<double>)p->window->getSize() / (double)2 - sf::Vector2<double>(p->radius * scale, p->radius * scale))));
        if(collision(rect, sf::RectangleShape((sf::Vector2f)p->window->getSize())) && point_collision(rect, mouse_pos) && circle_point_collision(mouse_pos, rect.getPosition() + sf::Vector2f(p->radius * scale, p->radius * scale), p->radius * scale)) {
            return true;
        }
    }
    return false;
}

enum view_setting{ORBIT, RADIUS, MASS, DISTANCE, SURFACE};
enum view_scale{STAR, GALAXY};

std::vector<Planemo*> p_vector;

int active_view_setting = ORBIT;
int active_view_scale = STAR;
Planemo* focus_object = NULL;
bool sorted = true;
bool center_focus_object = true;

sf::Sprite sprite;

std::vector<std::string> selected_object_text = {
    "Selected object: ",
    "Parent: ",
    "Mass: ",
    "Radius: ",
    "Density: ",
    "Type: ",
    "Orbital period: ",
    "Rotational Period: "
};

int main() {
    shadow_shader.loadFromMemory(shadow_fragment_shader, sf::Shader::Fragment);
    star_shader.loadFromMemory(star_fragment_shader, sf::Shader::Fragment);
    dark_shader.loadFromMemory(dark_fragment_shader, sf::Shader::Fragment);
    circle_shader.loadFromMemory(circle_fragment_shader, sf::Shader::Fragment);
    texture_shader.loadFromMemory(texture_fragment_shader, sf::Shader::Fragment);

    std::vector<sf::Texture> imgvector(5);
    
    imgvector[0].loadFromFile("startextures.png", sf::IntRect(92, 0, 15, 15));

    imgvector[1].loadFromFile("startextures.png", sf::IntRect(77, 0, 15, 15));

    imgvector[2].loadFromFile("startextures.png", sf::IntRect(54, 0, 23, 23));

    imgvector[3].loadFromFile("startextures.png", sf::IntRect(31, 0, 23, 23));

    imgvector[4].loadFromFile("startextures.png", sf::IntRect(0, 0, 31, 31));

    sf::Texture map_;
    map_.loadFromFile("map203x.png");
    sf::Sprite map_sprite(map_);
    sf::Texture map_2;
    map_2.loadFromFile("map206x.png");
    sf::Sprite map_2sprite(map_2);

    /*sf::Texture map_2;
    map_2.loadFromFile("surface_ares - Copy.png");
    sf::Sprite map_2sprite(map_2);*/

    //setup
    int frame = 0;
    int timeContainer = 0;
    sf::Clock clock;

    sf::Font font;
    font.loadFromFile("Q5text.ttf");
    sf::Text text;
    text.setFont(font);

    unsigned int window_width = 1600, window_height = 800;
    sf::RenderWindow window(sf::VideoMode(window_width, window_height), "Test Name", sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize);
    window.setFramerateLimit(100);

    double scale = 1; //0.5 * pow(1.2, -14);
    sf::Vector2<double> camera_pos(0.0f, 0.0f);
    sf::Vector2f mouse_pos(0.0f, 0.0f);

    //planets, stars, and moons
    RNG rng;
    rng.set_seed(0.7987987, 49.97843);
    Solar_system ss1;

    for(int i = 0; i < 100; i++) {
        ss1.spaceobjects.push_back(new Planemo(&window, &clock, std::to_string(i)));
    }

    ss1.spaceobjects[0]->star(&star_shader, NULL, false, 187000, pow((double)187000 / 200000 + 0.1 * rng.retrieve(), -1.3) * 1.4, sf::Vector2<double>(0, 0), sf::Vector3f(1.0f, 0.7f, 0.2f), "G-class star", 2, false);
    ss1.stars.push_back(ss1.spaceobjects[0]);

    add_satellite(&texture_shader, ss1.spaceobjects[1], ss1.spaceobjects[0], false, ss1.spaceobjects[0], 19300, -20, 20, 5.1, sf::Vector3f(0.2f, 0.9f, 0.7f), "wet terrestrial with life", false);
    ss1.spaceobjects[1]->tex = &map_;
    ss1.spaceobjects[1]->axial_tilt = -15;
    ss1.spaceobjects[1]->rotation_time = 4600;
    ss1.spaceobjects[1]->name = "Terra";

    add_satellite(&shadow_shader, ss1.spaceobjects[2], ss1.spaceobjects[1], false, ss1.spaceobjects[0], 217, 30, 0.087, 4.6, sf::Vector3f(0.6, 0.6, 0.75), "icy terrestrial", false);

    add_satellite(&shadow_shader, ss1.spaceobjects[3], ss1.spaceobjects[1], false, ss1.spaceobjects[0], 593, 170, 0.00013, 3.1, sf::Vector3f(0.55, 0.51, 0.5), "terrestrial asteroid", true);

    add_satellite(&texture_shader, ss1.spaceobjects[4], ss1.spaceobjects[0], false, ss1.spaceobjects[0], 13000, 40, 9, 4.7, sf::Vector3f(0.9f, 0.8f, 0.6f), "wet terrestrial with life", false);
    ss1.spaceobjects[4]->tex = &map_2;
    ss1.spaceobjects[4]->axial_tilt = 6;
    ss1.spaceobjects[4]->rotation_time = 6840;

    

    for(int i = 0; i < ss1.spaceobjects.size(); i++) {
        if(ss1.spaceobjects[i]->shader == NULL) {
            ss1.spaceobjects.erase(ss1.spaceobjects.begin() + i);
            i--;
        }
    }
    
    //loop
    while(window.isOpen()) {
        //input
        sf::Event event;
        while(window.pollEvent(event)) {
            switch(event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    window_width = event.size.width;
                    window_height = event.size.height;
                    
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                    break;
                case sf::Event::MouseMoved:
                    if(sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                        camera_pos -= ((sf::Vector2<double>)sf::Mouse::getPosition(window) - (sf::Vector2<double>)mouse_pos)/scale;
                    }
                    mouse_pos = (sf::Vector2f)sf::Mouse::getPosition(window);
                    break;
                case sf::Event::MouseWheelScrolled:
                    int delta = event.mouseWheelScroll.delta;
                    scale *= pow(1.2, delta);

                    if(scale < 0.00008) {
                        active_view_scale = GALAXY;
                    } else {
                        active_view_scale = STAR;
                    }

                    break;
            }
            if(event.type == event.MouseButtonPressed) {
                if(active_view_setting != SURFACE) {
                    if(active_view_scale == GALAXY) {
                        focus_object = NULL;
                    } else {
                        if(event.mouseButton.button == sf::Mouse::Right) {
                            bool planet_clicked = false;
                            for(Planemo* p : ss1.spaceobjects) {
                                if(is_planet_clicked(p, camera_pos, mouse_pos, scale)) {
                                    focus_object = p;
                                    planet_clicked = true;
                                    center_focus_object = false;
                                    break;
                                }
                            }
                            if(!planet_clicked) {
                                focus_object = NULL;
                            }
                        }
                    }
                }
            }
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::B)) {
            active_view_setting = ORBIT;
            center_focus_object = false;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::N)) {
            active_view_setting = RADIUS;
            sorted = false;
            center_focus_object = false;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::M)) {
            active_view_setting = MASS;
            sorted = false;
            center_focus_object = false;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::V)) {
            active_view_setting = DISTANCE;
            sorted = false;
            center_focus_object = false;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
            std::cout << "New G-value: ";
            std::cin >> G;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
            if(active_view_setting != SURFACE) {
                active_view_setting = SURFACE;
                focus_object = NULL;
            }
        }

        //rendering
        window.clear();

        for(Planemo* s : ss1.stars) {
            sf::Vector2u tex_size = imgvector[s->mk_class].getSize();
            sprite.setTextureRect(sf::IntRect(0, 0, tex_size.x, tex_size.y));
            sprite.setTexture(imgvector[s->mk_class]);
            sprite.setScale(scale * 12500 * 5, scale * 12500 * 5);
            sprite.setColor(sf::Color(255, 255, 255, std::clamp((1 / (scale * 5000) - 0.05), (double)0, (double)1) * 255));
            sprite.setPosition((sf::Vector2f)((-camera_pos + s->position) * scale + ((sf::Vector2<double>)window.getSize() - (sf::Vector2<double>)tex_size * (scale * 12500 * 5)) * 0.5));
            window.draw(sprite);
        }

        switch(active_view_scale) {
            case STAR:
                sf::Vector2<double> focus_add;
                sf::Vector2<double> coordinate;    
                switch(active_view_setting) {
                    case ORBIT:
                        for(Planemo* p : ss1.spaceobjects) if(p->parent != NULL) p->update_position();//p->grav.gravinteract();
                        //for(Planemo* p : ss1.spaceobjects) if(p->parent != NULL) p->grav.apply_acceleration();

                        for(Planemo* p : ss1.spaceobjects) p->view_position = p->position;
                        if(focus_object != NULL) camera_pos = focus_object->view_position;

                        /*if(!center_focus_object) {
                            if(focus_object != NULL) 
                            center_focus_object = true;
                        } else {
                            if(focus_object != NULL) camera_pos += focus_object->grav.get_velocity();
                        }*/
                        
                        for(Planemo* p : ss1.spaceobjects) p->render(camera_pos, scale);

                        break;

                    case RADIUS:
                        if(!sorted) {
                            std::sort(ss1.spaceobjects.begin(), ss1.spaceobjects.end(), compare_planet_radius);
                            sorted = true;
                        }

                        for(Planemo* p : ss1.spaceobjects) if(p->parent != NULL) p->update_position();

                        coordinate = sf::Vector2<double>(-ss1.spaceobjects[0]->radius, 0);
                        for(Planemo* p : ss1.spaceobjects) {
                            coordinate.x += p->radius;
                            p->view_position = coordinate;
                            coordinate.x += p->radius;
                        }

                        if(!center_focus_object) {
                            if(focus_object != NULL) camera_pos = focus_object->view_position;
                            center_focus_object = true;
                        }

                        for(Planemo* p : ss1.spaceobjects) p->render(camera_pos, scale);
                        break;
                    
                    case MASS:
                        if(!sorted) {
                            std::sort(ss1.spaceobjects.begin(), ss1.spaceobjects.end(), compare_planet_mass);
                            sorted = true;
                        }

                        for(Planemo* p : ss1.spaceobjects) if(p->parent != NULL) p->update_position();

                        coordinate = sf::Vector2<double>(-ss1.spaceobjects[0]->radius, 0);
                        for(Planemo* p : ss1.spaceobjects) {
                            coordinate.x += p->radius;
                            p->view_position = coordinate;
                            coordinate.x += p->radius;
                        }

                        if(!center_focus_object) {
                            if(focus_object != NULL) camera_pos = focus_object->view_position;
                            center_focus_object = true;
                        }

                        for(Planemo* p : ss1.spaceobjects) p->render(camera_pos, scale);
                        break;

                    case DISTANCE:
                        if(!sorted) {
                            std::sort(ss1.spaceobjects.begin(), ss1.spaceobjects.end(), compare_planet_distance);
                            sorted = true;
                        }

                        for(Planemo* p : ss1.spaceobjects) if(p->parent != NULL) p->update_position();

                        coordinate = sf::Vector2<double>(-ss1.spaceobjects[0]->radius, 0);
                        for(Planemo* p : ss1.spaceobjects) {
                            coordinate.x += p->radius;
                            p->view_position = coordinate;
                            coordinate.x += p->radius;
                        }

                        if(!center_focus_object) {
                            if(focus_object != NULL) camera_pos = focus_object->view_position;
                            center_focus_object = true;
                        }

                        for(Planemo* p : ss1.spaceobjects) p->render(camera_pos, scale);
                        break;
                    
                    case SURFACE:
                        map_sprite.setScale(scale * 16, scale * 16);
                        map_sprite.setPosition(-camera_pos.x * scale + window_width / 2, -camera_pos.y * scale + window_height / 2);

                        map_2sprite.setScale(scale * 16, scale * 16);
                        map_2sprite.setPosition((-camera_pos.x + 39200) * scale + window_width / 2, -(camera_pos.y) * scale + window_height / 2);

                        window.clear(sf::Color(50, 50, 50, 255));
                        window.draw(map_sprite);
                        window.draw(map_2sprite);

                        int scale2 = std::max(pow(10, floor(log10(100 / (scale * 2)))), 1.0);   
                        std::vector<sf::Vertex*> vertex_vec;
                        for(int i = -ceil(((double(window_width) / 2) / scale - camera_pos.x) / scale2) * scale2; i <= floor((double(window_width) / 2) / scale + camera_pos.x); i += scale2) {
                            //if(int(pow(10, floor(log10(100 / (scale * 3))))) == 0) break;
                            sf::Vertex vv[] {
                                sf::Vertex(sf::Vector2f((i - camera_pos.x) * scale + window_width / 2, 0), (i == 0 || i == 220) ? sf::Color(0xFF000020) : sf::Color(0xBFBFBF20)),
                                sf::Vertex(sf::Vector2f((i - camera_pos.x) * scale + window_width / 2, window_height - 1), (i == 0 || i == 220) ? sf::Color(0xFF000020) : sf::Color(0xBFBFBF20))
                            };
                            window.draw(vv, 2, sf::Lines);
                        }
                        for(int i = -ceil(((double(window_height) / 2) / scale - camera_pos.y) / scale2) * scale2; i <= floor((double(window_height) / 2) / scale + camera_pos.y); i += scale2) {//pow(10, floor(log10(1000 / scale)))
                            //if(int(pow(10, floor(log10(100 / (scale * 3))))) == 0) break;
                            sf::Vertex vv[] {
                                sf::Vertex(sf::Vector2f(0, (i - camera_pos.y) * scale + window_height / 2), (i == 0 || i == 220) ? sf::Color(0xFF000020) : sf::Color(0xBFBFBF20)),
                                sf::Vertex(sf::Vector2f(window_width - 1, (i - camera_pos.y) * scale + window_height / 2), (i == 0 || i == 220) ? sf::Color(0xFF000020) : sf::Color(0xBFBFBF20))
                            };
                            window.draw(vv, 2, sf::Lines);
                        }
                        /*for(int i = 0; i < vertex_vec.size(); i++) {
                            window.draw(vertex_vec[i], 2, sf::Lines);
                        }*/
                }
        }

        if(focus_object != NULL) {
            std::vector<std::string> active_text = selected_object_text;
            text.setCharacterSize(15);
            text.setFillColor(sf::Color(0xFFFFFFFF));
            active_text[0] += focus_object->name;
            //if(focus_object->parent != NULL) active_text[1] += focus_object->parent->name;

            active_text[2] += num_to_string(focus_object->mass) + " (";
            double mass_percentage = focus_object->mass / 20 * 100;
            if(mass_percentage > 100) {
                active_text[2] += num_to_string(focus_object->mass / 20) + "x";
            } else {
                active_text[2] += num_to_string(mass_percentage) + "%";
            }
            active_text[2] += " Terra mass)";

            active_text[3] += num_to_string(focus_object->radius) + " (";
            double radius_percentage = focus_object->radius / 19.57 * 100;
            if(radius_percentage > 100) {
                active_text[3] += num_to_string(focus_object->radius / 20) + "x";
            } else {
                active_text[3] += num_to_string(radius_percentage) + "%";
            }
            active_text[3] += " Terra radius)";

            active_text[4] += num_to_string(focus_object->density);
            active_text[5] += focus_object->type;

            if(focus_object->parent != NULL) {
                active_text[6] += num_to_string(focus_object->orbital_period) + " seconds";
                if(focus_object->orbital_period > 86400) {
                    active_text[6] += " (" + num_to_string(focus_object->orbital_period / 86400) + " days)";
                } else if(focus_object->orbital_period > 3600) {
                    active_text[6] += " (" + num_to_string(focus_object->orbital_period / 3600) + " hours)";
                } else if(focus_object->orbital_period > 60) {
                    active_text[6] += " (" + num_to_string(focus_object->orbital_period / 60) + " minutes)";
                } 
            }
            if(focus_object->rotation_time != 0) {
                active_text[7] += num_to_string(focus_object->rotation_time) + " seconds";
                if(focus_object->rotation_time > 86400) {
                    active_text[7] += " (" + num_to_string(focus_object->rotation_time / 86400) + " days)";
                } else if(focus_object->rotation_time > 3600) {
                    active_text[7] += " (" + num_to_string(focus_object->rotation_time / 3600) + " hours)";
                } else if(focus_object->rotation_time > 60) {
                    active_text[7] += " (" + num_to_string(focus_object->rotation_time / 60) + " minutes)";
                } 
            }
            

            for(int i = 0; i < active_text.size(); i++) {
                text.setString(active_text[i]);
                text.setPosition(12, 12 + 25 * i);
                window.draw(text);
            }
        } else {
            text.setCharacterSize(15);
            text.setString("scale: " + num_to_string(scale));
            text.setPosition(12, 12);
            window.draw(text);
        }

        window.display();
        frame++;
        if(frame % 1000 == 0 && frame != 0) {
            std::cout << (double)frame / (clock.getElapsedTime().asMilliseconds() - timeContainer) * 1000 << std::endl;
            timeContainer = clock.getElapsedTime().asMilliseconds();
            frame = 0;
        }
    }

    return 0;
}