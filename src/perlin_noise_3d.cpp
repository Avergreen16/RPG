#include <vector>
#include <array>
#include <iostream>
#define _USE_MATH_DEFINES

#include "global.cpp"

struct Prng {
    double value;
    double factor;

    double operator() () {
        double temp = value * factor;
        value = temp - floor(temp); 
        return value;
    }
};

struct Perlin_noise_factory {
    Prng rng;
    int octaves;
    double maximum_value;
    std::vector<std::array<double, 3>> vectors;

    void set_values(Prng rng, std::array<int, 3> size, int octaves, double persistance) {
        this->rng = rng;
        this->octaves = octaves;
        int octaves_bit_shift = 1 << (octaves - 1);
        vectors.clear();
        vectors.resize(size[0] * size[1] * size[2] * octaves_bit_shift * octaves_bit_shift * octaves_bit_shift);

        maximum_value = 0;
        for(int i = 0; i < octaves; i++) {
            maximum_value += pow(persistance, -i);
        }
    }

    void generate_vectors() {
        for(int i = 0; i < vectors.size(); i++) {
            double x = rng() * 2 - 1;
            double y = rng() * 2 - 1;
            double z = rng() * 2 - 1;
            double length = std::sqrt(x * x + y * y + z * z);
            vectors[i] = {x / length, y / length, z / length};
            std::cout << "Vectors generated.\n";
        }
    }

    double operator() (std::array<double, 3> pos, std::array<double, 3> size) {
        double value = 0;
        for(int i = 0; i < octaves; i++) {
            std::array<double, 8> dot_products;

            for(int i = 0; i < 8; i++) {
                
            }
        }
        return value;
    }
};