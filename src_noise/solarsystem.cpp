#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <iostream>

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

struct object {
    float mass, density, radius, rotation_speed, axial_tilt;
    object* parent;

    //planet
    double orbital_radius;
    float eccentricity;
    double hill_sphere;

    // star
    float luminosity;
    char _type;
    std::pair<double, double> hab_zone;
    std::pair<double, double> neighborhood;
    double frost_line;
    
    void planet(float mass_, float density_, float rotation_speed_, float axial_tilt_, double orbital_radius_, object* parent_) {
        parent = parent_;
        mass = mass_;
        density = density_;
        radius = cbrt((3 * mass) / (4 * density * M_PI)) * 20;
        rotation_speed = rotation_speed_;
        axial_tilt = axial_tilt_;
        orbital_radius = orbital_radius_;
        hill_sphere = orbital_radius * cbrt(mass / (3 * parent->mass));
    }

    void star(float mass_, float density_, float rotation_speed_, float axial_tilt_, char type_) {
        mass = mass_;
        density = density_;
        radius = cbrt((3 * mass) / (4 * density * M_PI)) * 20;
        rotation_speed = rotation_speed_;
        axial_tilt = axial_tilt_;
        _type = type_;
        luminosity = pow(mass / 200000, 4);
        hab_zone = std::pair<double, double>(pow(mass / 200000, 2) * 10000, pow(mass / 200000, 2) * 32000);
        frost_line = pow(mass / 200000, 2) * 50000;
        neighborhood = std::pair<double, double>(mass / 200000 * 2000, mass / 200000 * 200000);
    }
};

int main() {
    //RNG rng;
    //rng.set_seed(0.514786, 65.876438);
    object alpha;
    float m, d, r, a, o = 1;
    std::cout << "mass: ";
    std::cin >> m;
    std::cout << "density: ";
    std::cin >> d;

    alpha.star(m, d, 0, 0, 'G');

    printf("\nradius %f\nluminosity %f\nhabitable zone %f %f\nfrost line %f\nneighborhood %f %f\n", alpha.radius, alpha.luminosity, alpha.hab_zone.first, alpha.hab_zone.second, alpha.frost_line, alpha.neighborhood.first, alpha.neighborhood.second);

    object beta;

    while(true) {
        std::cout << "\norbital radius: ";
        std::cin >> o;
        if(o == 0) break;
        std::cout << "mass: ";
        std::cin >> m;
        std::cout << "density: ";
        std::cin >> d;

        beta.planet(m, d, 0, 0, o, &alpha);

        printf("\nradius %f\nhill sphere %f\n", beta.radius, beta.hill_sphere);
    }
}