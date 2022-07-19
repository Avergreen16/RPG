#define FREEIMAGE_LIB
#define _USE_MATH_DEFINES
#include <iostream>
#include <map>
#include <cmath>
#include <FreeImage\Dist\x64\FreeImage.h>
#include <string>
#include <ctime>
#include <vector>

class RNG {
    public:
        float seed;
        float factor;
        float rng_number;

        RNG() {}
        RNG(float p_seed, float p_factor);

        float retrieve();
};

RGBQUAD set_color(float v);

class PerlinNoiseFactory {
    private:
        float seed;
        int width;
        int height;
        int scale;
        int octaves;
        float persistance;
        bool loop_x;
        bool loop_y;
        float maxvalue;
        float offset;
        float multiplier;
        RNG rng;
    
    public:
        std::map<int, std::vector<float>> origin_vector_map;

        PerlinNoiseFactory() {}

        PerlinNoiseFactory(float p_seed, int p_width, int p_height, int p_scale, int p_octaves, int p_persistance, bool p_loop_x, bool p_loop_y, float p_multiplier=1, float p_offset=0);

        float interpolate(float x1, float x2, int coordinate, int scale);

        float pullvalue(int x, int y);

        float dotproduct(std::vector<float> v1, std::vector<float> v2);
};

int main() {
    time_t starting_time = std::time(NULL);
    const int ImageWidth = 512;
    const int ImageHeight = 512;

    FreeImage_Initialise();
    FIBITMAP* img = FreeImage_Allocate(ImageWidth, ImageHeight, 24);
    RGBQUAD color;

    PerlinNoiseFactory nfactory1(0.456874, 8, 8, 64, 3, 3, true, true);

    std::vector<float> values;
    for(int x = 0; x < ImageWidth; x++) {
        for(int y = 0; y < ImageHeight; y++) {
            float value1 = nfactory1.pullvalue(x, y);
            color = set_color(value1 * 255);

            FreeImage_SetPixelColor(img, x, y, &color);
        }
    }
    
    time_t ending_time = std::time(NULL);

    char filename[] = "noise2.png";
    if (FreeImage_Save(FIF_PNG, img, filename)) {
        std::cout << "Image saved as " << filename << std::endl;
        std::cout << ending_time - starting_time << std::endl;
    } else {
        std::cout << "File was not successfully saved." << std::endl;
    }

    FreeImage_DeInitialise();

    return 0;
}

PerlinNoiseFactory::PerlinNoiseFactory(float p_seed, int p_width, int p_height, int p_scale, int p_octaves, int p_persistance, bool p_loop_x, bool p_loop_y, float p_multiplier, float p_offset) {
    seed = p_seed;
    width = p_width;
    height = p_height;
    scale = p_scale;
    octaves = p_octaves;
    persistance = p_persistance;
    loop_x = p_loop_x;
    loop_y = p_loop_y;
    offset = p_offset;
    multiplier = p_multiplier;
    rng = RNG(p_seed, 313.871);

    maxvalue = 0;
    for(int i = 0; i < octaves; i++) {
        maxvalue += (1 * multiplier - offset) * pow(persistance, -i);
    }
}

float PerlinNoiseFactory::pullvalue(int x, int y) {
    float returnvalue = 0;
    for(int i = 0; i < octaves; i++) {
        // stores the point's 4 dot products for interpolation
        float dotproducts[4];

        // grabs the coordinate value of the top left vector of the 4 surrounding vectors in the "vector array", each vector gets an
        // integer coordinate. These values are used to determine the vector's key
        int topleft_x = floor(x / (scale / pow(2, i)));
        int topleft_y = floor(y / (scale / pow(2, i)));
        
        for(int j = 0; j < 4; j++) {
            // vector_x and vector_y are the x and y integer coordinates of the active vector, determined by j, in the "vector array"
            int vector_x = topleft_x + j % 2;
            int vector_y = topleft_y + floor(j / 2);

            // these are the true coordinates of the active vector so that the distance vector between the 
            // active vector's origin and the point's x and y can be calculated
            int pos_value_x = vector_x * (scale / pow(2, i));
            int pos_value_y = vector_y * (scale / pow(2, i));

            // these loops move vector_x and vector_y into the width and height range so the pattern can loop.
            // it only runs if the loop variables are true
            // it's separate for x and y so that noise that loops in only one axis is possible
            if(loop_x) {
                while(vector_x >= width * pow(2, i)) {
                    vector_x -= width * pow(2, i);
                }
                while(vector_x < 0) {
                    vector_x += width * pow(2, i);
                }
            }
            if(loop_y) {
                while(vector_y >= height * pow(2, i)) {
                    vector_y -= height * pow(2, i);
                }
                while(vector_y < 0) {
                    vector_y += height * pow(2, i);
                }
            }

            // creates vector key for lookup in the vector map
            int vector_key = vector_x + vector_y * height;

            // will create a vector if there isn't already one with the same key, and will store it in the 
            // vector map with that key. The key corresponds the the x and y position of the vector in the "vector array"
            if(origin_vector_map.find(vector_key) == origin_vector_map.end()) {
                float theta = rng.retrieve() * 2 * M_PI;
                std::vector<float> insertvector = {std::cos(theta), std::sin(theta)};
                origin_vector_map.insert({vector_key, insertvector});
            }

            // vector of active point, current active point is one of the 4 closest vectors, this variable is assigned each of the
            // 4 vectors as the j loop runs.
            std::vector<float> active_vector = origin_vector_map[vector_key];
            // distance vector between true position of active vector's origin and x and y coordinate
            std::vector<float> coordinate_distance_vector = {((float)x - pos_value_x) / (float)(scale / pow(2, i)), ((float)y - pos_value_y) / (float)(scale / pow(2, i))};
            //coordinate_distance_vector = normalize(coordinate_distance_vector);

            dotproducts[j] = dotproduct(active_vector, coordinate_distance_vector);
        }

        // interpolates the dot products bilineraly and returns the result
        float x1 = interpolate(dotproducts[0], dotproducts[1], x, scale / pow(2, i));
        float x2 = interpolate(dotproducts[2], dotproducts[3], x, scale / pow(2, i));
        float sqrt_half = std::sqrt(0.5);
        returnvalue += (interpolate(x1, x2 , y, scale / pow(2, i)) + sqrt_half) / (sqrt_half * 2) * pow(persistance, -i);
    }
    return returnvalue / maxvalue;
}

float PerlinNoiseFactory::interpolate(float x1, float x2, int coordinate, int scale) {
    float scaled_coordinate = (float)(coordinate % scale) / scale;
    float returnvalue = (x2 - x1) * (6 * pow(scaled_coordinate, 5) - 15 * pow(scaled_coordinate, 4) + 10 * pow(scaled_coordinate, 3)) + x1;
    return returnvalue;
}

float PerlinNoiseFactory::dotproduct(std::vector<float> v1, std::vector<float> v2) {
    return v1[0] * v2[0] + v1[1] * v2[1];
}

RNG::RNG(float p_seed, float p_factor) {
    seed = p_seed;
    factor = p_factor;
    rng_number = p_seed;
}

float RNG::retrieve() {
    rng_number = rng_number * factor + 0.87632486;
    rng_number -= floor(rng_number);
    return rng_number;
}

RGBQUAD set_color(float v) {
    RGBQUAD color;
    color.rgbRed = v;
    color.rgbGreen = v;
    color.rgbBlue = v;
    return color;
}