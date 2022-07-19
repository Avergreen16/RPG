#include <SFML-2.5.1\include\SFML\Graphics.hpp>
#include <iostream>
#include <cmath>
#include <ctime>

const std::string fragment_shader = R"""(#version 330 core
#ifdef GL_ES
precision mediump float;
#endif
 
uniform vec2 u_resolution;
uniform vec2 u_shapeSize;
uniform vec2 u_shapePos;
uniform float u_time;
uniform sampler2D u_texture;
uniform float u_axialTilt;
uniform int u_secondsToRotate;

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
    if(px3 > 0.2) {//px2 > shadow1_xCoord && px2 < shadow2_xCoord) {
        gl_FragColor = vec4(color.xyz / 4, 1.0);
    } else {
        gl_FragColor = vec4(color.xyz, 1.0);
    }
})""";

bool collision(sf::RectangleShape a, sf::RectangleShape b) {
    sf::Vector2f as = a.getSize();
    sf::Vector2f ap = a.getPosition();
    sf::Vector2f bs = b.getSize();
    sf::Vector2f bp = b.getPosition();
    return (ap.x <= bp.x + bs.x && ap.x + as.x >= bp.x && ap.y <= bp.y + bs.y && ap.y + as.y >= bp.y);
}

struct Planet {
    sf::RenderWindow* window;
    sf::Shader* shader;
    sf::Clock* clock;

    float radius;
    sf::Vector2f position;
    float axial_tilt;
    int rotation_time;
    sf::Texture texture;

    sf::RectangleShape draw_rect;

    Planet(sf::RenderWindow* p_window, sf::Shader* p_shader, sf::Clock* p_clock, float p_radius, sf::Vector2f p_position, float p_axial_tilt, int p_rotation_time, std::string texture_filepath) : window(p_window), shader(p_shader), clock(p_clock), radius(p_radius), position(p_position ), axial_tilt(p_axial_tilt), rotation_time(p_rotation_time)  {
        texture.loadFromFile(texture_filepath);
    }

    void render(sf::Vector2f camera_pos, float scale) {
        draw_rect.setSize(sf::Vector2f(radius * scale, radius * scale));
        draw_rect.setPosition((-camera_pos) * scale + ((sf::Vector2f)window->getSize() - draw_rect.getSize()) / 2.0f + position * scale);
        if(collision(draw_rect, sf::RectangleShape((sf::Vector2f)window->getSize()))) {
            shader->setUniform("u_texture", texture);
            shader->setUniform("u_resolution", (sf::Glsl::Vec2)window->getSize());
            shader->setUniform("u_shapeSize", (sf::Glsl::Vec2)draw_rect.getSize());
            shader->setUniform("u_shapePos", (sf::Glsl::Vec2)draw_rect.getPosition());
            shader->setUniform("u_time", (float)clock->getElapsedTime().asMilliseconds());
            shader->setUniform("u_axialTilt", axial_tilt);
            shader->setUniform("u_secondsToRotate", rotation_time);

            window->draw(draw_rect, shader);
        }
    }
};

int main() 
{   
    int frame = 0;
    int timeContainer = 0;
    sf::Clock clock;

    unsigned int window_width = 1600, window_height = 800;
    sf::RenderWindow window(sf::VideoMode(window_width, window_height), "Test Name", sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize);
    //window.setFramerateLimit(100);

    sf::Shader shader;
    shader.loadFromMemory(fragment_shader, sf::Shader::Fragment);

    float scale = 1;
    sf::Vector2f camera_pos(0.0f, 0.0f);
    sf::Vector2f mouse_pos(0.0f, 0.0f);

    Planet p1(&window, &shader, &clock, 10.0f, sf::Vector2f(0.0f, 0.0f), 15, 60, "surface_terra.png");
    Planet p2(&window, &shader, &clock, 7.0f, sf::Vector2f(20.0f, 0.0f), -25, 35, "surface_ares.png");
    
    while(window.isOpen()) 
    {
        sf::Event event_;
        while(window.pollEvent(event_))
        {
            switch(event_.type) 
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    window_width = event_.size.width;
                    window_height = event_.size.height;

                    window.setSize(sf::Vector2u(window_width, window_height));
                    break;
                case sf::Event::MouseMoved:
                    if(sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                        camera_pos -= ((sf::Vector2f)sf::Mouse::getPosition(window) - mouse_pos)/scale;
                    }
                    mouse_pos = (sf::Vector2f)sf::Mouse::getPosition(window);
                    break;
                case sf::Event::MouseWheelScrolled:
                    int delta = event_.mouseWheelScroll.delta;
                    scale *= pow(1.2, delta);
                    break;
            }
        }

        window.clear();
        
        p1.render(camera_pos, scale);
        p2.render(camera_pos, scale);

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