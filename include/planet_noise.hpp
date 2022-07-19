#include <map>
#include <vector>
#include <SDL2-w64\include\SDL2\SDL.h>

double clamp(double input, double min, double max);

double dotproduct(std::vector<double> v1, std::vector<double> v2);

double interpolate(double x1, double x2, int position);

double hypotenuse(double x, double y);

double arcsin_approx(double x);

struct RNG { 
    double factor;
    double rng_number;

    RNG() {}
    RNG(double p_seed, double p_factor);

    double retrieve();
};

struct PerlinNoiseFactory {
    double seed;
    int main_vector_width;
    int main_vector_height;
    int octaves;
    double persistance;
    bool loop_x;
    bool loop_y;
    double maxvalue;
    RNG rng;

    std::map<int, std::vector<double>> vector_map;

    PerlinNoiseFactory() {}

    PerlinNoiseFactory(double p_seed, double p_factor, int p_main_vector_width, int p_main_vector_height, int p_octaves, double p_persistance, bool p_loop_x, bool p_loop_y);

    void generate_vectors();

    double pullvalue(int x, int y, int width, int height);
};

struct v_Point {
    double coordinates[2];
    uint32_t color = 0x00000000;

    v_Point() {}

    v_Point(double p_x, double p_y);
};

struct VoronoiFactory {
    int point_map_width;
    int point_map_height;
    bool loop_x;
    bool loop_y;
    RNG rng;

    std::map<int, v_Point> point_map;

    VoronoiFactory() {}

    VoronoiFactory(double p_seed, double p_factor, int p_point_map_width, int p_point_map_height, bool p_loop_x, bool p_loop_y);

    void generate_points();

    int pull_closest_point_key(double x, double y, int width, int height);
};

struct PlanetTerrainGenerator {
    std::map<std::string, Uint32> biome_lookup_table;
    VoronoiFactory v_fac;
    PerlinNoiseFactory p_fac_elev;

    PlanetTerrainGenerator() {}

    PlanetTerrainGenerator(double p_seed, int p_vwidth, int p_vheight, int p_main_elevwidth, int p_main_elevheight, int p_elevoctaves, int p_elevpersistance);

    std::string get_biome(double elev);

    void assign_colors(SDL_Surface* surface);

    void draw_planet(SDL_Surface* surface);

    void draw_planet_blue(SDL_Surface* surface);
};