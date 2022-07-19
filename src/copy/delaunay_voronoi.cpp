#define _USE_MATH_DEFINES
#include <string>
#include <cmath>
#include <map>
#include <iostream>
#include <FreeImage\Dist\x64\FreeImage.h>
#include <vector>
#include <algorithm>

#include "delaunay_voronoi.hpp"

void D_triangle::calculate_circumcircle() {
    Point midpt1((p1->x + p2->x) / 2, (p1->y + p2->y) / 2);
    Point midpt2((p3->x + p2->x) / 2, (p3->y + p2->y) / 2);
    float slope1 = get_slope(*p1, *p2);
    float slope2 = get_slope(*p2, *p3);
    if(slope1 = INFINITY) {
        circumcenter = Point(midpt1.x, get_value(slope2, midpt2, midpt1.x));
    } else if(slope2 = INFINITY) {
        circumcenter = Point(midpt2.x, get_value(slope1, midpt1, midpt2.x));
    } else {
        circumcenter = get_intersection(slope1, midpt1, slope2, midpt2);
    }
    circumcircle_radius = get_length(*p1, circumcenter);

    circumcircle_calculated = true;
}

bool D_triangle::check_circumcircle(Point* input_point) {
    if(!circumcircle_calculated) {
        calculate_circumcircle();
    }
    if(get_length(*input_point, circumcenter) > circumcircle_radius) {
        return true;
    }
    return false;
}

std::vector<D_triangle> Generate_delaunay_triangulation(std::vector<Point> p_input_points) {
    std::vector<Point> input_points;
    std::vector<Point> exterior_points;
    std::vector<D_triangle> exterior_triangles;
    std::vector<D_triangle> interior_triangles;
    std::vector<D_triangle> bad_triangles;
    float min_x = input_points[0].x;
    float max_x = input_points[0].x;
    float min_y = input_points[0].y;
    float max_y = input_points[0].y;
    for(int i = 1; i < input_points.size(); i++) {
        Point p = input_points[i];
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }
    float c_x = (min_x + max_x) / 2;
    float c_y = (min_y + max_y) / 2;
    float r = get_length(Point(max_x - c_x + 5, max_y - c_y + 5), Point(c_x, c_y));
    exterior_points.push_back(Point(c_x, c_y + r));
    exterior_points.push_back(Point(c_x + cos((2/3) * M_PI) * r, c_y + sin((2/3) * M_PI) * r));
    exterior_points.push_back(Point(c_x + cos((4/3) * M_PI) * r, c_y + sin((4/3) * M_PI) * r));
    exterior_triangles.push_back(D_triangle(&exterior_points[0], &exterior_points[1], &exterior_points[2]));
    std::vector<Point*> reconnection_points;
    for(Point p : input_points) {
        for(D_triangle d_t : exterior_triangles) {
            if(d_t.check_circumcircle(&p)) {
                bad_triangles.push_back(d_t);
                exterior_triangles.erase(std::remove(exterior_triangles.begin(), exterior_triangles.end(), d_t), exterior_triangles.end());
            }
        }
        for(D_triangle d_t : interior_triangles) {
            if(d_t.check_circumcircle(&p)) {
                bad_triangles.push_back(d_t);
                interior_triangles.erase(std::remove(interior_triangles.begin(), interior_triangles.end(), d_t), interior_triangles.end());
            }
        }
    }
}