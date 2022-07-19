#define FREEIMAGE_LIB
#include <fstream>
#include <iostream>
#include <map>
#include <cmath>
#include <FreeImage\Dist\x64\FreeImage.h>
#include <string>
#include <ctime>
#include <bits/stdc++.h>

#include "Noisefactory.hpp"

// 0.865234
double seed = 0.865234;

int main() {
    time_t starting_time = std::time(NULL);
    FreeImage_Initialise();
    
    //std::vector<int> numbervector(20);
    /*for(int x = 0; x < ImageWidth; x++) {
        for(int y = 0; y < ImageHeight; y++) {

            color = set_color(nfactory1.pullvalue(x, y) * 255);
            //color = set_color(clamp(value, 0, 2) / 3 * 255, clamp(value - 3, 0, 2) / 3 * 255, clamp(value - 6, 0, 3) / 3 * 255);
            FreeImage_SetPixelColor(img, x, y, &color);
            //numbervector[value] += 1;
        }
    }*/

    //for(int i = 0; i < numbervector.size(); i++) {
    //    std::cout << std::to_string((float)i / 20) << " - " << std::to_string((float)(i + 1) / 20) << ": " << (float)numbervector[i] / (ImageWidth * ImageHeight) * 100 << "%" << std::endl;
    //}
    RNG rng(0.876386428, 12.876248);

    //0.397484 -> 6, 3
    int iterations = 20;
    float sd_average;
    VoronoiTerrainMapGenerator vgen;
    for(int i = 0; i < iterations; i++) {
        float seed = rng.retrieve(6);
        vgen.initialise_terrain_map(seed, 1000, 1000, 45, 45, 4, 4, 3, 1.5, 5, 5, 2, 1, 5, 5, 2, 1);
        vgen.generate("file" + std::to_string(i) + "_2.png");
        std::cout << std::to_string(seed) << "\n\n"; 
    }

    /*PerlinNoiseFactory p_fac(0.782364, 36.867213, 6, 3, 512/3, 2, 2, true, true);
    FIBITMAP* img = FreeImage_Allocate(1024, 512, 24);
    RGBQUAD color;
    p_fac.generate_vectors();
    for(int x = 0; x < 1024; x++) {
        for(int y = 0; y < 512; y++) {
            color = set_color(p_fac.pullvalue(x, y) * 255);
            FreeImage_SetPixelColor(img, x, y, &color);
        }
    }
    FreeImage_Save(FIF_PNG, img, "noisetest2.png");*/

    /*std::ofstream file0;
    file0.open("perlin_noise_vector_log.txt");
    if (file0.is_open()) {
        for (std::pair<int, std::vector<float>> _pair : nfactory1.origin_vector_map) {
            file0 << std::to_string(std::get<1>(_pair)[0]) << " " << std::to_string(std::get<1>(_pair)[1]) << "\n";
        }
        std::cout << nfactory1.origin_vector_map.size() << std::endl;
    }
    file0.close();*/

    //VoronoiTerrainMapGenerator vgen;
    //vgen.initialise_terrain_map(0.542735, 1024, 512, 60, 30, 8, 4, 2, 3, 4, 2, 1, 1, 4, 2, 1, 1);
    //vgen.generate("file0.png");

    time_t ending_time = std::time(NULL);
    std::cout << ending_time - starting_time << std::endl;

    FreeImage_DeInitialise();

    return 0;
}