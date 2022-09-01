#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm-master\glm\glm.hpp>
#include <glm-master\glm\gtc\matrix_transform.hpp>

typedef uint32_t uint;

enum directions : uint16_t {NORTH, EAST, SOUTH, WEST, NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST, NONE, OUT_OF_RANGE};
enum states : uint16_t {IDLE, WALKING, RUNNING, SWIMMING};
