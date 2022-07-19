#pragma once
#define _USE_MATH_DEFINES
#include <map>
#include <vector>
//#include <FreeImage\Dist\x64\FreeImage.h>
#include <iostream>

double clamp(double input, double lower, double upper);

struct Point {
    double x;
    double y;
    Point () {};
    Point(double p_x, double p_y) : x(p_x), y(p_y) {};
};

class RNG {
    public:
        double seed;
        double factor;
        double rng_number;

        RNG() {}
        RNG(double p_seed, double p_factor);

        double retrieve(int decimal_digits=0);
};

//RGBQUAD set_color(double r, double g, double b);

//RGBQUAD set_color(double r, double g, double b, double a);

//RGBQUAD set_color(double v);

class ValueNoiseFactory {
    private:
        double seed;
        int width;
        int height;
        double scale;
        int octaves;
        double persistance;
        bool loop_x;
        bool loop_y;
        double maxvalue;
        double offset;
        double multiplier;
        RNG rng;

    public:
        std::map<int, double> origin_value_map;

        ValueNoiseFactory() {}

        ValueNoiseFactory(double p_seed, int p_width, int p_height, double p_scale, int p_octaves, double p_persistance, bool p_loop_x, bool p_loop_y, double p_multiplier=1, double p_offset=0);

        double interpolate(double x1, double x2, int coordinate, int scale);

        double pullvalue(int x, int y);

        void generate_values();
};

class PerlinNoiseFactory {
    private:
        double seed;
        int width;
        int height;
        double scale;
        int octaves;
        double persistance;
        bool loop_x;
        bool loop_y;
        double maxvalue;
        double offset;
        double multiplier;
        RNG rng;
    
    public:
        std::map<int, std::vector<double>> origin_vector_map;

        PerlinNoiseFactory() {}

        PerlinNoiseFactory(double p_seed, double p_factor, int p_width, int p_height, double p_scale, int p_octaves, double p_persistance, bool p_loop_x, bool p_loop_y, double p_multiplier=1, double p_offset=0);

        double interpolate(double x1, double x2, int coordinate, double scale);

        double pullvalue(int x, int y);

        double dotproduct(std::vector<double> v1, std::vector<double> v2);

        void generate_vectors();

        void generate_vectors2();
};

class VoronoiPoint {
    public:
        double x;
        double y;
        std::string biome = "x";
        double value = 300;
        uint32_t color = 0x00000000;

        VoronoiPoint() {}

        VoronoiPoint(double p_x, double p_y);

        Point transfer() {
            return Point(x, y);
        }
};

class VoronoiFactory {
    public:
        double seed;
        int width;
        int height;
        int voronoi_map_width;
        int voronoi_map_height;
        bool loop_x;
        bool loop_y;
        RNG rng;

        std::map<int, VoronoiPoint> voronoi_map;

        VoronoiFactory() {};

        VoronoiFactory(double p_seed, int p_width, int p_height, int p_voronoi_map_width, int p_voronoi_map_height, bool p_loop_x, bool p_loop_y);

        int pull_closest_point_key(double x, double y);

        void generate_points();

        double pyth(int x, int y);
};

/*class VoronoiTerrainMapGenerator {
    public:
        int width;
        int height;
        int v_xsize;
        int v_ysize;
        double seed;
        RNG biome_rng;
        VoronoiFactory v_fac;
        PerlinNoiseFactory p_fac_e;
        PerlinNoiseFactory p_fac_h;
        PerlinNoiseFactory p_fac_t;

        void initialise_terrain_map(double p_seed, int p_width, int p_height, int p_v_xsize, int p_v_ysize, int p_e_width, int p_e_height, int p_e_octaves, double p_e_persistance, int p_h_width, int p_h_height, int p_h_octaves, double p_h_persistance, int p_t_width, int p_t_height, int p_t_octaves, double p_t_persistance) {
            width = p_width;
            height = p_height;
            v_xsize = p_v_xsize;
            v_ysize = p_v_ysize;
            seed = p_seed;
            biome_rng = RNG(p_seed, 20.827648);
            v_fac = VoronoiFactory(p_seed * 1.01, p_width, p_height, p_v_xsize, p_v_ysize, false, true);
            p_fac_e = PerlinNoiseFactory(p_seed, p_seed * 23.663879, p_e_width, p_e_height, (double)std::min(p_width, p_height) / std::min(p_e_width, p_e_height), p_e_octaves, p_e_persistance, false, true);
            p_fac_h = PerlinNoiseFactory(p_seed, p_seed * 48.679649, p_h_width, p_h_height, (double)std::min(p_width, p_height) / std::min(p_h_width, p_h_height), p_h_octaves, p_h_persistance, false, true);
            p_fac_t = PerlinNoiseFactory(p_seed, p_seed * 56.343759, p_t_width, p_t_height, (double)std::min(p_width, p_height) / std::min(p_t_width, p_t_height), p_t_octaves, p_t_persistance, false, true);
        }

        void generate(std::string p_filename) {
            char const* filename = p_filename.c_str();
            FIBITMAP* img = FreeImage_Allocate(width + 500, height + 500, 32);
            FreeImage_SetTransparent(img, true);

            double value1;
            double value2;
            double value3;
            RGBQUAD color;
            VoronoiPoint* vpoint;
            v_fac.generate_points();
            p_fac_e.generate_vectors();
            p_fac_h.generate_vectors();
            p_fac_t.generate_vectors();
            for(int y = 0; y < height; y++) {
                double py = ((double)y + 0.5) / height * 2 - 1;
                double width_at_height = sqrt(1 - py * py);
                double py2 = asin(py) * 2/M_PI;
                for(int x = 0; x < width; x++) {
                    double px = ((double)x + 0.5) / width * 2 - 1;
                    double magSq = py * py + px * px;
                    if(magSq > 1) {
                        continue;
                    }

                    px = asin(px / width_at_height) * 2/M_PI;
                    vpoint = &v_fac.voronoi_map.at(v_fac.pull_closest_point_key((px + 1) * width / 2 - 0.5, (py2 + 1) * width / 2 - 0.5));
                    if(vpoint->biome == "x") {
                        value1 = p_fac_e.pullvalue(vpoint->x, vpoint->y);
                        value2 = p_fac_h.pullvalue(vpoint->x, vpoint->y);
                        value3 = clamp(1 - std::abs((vpoint->y - height / 2) / height * 2 + (p_fac_t.pullvalue(vpoint->x, vpoint->y) * 2 - 1) * 0.1), 0, 1);
                        std::vector<double> new_values = {value1, value2, value3};
                        vpoint->change_biome(get_biome(new_values[0], new_values[1], new_values[2]));
                    }
                    
                    std::string biome = vpoint->biome;
                    color = biome_lookup_table.at(biome);
                    /*if (vpoint->value == 300) {
                        vpoint->value = p_fac_e.pullvalue(vpoint->x, vpoint->y) * 5;
                    }

                    double value = vpoint->value;
                    color = set_color(255 - clamp(value - 3, 0, 1) * 55, 255 - clamp(value - 3, 0, 1) * 55, 255 - clamp(value - 3, 0, 1) * 55, 255 * clamp(value - 2, 0, 1));*/

                    /*FreeImage_SetPixelColor(img, x + 250, y + 250, &color);
                }
            }

            if (FreeImage_Save(FIF_PNG, img, filename)) {
                std::cout << "Image saved as " << filename << std::endl;
            } else {
                std::cout << "File was not successfully saved." << std::endl;
            }
        }

        std::map<std::string, RGBQUAD> biome_lookup_table = {
            {"sea", set_color(30, 30, 180)},
            {"deep_sea", set_color(10, 10, 160)},
            {"reef", set_color(50, 50, 150)},
            {"frozen_sea", set_color(150, 180, 220)},
            {"grassland", set_color(120, 200, 120)},
            {"swamp", set_color(50, 145, 80)},
            {"forest", set_color(10, 180, 10)},
            {"rainforest", set_color(10, 250, 50)},
            {"boreal_forest", set_color(20, 100, 10)},
            {"shrubland", set_color(180, 200, 20)},
            {"desert", set_color(220, 200, 50)},
            {"ice_cap", set_color(240, 240, 250)},
            {"tundra", set_color(100, 100, 20)},
            {"river", set_color(60, 60, 200)},
            {"canyons", set_color(200, 140, 20)},
            {"mountains", set_color(150, 150, 150)},
            {"glacier", set_color(215, 215, 250)}
        };

        std::string get_biome(double elev, double humid, double temp) {
            if(elev > 0.82) {
                return "glacier";
            } else if(elev > 0.75) {
                if(temp < 0.35) {
                    return "glacier";
                } else {
                    return "mountains";
                }
            } else if(elev > 0.7) {
                if(temp < 0.35) {
                    return "ice_cap";
                } else {
                    if(humid < 0.45) {
                        return "grassland";
                    } else if(humid < 0.65) {
                        return "boreal_forest";
                    } else {
                        return "forest";
                    }
                }
            } else if(elev > 0.55) {
                if(temp < 0.35) {
                    if(humid < 0.4) {
                        return "tundra";
                    } else {
                        return "ice_cap";
                    }
                } else if(temp < 0.55) {
                    if(humid < 0.1) {
                        return "desert";
                    } else if(humid < 0.3) {
                        return "grassland";
                    } else {
                        return "boreal_forest";
                    }
                } else if(temp < 0.8) {
                    if(humid < 0.35) {
                        return "desert";
                    } else if(humid < 0.45) {
                        return "grassland";
                    } else if(humid < 0.65) {
                        return "forest";
                    } else {
                        return "swamp";
                    }
                } else {
                    if(humid < 0.3) {
                        return "canyons";
                    } else if(humid < 0.45) {
                        return "desert";
                    } else if(humid < 0.5) {
                        return "shrubland";
                    } else if(humid < 0.55) {
                        return "forest";
                    } else if(humid < 0.7) {
                        return "rainforest";
                    } else {
                        return "swamp";
                    }
                } 
            } else {
                if(temp < 0.35) {
                    return "frozen_sea";
                } else if(elev < 0.4) {
                    return "deep_sea";
                } else if (humid > 0.5 && temp > 0.7) {
                    return "reef";
                } else {
                    return "sea";
                }
            }
        }
};*/