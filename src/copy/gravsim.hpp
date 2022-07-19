#include <vector>

struct Gravobject {
    std::vector<double> position;
    std::vector<double> velocity;
    Gravobject(std::vector<double> starting_position, std::vector<double> starting_velocity) {};
};