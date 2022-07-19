#include <array>
#include <queue>
#include <unordered_map>
#include <vector>
#include <cmath>

#include "global.cpp"

struct vec_2 {
    int x;
    int y;
};

struct node {
    vec_2 pos;
    int d;
    int h;
    int prev;
};

vec_2 operator+(vec_2 a, vec_2 b) {
    return {a.x + b.x, a.y + b.y};
}

vec_2 operator-(vec_2 a, vec_2 b) {
    return {a.x - b.x, a.y - b.y};
}

bool operator==(vec_2 a, vec_2 b) {
    return a.x == b.x && a.y == b.y;
}

struct compare_node { 
    bool operator() (node a, node b) {
        return a.h > b.h;
    }
};

bool is_valid(vec_2 pos, vec_2 size) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y;
}

int get_key(vec_2 pos, int width) {
    return pos.x + pos.y * width;
}

int get_distance(vec_2 pos, vec_2 target) {
    vec_2 distance_vec = {abs(pos.x - target.x), abs(pos.y - target.y)};
    return abs(10 * (distance_vec.x + distance_vec.y) - 6 * std::min(distance_vec.x, distance_vec.y));
}

template<size_t width, size_t height>
directions Astar_pathfinding(vec_2 start_pos, vec_2 target_pos, std::array<std::array<int, height>, width> input_array) {
    if(start_pos == target_pos || !is_valid(target_pos, {width, height})) {
        return NONE;
    }
    
    std::priority_queue<node, std::vector<node>, compare_node> open_queue;
    std::unordered_map<int, node> closed_map;

    open_queue.push({start_pos, 0, get_distance(start_pos, target_pos), -1});
    
    bool target_found = false;

    node current_node;

    while(!target_found) {
        current_node = open_queue.top();
        open_queue.pop();
        
        if(current_node.pos == target_pos) {
            target_found = true;
            break;
        }

        int key = get_key(current_node.pos, width);

        vec_2 north_nbr = {current_node.pos.x, current_node.pos.y + 1};
        vec_2 ne_nbr = {current_node.pos.x + 1, current_node.pos.y + 1};
        vec_2 east_nbr = {current_node.pos.x + 1, current_node.pos.y};
        vec_2 se_nbr = {current_node.pos.x + 1, current_node.pos.y - 1};
        vec_2 south_nbr = {current_node.pos.x, current_node.pos.y - 1};
        vec_2 sw_nbr = {current_node.pos.x - 1, current_node.pos.y - 1};
        vec_2 west_nbr = {current_node.pos.x - 1, current_node.pos.y};
        vec_2 nw_nbr = {current_node.pos.x - 1, current_node.pos.y + 1};

        if(is_valid(ne_nbr, {width, height})) {
            if(!closed_map.contains(key + width + 1)) {
                open_queue.push({ne_nbr, current_node.d + 14, get_distance(ne_nbr, target_pos) + current_node.d + 14, key});
            } /*else if(closed_map[key + width + 1].d > current_node.d + 14) {
                closed_map.erase(key + width + 1);
                open_queue.push({ne_nbr, current_node.d + 14, get_distance(ne_nbr, target_pos) + current_node.d + 14, key});
            }*/
        }

        if(is_valid(se_nbr, {width, height})) {
            if(!closed_map.contains(key - width + 1)) {
                open_queue.push({se_nbr, current_node.d + 14, get_distance(se_nbr, target_pos) + get_distance(se_nbr, start_pos), key});
            } /*else if(closed_map[key - width + 1.d].d > current_node.d + 14) {
                closed_map.erase(key + width + 1);
                open_queue.push({se_nbr, current_node.d + 14, get_distance(se_nbr, target_pos) + get_distance(se_nbr, start_pos), key});
            }*/
        }

        if(is_valid(sw_nbr, {width, height})) {
            if(!closed_map.contains(key - width - 1)) {
                open_queue.push({sw_nbr, current_node.d + 14, get_distance(sw_nbr, target_pos) + current_node.d + 14, key});
            } /*else if(closed_map[key - width - 1].d > current_node.d + 14) {
                closed_map.erase(key - width - 1);
                open_queue.push({sw_nbr, current_node.d + 14, get_distance(sw_nbr, target_pos) + current_node.d + 14, key});
            }*/
        }

        if(is_valid(nw_nbr, {width, height})) {
            if(!closed_map.contains(key + width - 1)) {
                open_queue.push({nw_nbr, current_node.d + 14, get_distance(nw_nbr, target_pos) + current_node.d + 14, key});
            } /*else if(closed_map[key + width - 1].d > current_node.d + 14) {
                closed_map.erase(key + width - 1);
                open_queue.push({nw_nbr, current_node.d + 14, get_distance(nw_nbr, target_pos) + current_node.d + 14, key});
            }*/
        }

        if(is_valid(north_nbr, {width, height})) {
            if(!closed_map.contains(key + width)) {
                open_queue.push({north_nbr, current_node.d + 10, get_distance(north_nbr, target_pos) + current_node.d + 10, key});
            } /*else if(closed_map[key + width].d > current_node.d + 10) {
                closed_map.erase(key + width);
                open_queue.push({north_nbr, current_node.d + 10, get_distance(north_nbr, target_pos) + current_node.d + 10, key});
            }*/
        }

        if(is_valid(east_nbr, {width, height})) {
            if(!closed_map.contains(key + 1)) {
                open_queue.push({east_nbr, current_node.d + 10, get_distance(east_nbr, target_pos) + current_node.d + 10, key});
            } /*else if(closed_map[key + 1].d > current_node.d + 10) {
                closed_map.erase(key + 1);
                open_queue.push({east_nbr, current_node.d + 10, get_distance(east_nbr, target_pos) + current_node.d + 10, key});
            }*/
        }

        if(is_valid(south_nbr, {width, height})) {
            if(!closed_map.contains(key - width)) {
                open_queue.push({south_nbr, current_node.d + 10, get_distance(south_nbr, target_pos) + current_node.d + 10, key});
            } /*else if(closed_map[key - width].d > current_node.d + 10) {
                closed_map.erase(key - width);
                open_queue.push({south_nbr, current_node.d + 10, get_distance(south_nbr, target_pos) + current_node.d + 10, key});
            }*/
        }

        if(is_valid(west_nbr, {width, height})) {
            if(!closed_map.contains(key - 1)) {
                open_queue.push({west_nbr, current_node.d + 10, get_distance(west_nbr, target_pos) + current_node.d + 10, key});
            } /*else if(closed_map[key - 1].d > current_node.d + 10) {
                closed_map.erase(key - 1);
                open_queue.push({west_nbr, current_node.d + 10, get_distance(west_nbr, target_pos) + current_node.d + 10, key});
            }*/
        }


        closed_map.insert({key, current_node});
    }

    node prev_node;

    while(current_node.prev != -1) {
        prev_node = current_node;
        current_node = closed_map[current_node.prev];
    }

    vec_2 move_pos = prev_node.pos;

    if(move_pos.y > start_pos.y) {
        if(move_pos.x > start_pos.x) {
            return NORTHEAST;
        } else if(move_pos.x < start_pos.x) {
            return NORTHWEST;
        }
        return NORTH;
    } else if(move_pos.y < start_pos.y) {
        if(move_pos.x > start_pos.x) {
            return SOUTHEAST;
        } else if(move_pos.x < start_pos.x) {
            return SOUTHWEST;
        }
        return SOUTH;
    } else if(move_pos.x > start_pos.x) {
        return EAST;
    } else if(move_pos.x < start_pos.x) {
        return WEST;
    }
}