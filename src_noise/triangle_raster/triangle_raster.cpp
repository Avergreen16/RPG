#define FREEIMAGE_LIB
#define _USE_MATH_DEFINES
#include <iostream>
#include <ctime>
#include <cmath>
#include <vector>
#include <FreeImage\Dist\x64\FreeImage.h>
#include <bits/stdc++.h>
#include <algorithm>
#include <map>
#include "noisefactory.hpp"

struct triangle {
    Point p1;
    Point p2;
    Point p3;
    triangle(Point p_p1, Point p_p2, Point p_p3) : p1(p_p1), p2(p_p2), p3(p_p3) {};
    void fill(FIBITMAP* img, RGBQUAD color);
};

struct {
    bool operator() (Point a, Point b) const {
        return a.y < b.y;
    }
} point_sort_y;

struct {
    bool operator() (Point a, Point b) const {
        return a.x < b.x;
    }
} point_sort_x;

void fill(FIBITMAP* img, int starting_x, int starting_y, RGBQUAD color);

class PerlinNoise1 {
    public:
        int array_length;
        int scale;
        int octaves;
        float persistance;
        float maxvalue;
        bool loop;

        std::map<int, float> slope_array;
        RNG rng;
        PerlinNoise1(float p_seed, float p_factor, int p_array_length, int p_scale, int p_octaves, float p_persistance, bool p_loop);
        void generate_slopes();
        float pullvalue(float x);
        float interpolate(float x1, float x2, float coordinate, float scale);
};

struct Line {
    Point pt;
    float slope;

    Line() {};
    Line(Point p_pt, float p_slope) : pt(p_pt), slope(p_slope) {};
    void rasterize(FIBITMAP* img, RGBQUAD color, int img_w, int img_h);
};

struct LineSegment {
    Point p1;
    Point p2;
    std::vector<LineSegment> children;
    std::vector<Point> child_points;
    bool distorted = false;

    LineSegment() {}
    LineSegment(Point p_p1, Point p_p2) : p1(p_p1), p2(p_p2) {}
    float get_length();
    float get_angle();
    float get_slope();
    Point get_midpoint();
    void rasterize(FIBITMAP* img, RGBQUAD color);
    void perlin_noise_distortion(PerlinNoise1 generator, int factor, int line_count);
    void rasterize_distortion(FIBITMAP* img, RGBQUAD color);
    Line get_perp_bisector();
    std::vector<float> convert_to_vector();
};

template <typename T>
int signum(T input) {
    if (input > 0) {
        return 1;
    }
    if (input < 0) {
        return -1;
    }
    return 0;
}




//----------------------------

int main() {
    time_t starting_time = std::time(NULL);

    char filename[] = "triangle_raster.png";
    RGBQUAD color = set_color(255, 255, 255);
    FreeImage_Initialise();
    FIBITMAP* img = FreeImage_Allocate(512, 512, 24);

    VoronoiFactory v_fac(0.827364, 512, 512, 8, 8, true, true);
    v_fac.generate_points();

    std::vector<Line> line_vector;
    int target_point_key = 35;
    Point target_point = v_fac.voronoi_map[target_point_key].transfer();
    for(int x = -2; x < 3; x++) {
        for(int y = -2; y < 3; y++) {
            Point point = v_fac.voronoi_map[target_point_key + x + y * v_fac.voronoi_map_width].transfer();
            LineSegment line_segment(target_point, point);
            line_vector.push_back(line_segment.get_perp_bisector());
        }
    }


    for(int i = 0; i < line_vector.size(); i++) {
        for(int j = 0; j < line_vector.size(); j++) {
            if (j != i) {
                Point intersection_point = get_intersection(line_vector[i], line_vector[j]);
                std::vector<Line> lines = {line_vector[i], line_vector[j]};
                for(int k = 0; k < line_vector.size(); k++) {
                    if(k != i && k != j) {
                        
                    }
                }
            }
        }
    }


    if (FreeImage_Save(FIF_PNG, img, filename)) {
        std::cout << "Image saved as " << filename << std::endl;
    }

    FreeImage_DeInitialise();
    time_t ending_time = std::time(NULL);
    std::cout << ending_time - starting_time << std::endl;
    return 0;
}

Point get_intersection(Line line1, Line line2) {
    float x = (-line1.slope * line1.pt.x + line1.pt.y + line2.slope * line2.pt.x - line2.pt.y) / (line2.slope - line1.slope);
    float y = get_value(line1.slope, line1.pt.x, line1.pt.y, x);
    return Point(x, y);
}

//-----------------------------

Line LineSegment::get_perp_bisector() {
    float slope = -1/get_slope();
    Point midpoint = get_midpoint();
    return Line(midpoint, slope);
}


void Line::rasterize(FIBITMAP* img, RGBQUAD color, int img_w, int img_h) {
    if(fabs(slope) <= 1) {
        for(int i = 0; i < img_w; i++) {
            int value = round(get_value(slope, pt.x, pt.y, i));
            FreeImage_SetPixelColor(img, i, value, &color);
        }
    } else {
        for(int i = 0; i < img_h; i++) {
            int value = round(get_value_y(slope, pt.x, pt.y, i));
            FreeImage_SetPixelColor(img, value, i, &color);
        }
    }
}

PerlinNoise1::PerlinNoise1(float p_seed, float p_factor, int p_array_length, int p_scale, int p_octaves, float p_persistance, bool p_loop) 
    : array_length(p_array_length), scale(p_scale), octaves(p_octaves), persistance(p_persistance), loop(p_loop) {
    rng = RNG(p_seed, p_factor);

    //for (int i = 0; i < p_octaves; i++) {
        //maxvalue += 1 * pow(persistance, -i);
    //}
}

void PerlinNoise1::generate_slopes() {
    for(int i = 0; i < array_length * pow(2, octaves); i++) {
        float slope = rng.retrieve() * 2 - 1;
        slope_array[i] = slope;
    }
}

float PerlinNoise1::interpolate(float x1, float x2, float coordinate, float scale) {
    float scaled_coordinate = (float)(fmod(coordinate, scale)) / scale;
    float returnvalue = (x2 - x1) * (6 * pow(scaled_coordinate, 5) - 15 * pow(scaled_coordinate, 4) + 10 * pow(scaled_coordinate, 3)) + x1;
    return returnvalue;
}

void LineSegment::perlin_noise_distortion(PerlinNoise1 generator, int factor, int line_count) {
    child_points.push_back(p1);
    for(int i = 1; i < line_count + 1; i++) {
        float value_x = i * get_length() / line_count;
        float value_y = generator.pullvalue(generator.scale * generator.array_length / get_length() * value_x) * factor;
        float angle = get_angle();

        Point point1(0, 0);
        Point point2(round(value_x), round(value_y));
        
        float length = get_length_2(point1, point2);
        float angle_2 = get_angle_2(point1, point2);
        float true_angle = angle + angle_2;
        child_points.push_back(Point(cos(true_angle) * length + p1.x, sin(true_angle) * length + p1.y));
    }
    for(int i = 0; i < child_points.size() - 1; i++) {
        children.push_back(LineSegment(child_points[i], child_points[i+1]));
    }
    distorted = true;
}

float PerlinNoise1::pullvalue(float x) {
    float returnvalue;
    for (int i = 0; i < octaves; i++) {
        int o_lsk = floor(x / (scale / pow(2, i)));
        int o_rsk = o_lsk + 1;
        int left_slope_key = o_lsk;
        int right_slope_key = o_rsk;
        if (loop) {
            while (left_slope_key < array_length * pow(2, i)) {
                left_slope_key += array_length * pow(2, i);
            }
            while (left_slope_key > array_length * pow(2, i)) {
                left_slope_key -= array_length * pow(2, i);
            }
            while (right_slope_key < array_length * pow(2, i)) {
                right_slope_key += array_length * pow(2, i);
            }
            while (right_slope_key > array_length * pow(2, i)) {
                right_slope_key -= array_length * pow(2, i);
            }
        }

        if (slope_array.find(left_slope_key) == slope_array.end()) {
            slope_array[left_slope_key] = rng.retrieve() * 2 - 1;
        }
        if (slope_array.find(right_slope_key) == slope_array.end()) {
            slope_array[right_slope_key] = rng.retrieve() * 2 - 1;
        }

        float left_slope = slope_array[left_slope_key];
        float right_slope = slope_array[right_slope_key];
        float left_value = get_value(left_slope, o_lsk * (scale / pow(2, i)), 0, x);
        float right_value = get_value(right_slope, o_rsk * (scale / pow(2, i)), 0, x);

        returnvalue += interpolate(left_value, right_value, x, scale / pow(2, i));// * pow(persistance, -i);
    }
    return returnvalue; /// maxvalue;
}

void triangle::fill(FIBITMAP* img, RGBQUAD color) {
    std::vector<Point> y_vec = {p1, p2, p3};
    std::sort(y_vec.begin(), y_vec.end(), point_sort_y);
    std::vector<Point> x_vec;
    bool first = true;
    bool second = true;
    float slope1;
    float slope2;
    int value1;
    int value2;
    for(int y = y_vec[0].y; y <= y_vec[2].y; y++) {
        if (y < y_vec[1].y) {
            if(first) {
                x_vec = {y_vec[1], y_vec[2]};
                std::sort(x_vec.begin(), x_vec.end(), point_sort_x);
                slope1 = (float)(y_vec[0].y - x_vec[0].y) / (y_vec[0].x - x_vec[0].x);
                slope2 = (float)(y_vec[0].y - x_vec[1].y) / (y_vec[0].x - x_vec[1].x);
                first = false;
            }
            value1 = round(get_value_y(slope1, y_vec[0].x, y_vec[0].y, y));
            value2 = round(get_value_y(slope2, y_vec[0].x, y_vec[0].y, y));
            for (int x = value1; x <= value2; x++) {
                FreeImage_SetPixelColor(img, x, y, &color);
            }
        } else {
            if(second) {
                x_vec = {y_vec[0], y_vec[1]};
                std::sort(x_vec.begin(), x_vec.end(), point_sort_x);
                slope1 = (float)(y_vec[2].y - x_vec[0].y) / (y_vec[2].x - x_vec[0].x);
                slope2 = (float)(y_vec[2].y - x_vec[1].y) / (y_vec[2].x - x_vec[1].x);
                second = false;
            }
            value1 = round(get_value_y(slope1, y_vec[2].x, y_vec[2].y, y));
            value2 = round(get_value_y(slope2, y_vec[2].x, y_vec[2].y, y));
            for (int x = value1; x <= value2; x++) {
                FreeImage_SetPixelColor(img, x, y, &color);
            }
        }
    }
}

float LineSegment::get_length() {
    return std::sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

float LineSegment::get_angle() {
    float x = p1.x - p2.x;
    float y = p1.y - p2.y;
    return std::atan2(y, x);
}

void LineSegment::rasterize(FIBITMAP* img, RGBQUAD color) {
    float slope = (p1.y - p2.y) / (p1.x - p2.x);
    if(fabs(slope) <= 1) {
        for(int i = p1.x; i != p2.x + signum(p2.x - p1.x); i += signum(p2.x - p1.x)) {
            int value = round(get_value(slope, p1.x, p1.y, i));
            FreeImage_SetPixelColor(img, i, value, &color);
        }
    } else {
        for(int i = p1.y; i != p2.y + signum(p2.y - p1.y); i += signum(p2.y - p1.y)) {
            int value = round(get_value_y(slope, p1.x, p1.y, i));
            FreeImage_SetPixelColor(img, value, i, &color);
        }
    }
}

float LineSegment::get_slope() {
    return (p1.y - p2.y) / (p1.x - p2.x);
}

Point LineSegment::get_midpoint() {
    return Point((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
}

void LineSegment::rasterize_distortion(FIBITMAP* img, RGBQUAD color) {
    for(LineSegment _line : children) {
        _line.rasterize(img, color);
    }
}

void fill(FIBITMAP* img, int x, int y, RGBQUAD color) {
    RGBQUAD check_color;
    FreeImage_GetPixelColor(img, x, y, &check_color);
    if(check_color.rgbRed == 0 && check_color.rgbGreen == 0 && check_color.rgbBlue == 0) {
        FreeImage_SetPixelColor(img, x, y, &color);
        fill(img, x + 1, y, color);
        fill(img, x, y + 1, color);
        fill(img, x - 1, y, color);
        fill(img, x, y - 1, color);
    }
}