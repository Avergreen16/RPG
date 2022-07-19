#pragma once
#include <vector>

struct Point {
    float x;
    float y;
    Point () {};
    Point(float p_x, float p_y) : x(p_x), y(p_y) {};
};

float get_length(Point p1, Point p2) {
    return std::sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

float get_angle(Point p1, Point p2) {
    float x = p1.x - p2.x;
    float y = p1.y - p2.y;
    return std::atan2(y, x);
}

float get_slope(Point p1, Point p2) {
    float x = p2.x - p1.x;
    float y = p2.y - p1.y;
    if(x == 0) {
        return INFINITY;
    }
    return y/x;
}

float get_value_y(float slope, float x1, float y1, float y) {
    return (float)(y - y1) / slope + x1;
}

float get_value(float slope, Point p, float x) {
    return slope * (x - p.x) + p.y;
}

Point get_intersection(float slope1, Point p1, float slope2, Point p2) {
    float x = (-slope1 * p1.x + p1.y + slope2 * p2.x - p2.y) / (slope2 - slope1);
    float y = get_value(slope1, p1, x);
    return Point(x, y);
}

// - - - - - - - - - - - - - - - -

struct D_triangle {
    Point* p1;
    Point* p2;
    Point* p3;
    Point circumcenter;
    float circumcircle_radius;
    bool circumcircle_calculated = false;
    bool exterior;

    D_triangle(Point* p_p1, Point* p_p2, Point* p_p3) : p1(p_p1), p2(p_p2), p3(p_p3) {};

    void calculate_circumcircle();

    bool check_circumcircle(Point* input_point);
};

std::vector<D_triangle> Generate_delaunay_triangulation(std::vector<Point> p_input_points);