#include <vector>
#include <iostream>
#include <cmath>
#include <string>

struct RNG { 
    double factor;
    double rng_number;

    RNG() {}
    RNG(double p_seed, double p_factor);

    double retrieve();
};

int main() {
    std::vector<double> accretion_vector;

    double i_seed;
    std::cout << "seed: ";
    std::cin >> i_seed;

    RNG prng(i_seed, i_seed * 32.414);
    double retrieve_mass_value = prng.retrieve() * 0.75 + 0.05;
    double accretion_mass = 200000 * (retrieve_mass_value - 0.05) + 500000;
    double disc_width = 80 * (accretion_mass / 150000);

    double falloff_point = disc_width * (0.4 + prng.retrieve() * 0.25);
    double start_disc_edge = disc_width * (0.75 + prng.retrieve() * 0.25);
    disc_width = std::round(disc_width * 10) / 10;
    double graph_height = accretion_mass / (falloff_point + start_disc_edge / 2) ;

    double mass_collector = 0;
    int i = 0;
    while(i < falloff_point * 10 + 1) {
        accretion_vector.push_back(graph_height);
        mass_collector += graph_height;
        i++;
        std::cout << "check0" << std::endl;
    }
    double slope = -graph_height / (start_disc_edge - falloff_point);
    while(i < start_disc_edge * 10 + 1) {
        float i_val = slope * (((double)i + 0.5) / 10 - falloff_point) + graph_height;
        accretion_vector.push_back(i_val);
        mass_collector += i_val;
        i++;
        std::cout << "check1" << std::endl;
    }
    while(i < disc_width * 10 + 1) {
        accretion_vector.push_back(0.0);
        i++;
        std::cout << "check2" << std::endl;
    }

    std::cout << std::to_string(accretion_mass) << " " << std::to_string(mass_collector) << std::endl;
    /*bool run = true;
    while(run) {

    }*/

    return 0;
}

RNG::RNG(double p_seed, double p_factor) {
    factor = p_factor;
    rng_number = p_seed;
}

double RNG::retrieve() {
    rng_number = rng_number * factor;
    rng_number -= std::floor(rng_number);
    return rng_number;
}