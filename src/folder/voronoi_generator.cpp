#define _USE_MATH_DEFINES
#include <SFML-2.5.1\include\SFML\Graphics.hpp>
#include <cmath>
#include <vector>
#include <iostream>
#include <ctime>
#include <string>
#include <stdio.h>
#include <quadmath.h>

struct RNG {
    double factor;
    double product;

    void set_seed(double p_seed, double p_factor) {
        factor = p_factor;
        product = p_seed;
    }

    double retrieve() {
        product = product * factor;
        product -= floor(product);
        return product;
    }
};

long double get_angle(sf::Vector2<long double> input) {
    return atan2(input.x, input.y);
}

long double get_length(sf::Vector2<long double> input) {
    return std::sqrt(input.x * input.x + input.y * input.y);
}

long double get_slope(sf::Vector2<long double> p1, sf::Vector2<long double> p2) {
    long double delta_x = (p1.x - p2.x);
    if(delta_x == 0) return nanl("1");
    return (p1.y - p2.y) / delta_x;
}

sf::Vector2<long double> get_midpoint(sf::Vector2<long double> p1, sf::Vector2<long double> p2) {
    return sf::Vector2<long double>((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
}

long double get_value(sf::Vector2<long double> point, long double slope, long double x) {
    return slope * (x - point.x) + point.y;
}

sf::Vector2<long double> get_intersection(sf::Vector2<long double> p1, long double s1, sf::Vector2<long double> p2, long double s2) {
    if(s1 == s2) return sf::Vector2<long double>(nanl("1"), nanl("1"));
    if(__isnanl(s1)) {
        if(__isnanl(s2)) return sf::Vector2<long double>(nanl("1"), nanl("1"));
        return sf::Vector2<long double>(p1.x, get_value(p2, s2, p1.x));
    }
    if(__isnanl(s2)) return sf::Vector2<long double>(p2.x, get_value(p1, s1, p2.x));
    long double x = (s1 * p1.x - p1.y - s2 * p2.x + p2.y) / (s1 - s2);
    return sf::Vector2<long double>(x, get_value(p1, s1, x));
}

sf::Vector2<long double> p_invert(sf::Vector2<long double> point) {
    return sf::Vector2<long double>(point.y, point.x);
}

sf::Vector2<long double> create_angle_unit_vector(long double angle) {
    long double angle_in_radians = (long double)angle * M_PI / 180;
    return sf::Vector2<long double>(std::cos(angle_in_radians) , std::sin(angle_in_radians));
}

long double get_perpendicular_slope(long double slope) {
    if(__isnanl(slope)) return 0;
    if(slope == 0) return nanl("1");
    return -1 / slope;
}

std::pair<sf::Vector2<long double>*, sf::Vector2<long double>*> create_edge(sf::Vector2<long double>* p1, sf::Vector2<long double>* p2) {
    if(p1 > p2) {
        return std::pair(p1, p2);
    }
    return std::pair(p2, p1);
}

long double get_flipped_slope(long double slope) {
    if(__isnanl(slope)) return 0;
    if(slope == 0) return nanl("1");
    return 1 / slope;
}

void draw_line(sf::Vector2<long double> p1, sf::Vector2<long double> p2, sf::Image* img, sf::Color color) {
    long double slope = get_slope(p1, p2);
    if(__isnanl(slope)) {
        for(int y = round(p1.y); y != round(p2.y); y += (p2.y - p1.y) / fabs(p2.y - p1.y)) {
            img->setPixel(round(p1.x), y, color);
        }
    } else if(fabs(slope) <= 1) {
        for(int x = round(p1.x); x != round(p2.x); x += (p2.x - p1.x) / fabs(p2.x - p1.x)) {
            int y = round(get_value(p1, slope, (long double)x));
            img->setPixel(x, y, color);
        }
    } else {
        long double flipped_slope = get_flipped_slope(slope);
        for(int y = round(p1.y); y != round(p2.y); y += (p2.y - p1.y) / fabs(p2.y - p1.y)) {
            sf::Vector2<long double> inverted_p1(p1.y, p1.x);
            int x = round(get_value(inverted_p1, flipped_slope, (long double)y));
            img->setPixel(x, y, color);
        }
    }
}

class Dtriangle {
    public:

    sf::Vector2<long double> c_center;
    long double c_radius;
    
    sf::Vector2<long double> perp_vector;
    std::pair<int, int> inclusion_data;
    int infpoints;
    
    int sign(long double x) {
        return (x > 0) - (x < 0);
    }

    long double dot_product(sf::Vector2<long double> v1, sf::Vector2<long double> v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    
    std::vector<sf::Vector2<long double>*> points;

    Dtriangle() {};

    Dtriangle(sf::Vector2<long double>* p1, sf::Vector2<long double>* p2, sf::Vector2<long double>* p3, int p_infpoints) {
        points = {p1, p2, p3};
        infpoints = p_infpoints;
        if(infpoints == 0) {
            long double slope1 = get_perpendicular_slope(get_slope(*p1, *p2));
            sf::Vector2<long double> midpt1 = get_midpoint(*p1, *p2);
            long double slope2 = get_perpendicular_slope(get_slope(*p2, *p3));
            sf::Vector2<long double> midpt2 = get_midpoint(*p2, *p3);
            c_center = get_intersection(midpt1, slope1, midpt2, slope2);
            c_radius = get_length(c_center - *p2);
            //sf::Vector2<long double> c_center(p1->x * sin(2 * (get_angle(*p1 - *p2) - get_angle(*p1 - *p3) + 
            //)));
        } else if(infpoints == 1) {
            long double perp_slope = get_perpendicular_slope(get_slope(*p2, *p3));
            if(__isnanl(perp_slope)) {
                perp_vector = sf::Vector2<long double>(0, 1);
            } else perp_vector = sf::Vector2<long double>(1, perp_slope);
        } else if(infpoints == 2) {
            sf::Vector2<long double> inf_sign1(sign(p1->x), sign(p1->y));
            sf::Vector2<long double> inf_sign2(sign(p2->x), sign(p2->y));
            long double perp_slope = get_perpendicular_slope(get_slope(inf_sign1, inf_sign2));
            if(__isnanl(perp_slope)) {
                perp_vector = sf::Vector2<long double>(0, 1);
            } else perp_vector = sf::Vector2<long double>(1, perp_slope);
        }
    }

    bool circumcircle_inclusion(sf::Vector2<long double> point) {
        if(infpoints == 0) {
            sf::Vector2<long double> dist = c_center - point;
            return dist.x * dist.x + dist.y * dist.y < c_radius * c_radius;
        } else if(infpoints == 1 || infpoints == 2) {
            sf::Vector2<long double> inf_sign(sign(points[0]->x), sign(points[0]->y));
            return sign(dot_product(point - *points[2], perp_vector)) == sign(dot_product(inf_sign, perp_vector));
        } else if(infpoints == 3) {
            return true;
        }
    }
};

std::vector<Dtriangle> delaunay_triangulation(std::vector<sf::Vector2<long double>*> center_points, sf::Vector2i bounding_box_size) {
    std::vector<Dtriangle> good_triangles;
    std::vector<Dtriangle> bad_triangles;
    std::vector<std::pair<sf::Vector2<long double>*, sf::Vector2<long double>*>> edge_collector;
    std::vector<std::pair<sf::Vector2<long double>*, sf::Vector2<long double>*>> polygon;

    std::vector<sf::Vector2<long double>> exterior_points = {
        sf::Vector2<long double>(-INFINITY, -INFINITY),
        sf::Vector2<long double>(INFINITY, 0.0f),
        sf::Vector2<long double>(0.0f, INFINITY)
    };

    good_triangles.push_back(Dtriangle(&exterior_points[0], &exterior_points[1], &exterior_points[2], 3));

    for(sf::Vector2<long double>* point : center_points) {
        for(int j = 0; j < good_triangles.size(); j++) {
            Dtriangle tri = good_triangles[j];
            if(tri.circumcircle_inclusion(*point)) {
                bad_triangles.push_back(tri);
                good_triangles.erase(good_triangles.begin() + j);
                j--;
            }
        }

        for(Dtriangle tri : bad_triangles) {
            edge_collector.push_back(create_edge(tri.points[0], tri.points[1]));
            edge_collector.push_back(create_edge(tri.points[1], tri.points[2]));
            edge_collector.push_back(create_edge(tri.points[2], tri.points[0]));
        }

        for(int i = 0; i < edge_collector.size(); i++) {
            bool unique = true;
            std::pair<sf::Vector2<long double>*, sf::Vector2<long double>*> edge1 = edge_collector[i];
            for(int j = i + 1; j < edge_collector.size(); j++) {
                    std::pair<sf::Vector2<long double>*, sf::Vector2<long double>*> edge2 = edge_collector[j];
                    if(edge1.first == edge2.first && edge1.second == edge2.second) {
                    unique = false;
                    edge_collector.erase(edge_collector.begin() + j);
                    j--;
                }
            }
            if(unique) polygon.push_back(edge1);
            else {
                edge_collector.erase(edge_collector.begin() + i);
                i--;
            }
        }

        for(std::pair<sf::Vector2<long double>*, sf::Vector2<long double>*> edge : polygon) {
            int infpoints = 0;
            bool flip = false;
            if(edge.first == &exterior_points[0] || edge.first == &exterior_points[1] || edge.first == &exterior_points[2]) infpoints++;
            if(edge.second == &exterior_points[0] || edge.second == &exterior_points[1] || edge.second == &exterior_points[2]) {
                if(infpoints == 0) flip = true;
                infpoints++;
            }

            Dtriangle tri;
            flip ? tri = Dtriangle(edge.second, edge.first, point, infpoints) : tri = Dtriangle(edge.first, edge.second, point, infpoints);
            
            good_triangles.push_back(tri);
        }

        bad_triangles.clear();
        edge_collector.clear();
        polygon.clear();
    }

    for(int i = 0; i < good_triangles.size(); i++) {
        Dtriangle tri = good_triangles[i];
        for(sf::Vector2<long double>* point : tri.points) {
            if(point == &exterior_points[0] || point == &exterior_points[1] || point == &exterior_points[2]) {
                good_triangles.erase(good_triangles.begin() + i);
                i--;
                break;
            }
        }
    }

    return good_triangles;
}

bool compare_point(sf::Vector2<long double>* d1, sf::Vector2<long double>* d2) {
    //if(get_length(*d1 - sf::Vector2<long double>(250, 250)) < get_length(*d2 - sf::Vector2<long double>(250, 250))) return true;
    if(d1->x < d2->x) return true; 
    if(d1->x == d2->x && d1->y < d2->y) return true;
    return false;
}

int main() {
    time_t starting_time = std::time(NULL);

    sf::Vector2i img_size(500, 500);
    sf::Vector2i point_wh(20, 20);

    std::vector<sf::Vector2<long double>> input_points;

    RNG rng0;
    RNG rng;
    rng0.set_seed(0.6541764, 71.923749);

    int scale_factor = 1;

    for(int i = 0; i < 20; i++) {
        rng.set_seed(rng0.retrieve(), rng0.retrieve() * 56.29482);
        input_points.clear();
        for(int i = 0; i < point_wh.x; i++) {
            for(int j = 0; j < point_wh.y; j++) {
                //if(rng.retrieve() > 0.5) {
                    long double x = (i + rng.retrieve()) * ((long double)img_size.x / scale_factor / point_wh.x);
                    long double y = (j + rng.retrieve()) * ((long double)img_size.y / scale_factor / point_wh.y);
                    input_points.push_back(sf::Vector2<long double>(floor(x), floor(y)));
                //}
            }
        }

        std::vector<sf::Vector2<long double>*> pointers;
        for(int i = 0; i < input_points.size(); i++) {
            pointers.push_back(&input_points[i]);
        }

        std::sort(pointers.begin(), pointers.end(), compare_point);

        std::vector<Dtriangle> good_triangles = delaunay_triangulation(pointers, img_size);

        sf::Image img;
        img.create(img_size.x, img_size.y);
        sf::Color color(0x0000FFFF);

        for(Dtriangle tri : good_triangles) {
            draw_line(*tri.points[0] * (long double)scale_factor, *tri.points[1] * (long double)scale_factor, &img, color);
            draw_line(*tri.points[1] * (long double)scale_factor, *tri.points[2] * (long double)scale_factor, &img, color);
            draw_line(*tri.points[2] * (long double)scale_factor, *tri.points[0] * (long double)scale_factor, &img, color);
        }

        for(sf::Vector2<long double> point : input_points) {
            img.setPixel(point.x * scale_factor, point.y * scale_factor, sf::Color(0xFF0000FF));
        }
        
        img.saveToFile("triangulation" + std::to_string(i) + "t.png");
        printf("%i triangle count %i\n", i, good_triangles.size());
    }

    time_t ending_time = std::time(NULL);
    std::cout << std::to_string(ending_time - starting_time) << std::endl;
}