#pragma once
#define _USE_MATH_DEFINES
#include <SFML-2.5.1\include\SFML\Graphics.hpp>
#include <cmath>
#include <vector>
#include <iomanip>

bool collision(sf::RectangleShape a, sf::RectangleShape b) {
    sf::Vector2f as = a.getSize();
    sf::Vector2f ap = a.getPosition();
    sf::Vector2f bs = b.getSize();
    sf::Vector2f bp = b.getPosition();
    return (ap.x <= bp.x + bs.x && ap.x + as.x >= bp.x && ap.y <= bp.y + bs.y && ap.y + as.y >= bp.y);
}

bool point_collision(sf::RectangleShape a, sf::Vector2f b) {
    sf::Vector2f as = a.getSize();
    sf::Vector2f ap = a.getPosition();
    return (b.x >= ap.x && b.x <= ap.x + as.x && b.y >= ap.y && b.y <= ap.y + as.y);
}

bool circle_point_collision(sf::Vector2f a, sf::Vector2f b, float radius) {
    sf::Vector2f difference = a - b;
    return hypot(difference.x, difference.y) <= radius;
}

struct RNG {
    float factor;
    float product;

    void set_seed(float p_seed, float p_factor) {
        factor = p_factor;
        product = p_seed;
    }

    float retrieve() {
        product = product * factor;
        product -= floor(product);
        return product;
    }
};

std::string num_to_string(double input) {
    std::string str;
    std::stringstream s_stream(str);
    if(input >= 1000) {
        str = std::to_string((int)round(input));
    } else {
        if(input != (int)input) s_stream.precision(3);
        s_stream << input;
        s_stream >> str;
    }
    return str;
}