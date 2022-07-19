#define _USE_MATH_DEFINES
#include <SFML-2.5.1\include\SFML\Graphics.hpp>
#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>
#include <array>

#include "worldgen.hpp"

const std::string vertex_shader = R"""(#version 330 core
layout(location = 0) in vec4 vertices;

void main() {
    gl_Position.xy = vertices.xy;
    gl_Position.z = 0.0;
    gl_Position.w = 0.0;
})""";

const std::string fragment_shader = R"""(#version 330 core

void main() {

    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})""";

const sf::Vector2u chunk_size(16, 16);

const double log10_4 = log10(4);

bool check_point_inclusion(sf::IntRect intrect, sf::Vector2<double> point) {
    if(point.x >= intrect.left && point.x < intrect.left + intrect.width && point.y >= intrect.top && point.y < intrect.top + intrect.height) return true;
    return false;
}

bool check_within_generated_chunk(std::vector<std::pair<std::array<int, 256>, sf::IntRect>> chunk_vector, sf::Vector2<double> point) {
    for(std::pair<std::array<int, 256>, sf::IntRect> p : chunk_vector) {
        if(check_point_inclusion(p.second, point)) {
            return true;
        }
    }
    return false;
}

template<typename type>
bool rect_collision(sf::Rect<type> rect1, sf::Rect<type> rect2) {
    if(rect1.left < rect2.left + rect2.width && rect1.left + rect1.width > rect2.left && rect1.top < rect2.top + rect2.height && rect1.top + rect1.height > rect2.top) return true;
    return false;
}

struct Int_counter {
    int pos = 0;
    int lim;

    Int_counter() {};
    Int_counter(int p_lim) : lim(p_lim) {};

    bool increment() {
        pos++;
        if(pos >= lim) {
            pos = 0;
            return true;
        }
        return false;
    }

    void reset() {
        pos = 0;
    }
};

struct Double_counter {
    double pos = 0;
    double lim;

    Double_counter() {};
    Double_counter(double p_lim) : lim(p_lim) {};

    bool increment(double value) {
        pos += value;
        if(pos >= lim) {
            pos = fmod(pos, lim);
            return true;
        }
        return false;
    }

    void reset() {
        pos = 0;
    }
};

enum state{IDLE, RUNNING};
enum direction{NORTH, WEST, SOUTH, EAST};

struct Entity {
    sf::Texture spritesheet;
    sf::Sprite sprite;

    sf::Vector2<double> visual_size = sf::Vector2<double>(2, 2);
    sf::Vector2<double> position = sf::Vector2<double>(13155 + 0.5, 8104 + 0.5);
    sf::Vector2u sprite_size = sf::Vector2u(32, 32);
    sf::Vector2u active_sprite = sf::Vector2u(0, 1);
    sf::Vector2<double> sprite_offset = sf::Vector2<double>(-1, -1.875);

    Double_counter cycle_timer = Double_counter(150);
    Int_counter sprite_counter = Int_counter(4);

    int state = IDLE;
    int direction = SOUTH;

    Entity(std::string p_spritesheet_filepath) {
        spritesheet.loadFromFile(p_spritesheet_filepath);
        sprite.setTexture(spritesheet);
    }
    
    void render(sf::Vector2<double> camera_pos, double scale, sf::RenderWindow* window) {
        sprite.setTextureRect(sf::IntRect(sprite_size.x * active_sprite.x, sprite_size.y * active_sprite.y, sprite_size.x, sprite_size.y));
        sprite.setScale(scale * visual_size.x / sprite_size.x, scale * visual_size.y / sprite_size.y);
        sprite.setPosition(sf::Vector2f((position + sprite_offset - camera_pos) * scale) + sf::Vector2f(window->getSize()) / 2.0f);
        window->draw(sprite);
    }

    void tick(double factor, double delta) {
        if(state == RUNNING) {
            if(cycle_timer.increment(delta)) {
                sprite_counter.increment();
            }
            active_sprite.x = sprite_counter.pos;
            active_sprite.y = direction;

            if(direction == NORTH) position.y -= 0.006 * delta * factor;
            else if(direction == WEST) position.x -= 0.006 * delta * factor;
            else if(direction == SOUTH) position.y += 0.006 * delta * factor;
            else if(direction == EAST) position.x += 0.006 * delta * factor;
        } else if(state == IDLE) {
            sprite_counter.reset();
            cycle_timer.reset();
            active_sprite.x = 3;
        }
    }
};

std::array<int, 256> generate_chunk(Worldgen* worldgen, sf::Vector2u top_left, bool is_overlay) {
    std::array<int, 256> chunk;
    for(int x = 0; x < chunk_size.x; x++) {
        for(int y = 0; y < chunk_size.y; y++) {
            //return_chunk[x + y * chunk_size.x] = pbg.get_biome(top_left.x + x, top_left.y + y, 39200, 19600);
            if(is_overlay) chunk[x + y * chunk_size.x] = get_floor_tile_objects(worldgen->retrieve({(int)top_left.x + x, (int)top_left.y + y}, {39200, 19600}), worldgen);
            else chunk[x + y * chunk_size.x] = get_floor_tile(worldgen->retrieve({(int)top_left.x + x, (int)top_left.y + y}, {39200, 19600}), worldgen);
        }
    }
    return chunk;
}

int main() {
    sf::Texture map_texture;
    map_texture.loadFromFile("map203x.png");
    sf::Sprite map_sprite(map_texture);
    std::array<int, 256> active_chunk;
    std::array<int, 256> active_chunk_overlay;
    sf::Vector2u active_chunk_top_left;
    sf::IntRect chunk_boundary(0, 0, chunk_size.x, chunk_size.y);

    std::vector<std::pair<std::array<int, 256>, sf::IntRect>> chunk_vector;
    std::vector<std::pair<std::array<int, 256>, sf::IntRect>> chunk_overlay_vector;

    sf::VertexArray varray1;
    varray1.append(sf::Vertex(sf::Vector2f(0.2 * 100, 0.2 * 100), sf::Vector2f(0, 0)));
    varray1.append(sf::Vertex(sf::Vector2f(0.5 * 100, 0.2 * 100), sf::Vector2f(16, 0)));
    varray1.append(sf::Vertex(sf::Vector2f(0.5 * 100, 0.5 * 100), sf::Vector2f(16, 16)));
    varray1.append(sf::Vertex(sf::Vector2f(0.2 * 100, 0.2 * 100), sf::Vector2f(0, 0)));
    varray1.append(sf::Vertex(sf::Vector2f(0.2 * 100, 0.5 * 100), sf::Vector2f(0, 16)));
    varray1.append(sf::Vertex(sf::Vector2f(0.5 * 100, 0.5 * 100), sf::Vector2f(16, 16)));

    sf::Shader shader;
    shader.loadFromMemory(fragment_shader, sf::Shader::Fragment);
    sf::Shader::bind(&shader);

    
    if (!shader.isAvailable()) {
        std::cout << "The shader is not available\n";
    }
    //setup
    /*int frame = 0;
    int timeContainer = 0;
    sf::Clock clock;*/

    sf::Font font;
    font.loadFromFile("Q5text.ttf");
    sf::Text text;
    text.setFont(font);

    double scale = 48;
    sf::Vector2<double> camera_pos(0.0f, 0.0f);
    sf::Vector2f mouse_pos(0.0f, 0.0f);

    sf::Vector2u window_size(1000, 600);
    sf::RenderWindow window(sf::VideoMode(window_size.x, window_size.y), "Test Name", sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize);
    window.setFramerateLimit(100);

    Entity player("res\\entities\\Player\\player_spritesheet.png");
    
    //pbg.set_parameters(0.7707326, 6, 3, 7, 1.5, 4, 4, 3, 1.5, 4, 4, 4, 1.3);

    Worldgen worldgen(0.7707326, 6, 3, 7, 1.5, 4, 4, 3, 1.5, 4, 4, 4, 1.3);

    active_chunk = generate_chunk(&worldgen, sf::Vector2u(chunk_boundary.left, chunk_boundary.top), false);
    active_chunk_overlay = generate_chunk(&worldgen, sf::Vector2u(chunk_boundary.left, chunk_boundary.top), true);

    sf::Sprite tile_sprite;
    sf::Texture floor_tileset;
    floor_tileset.loadFromFile("res\\tiles\\floor_tileset.png");
    tile_sprite.setTexture(floor_tileset);

    double player_speed_factor = 1;
    int delta;

    window.setKeyRepeatEnabled(false);

    sf::Clock clock;
    sf::Uint64 prev_time = clock.getElapsedTime().asMicroseconds();
    
    //loop
    while(window.isOpen()) {
        //input
        sf::Event event;
        while(window.pollEvent(event)) {
            switch(event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::MouseMoved:
                    /*if(sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                        camera_pos -= ((sf::Vector2<double>)sf::Mouse::getPosition(window) - (sf::Vector2<double>)mouse_pos)/scale;
                    }*/
                    mouse_pos = (sf::Vector2f)sf::Mouse::getPosition(window);
                    break;
                case sf::Event::MouseWheelScrolled:
                    delta = event.mouseWheelScroll.delta;
                    scale *= pow(1.2, delta);

                    break;
                case sf::Event::KeyPressed:
                    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) player_speed_factor *= 1.5;
                    else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) player_speed_factor /= 1.5;
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));

                    break;
            }
        }

        window_size = window.getSize();

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            player.direction = NORTH;
            player.state = RUNNING;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            player.direction = WEST;
            player.state = RUNNING;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            player.direction = SOUTH;
            player.state = RUNNING;
        } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            player.direction = EAST;
            player.state = RUNNING;
        } else {
            player.state = IDLE;
        }

        if(sf::Keyboard::isKeyPressed)

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
            chunk_vector.clear();
            chunk_overlay_vector.clear();
            chunk_vector.push_back(std::pair(generate_chunk(&worldgen, sf::Vector2u(chunk_boundary.left, chunk_boundary.top), false), chunk_boundary));
            chunk_overlay_vector.push_back(std::pair(generate_chunk(&worldgen, sf::Vector2u(chunk_boundary.left, chunk_boundary.top), true), chunk_boundary));
        }

        //rendering
        window.clear(sf::Color(50, 50, 50, 255));

        //if(!check_point_inclusion(chunk_boundary, player.position)) {
        if(!check_within_generated_chunk(chunk_vector, player.position)) {
            sf::Vector2u top_left(int(player.position.x / chunk_size.x) * chunk_size.x, int(player.position.y / chunk_size.y) * chunk_size.y);
            chunk_boundary.left = top_left.x;
            chunk_boundary.top = top_left.y;
            chunk_vector.push_back(std::pair(generate_chunk(&worldgen, sf::Vector2u(chunk_boundary.left, chunk_boundary.top), false), chunk_boundary));
            chunk_overlay_vector.push_back(std::pair(generate_chunk(&worldgen, sf::Vector2u(chunk_boundary.left, chunk_boundary.top), true), chunk_boundary));
        }
                        
        map_sprite.setScale(scale * 16, scale * 16);
        map_sprite.setPosition(-camera_pos.x * scale + window_size.x / 2, -camera_pos.y * scale + window_size.y / 2);

        window.draw(map_sprite);

        for(std::pair<std::array<int, 256>, sf::IntRect> p : chunk_vector) {
            if(rect_collision(sf::Rect<double>(p.second), sf::Rect<double>(camera_pos - sf::Vector2<double>(window_size) / (scale * 2), sf::Vector2<double>(window_size) / (scale)))) {
                for(int x = 0; x < chunk_size.x; x++) {
                    for(int y = 0; y < chunk_size.y; y++) {
                        double uncentered_x = (x + p.second.left - camera_pos.x) * scale;
                        double uncentered_y = (y + p.second.top - camera_pos.y) * scale;
                        sf::Vector2<double> position(uncentered_x + double(window_size.x) / double(2), uncentered_y + double(window_size.y) / double(2));
                        tile_sprite.setPosition(position.x, position.y);
                        int tile_ID = p.first[x + y * 16];
                        tile_sprite.setTextureRect(sf::IntRect((tile_ID % 8) * 16, int(tile_ID / 8) * 16, 16, 16));
                        tile_sprite.setScale(scale / 16, scale / 16);
                        window.draw(tile_sprite);
                    }
                }
            }
        }

        for(std::pair<std::array<int, 256>, sf::IntRect> p : chunk_overlay_vector) {
            if(rect_collision(sf::Rect<double>(p.second), sf::Rect<double>(camera_pos - sf::Vector2<double>(window_size) / (scale * 2), sf::Vector2<double>(window_size) / (scale)))) {
                for(int x = 0; x < chunk_size.x; x++) {
                    for(int y = 0; y < chunk_size.y; y++) {
                        double uncentered_x = (x + p.second.left - camera_pos.x) * scale;
                        double uncentered_y = (y + p.second.top - camera_pos.y) * scale;
                        sf::Vector2<double> position(uncentered_x + double(window_size.x) / 2, uncentered_y + double(window_size.y) / 2);
                        tile_sprite.setPosition(position.x, position.y);
                        int tile_ID = p.first[x + y * 16];
                        tile_sprite.setTextureRect(sf::IntRect((tile_ID % 8) * 16, int(tile_ID / 8) * 16, 16, 16));
                        tile_sprite.setScale(scale / 16, scale / 16);
                        window.draw(tile_sprite);
                    }
                }
            }
        }

        /*for(int x = 0; x < chunk_size.x; x++) {
            for(int y = 0; y < chunk_size.y; y++) {
                sf::Vector2<double> position((x + chunk_boundary.left - camera_pos.x) * scale + double(window_size.x) / double(2), (y + chunk_boundary.top - camera_pos.y) * scale + double(window_size.y) / double(2));
                tile_sprite.setPosition(position.x, position.y);
                tile_sprite.setColor(active_chunk[x + y * chunk_size.x]);
                tile_sprite.setScale(scale / 16, scale / 16);
                window.draw(tile_sprite);
            }
        }*/
       

        /*int scale2 = std::max(pow(4, floor(log10(128.0 / scale) / log10_4)), 1.0);   
        std::vector<sf::Vertex*> vertex_vec;
        for(int i = -ceil(((double(window_size.x) / 2) / scale - camera_pos.x) / scale2) * scale2; i <= floor((double(window_size.x) / 2) / scale + camera_pos.x); i += scale2) {
            sf::Vertex vv[] {
                sf::Vertex(sf::Vector2f((i - camera_pos.x) * scale + window_size.x / 2, 0), (i == 0 || i == 19600) ? sf::Color(0xFF0000FF) : sf::Color(0xBFBFBF20)),
                sf::Vertex(sf::Vector2f((i - camera_pos.x) * scale + window_size.x / 2, window_size.y - 1), (i == 0 || i == 19600) ? sf::Color(0xFF0000FF) : sf::Color(0xBFBFBF20))
            };
            window.draw(vv, 2, sf::Lines);
        }
        for(int i = -ceil(((double(window_size.y) / 2) / scale - camera_pos.y) / scale2) * scale2; i <= floor((double(window_size.y) / 2) / scale + camera_pos.y); i += scale2) {
            sf::Vertex vv[] {
                sf::Vertex(sf::Vector2f(0, (i - camera_pos.y) * scale + window_size.y / 2), (i == 0 || i == 39200) ? sf::Color(0xFF0000FF) : sf::Color(0xBFBFBF20)),
                sf::Vertex(sf::Vector2f(window_size.x - 1, (i - camera_pos.y) * scale + window_size.y / 2), (i == 0 || i == 39200) ? sf::Color(0xFF0000FF) : sf::Color(0xBFBFBF20))
            };
            window.draw(vv, 2, sf::Lines);
        }*/

        sf::Uint64 new_time = clock.getElapsedTime().asMicroseconds();
        player.tick(player_speed_factor, (new_time - prev_time) / 1000);
        prev_time = new_time;

        camera_pos = player.position + sf::Vector2<double>(0, -0.875);

        player.render(camera_pos, scale, &window);

        /*sf::Vertex vv[] {
            sf::Vertex(sf::Vector2f(window_size) * 0.25f, sf::Color(0x00FF003F)),
            sf::Vertex(sf::Vector2f(float(window_size.x) * 0.75f, float(window_size.y) * 0.25f), sf::Color(0x00FF003F)),
            sf::Vertex(sf::Vector2f(window_size) * 0.75f, sf::Color(0x00FF003F)),
            sf::Vertex(sf::Vector2f(float(window_size.x) * 0.25f, float(window_size.y) * 0.75f), sf::Color(0x00FF003F))
        };
        window.draw(vv, 4, sf::PrimitiveType::Quads);*/

        window.draw(varray1);

        text.setCharacterSize(15);
        text.setString("Coordinates: " + std::to_string(int(player.position.x)) + " " + std::to_string(int(player.position.y)));
        text.setPosition(12, 12);
        window.draw(text);

        window.display();
    }

    return 0;
}