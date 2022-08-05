#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef unsigned int uint;

enum directions : uint16_t {NORTH, EAST, SOUTH, WEST, NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST, NONE, OUT_OF_RANGE};
enum states : uint16_t {IDLE, WALKING, RUNNING, SWIMMING};
