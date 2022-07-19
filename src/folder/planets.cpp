#define GLFW_INCLUDE_NONE
#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glfw\glfw3.h>
#include <glad\glad.h>
#include <array>
#include <map>
#include <cmath>
#include <ctime>
#include <vector>
#include <iostream>

//shaders

const char* vertex_shader_ts = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_f;
out vec2 internal_pos;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;
layout(location = 7) uniform vec4 texture_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / window_size * 2, z_value, 1.0);
    tex_coord_f = tex_coord * texture_rect.zw + texture_rect.xy;
    internal_pos = pos * 2 - 1;
}
)""";

const char* fragment_shader_ts = R"""(
#version 460 core

in vec2 tex_coord_f;
in vec2 internal_pos;

layout(location = 11) uniform float axial_tilt;
layout(location = 12) uniform float rotation_angle;
layout(location = 13) uniform float light_angle;

layout(location = 14) uniform sampler2D input_texture;

void main() {
    float pi = 3.14159265358979;
    float angle_in_radians = axial_tilt * pi / 180;
    float py_rot = internal_pos.y * cos(angle_in_radians) + internal_pos.x * sin(angle_in_radians);
    float px_rot = internal_pos.x * cos(angle_in_radians) - internal_pos.y * sin(angle_in_radians);

    if(px_rot * px_rot + py_rot * py_rot >= 1) {
        discard;
    }
    
    float width_at_height = sqrt(1 - py_rot * py_rot);
    float width_at_height_horizontal = sqrt(1 - internal_pos.y * internal_pos.y);
    float py2 = asin(py_rot) * 2/pi;
    float px2 = asin(px_rot / width_at_height) * 2/pi;
    float px3 = asin(internal_pos.x / width_at_height_horizontal) * 2/pi;

    float xCoord = (px2 + 1) / 4 + rotation_angle;
    float tex_x = xCoord - floor(xCoord);
    float tex_y = (py2 + 1) / 2;

    vec4 color = texture(input_texture, vec2(tex_x, tex_y));

    float light_angle2 = light_angle + 0.25;
    if(light_angle2 < 0) {
        light_angle2 += 1;
    }

    float light_xcoord = light_angle2 * 4 - 2;
    float shadow_xcoord0 = light_xcoord - 1;
    float shadow_xcoord1 = light_xcoord + 1;

    if(px3 > shadow_xcoord0 && px3 < shadow_xcoord1) {
        gl_FragColor = vec4(color.xyz / 4, 1.0);
    } else {
        gl_FragColor = vec4(color.xyz, 1.0);
    }
}
)""";

const char* vertex_shader_c_cs = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 internal_pos;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / window_size * 2, z_value, 1.0);
    internal_pos = pos * 2 - 1;
}
)""";

const char* fragment_shader_cs = R"""(
#version 460 core

in vec2 internal_pos;

layout(location = 7) uniform vec3 color;
layout(location = 10) uniform float light_angle;
layout(location = 11) uniform float axial_tilt;

void main() {
    float pi = 3.14159265358979;
    float angle_in_radians = axial_tilt * pi / 180;
    float py_rot = internal_pos.y * cos(angle_in_radians) + internal_pos.x * sin(angle_in_radians);
    float px_rot = internal_pos.x * cos(angle_in_radians) - internal_pos.y * sin(angle_in_radians);

    if(px_rot * px_rot + py_rot * py_rot >= 1) {
        discard;
    }
    
    float width_at_height = sqrt(1 - internal_pos.y * internal_pos.y);
    float px2 = asin(internal_pos.x / width_at_height) * 2/pi;

    float light_angle2 = light_angle + 0.25;
    if(light_angle2 < 0) {
        light_angle2 += 1;
    }

    float light_xcoord = (light_angle2) * 4 - 2;
    float shadow_xcoord0 = light_xcoord - 1;
    float shadow_xcoord1 = light_xcoord + 1;

    if(px2 > shadow_xcoord0 && px2 < shadow_xcoord1) {
        gl_FragColor = vec4(color / 4, 1.0);
    } else {
        gl_FragColor = vec4(color, 1.0);
    }
}
)""";

const char* fragment_shader_cs2 = R"""(
#version 460 core

in vec2 internal_pos;

layout(location = 7) uniform vec3 color;
layout(location = 10) uniform float light_angle;
layout(location = 11) uniform float axial_tilt;
layout(location = 12) uniform float eccentricity;
layout(location = 13) uniform float shear_factor;
layout(location = 14) uniform float bounding_box_height;
layout(location = 15) uniform float width_at_y0;

void main() {
    float pi = 3.14159265358979;
    float angle_in_radians = axial_tilt * pi / 180;
    float py_rot = (internal_pos.y * cos(angle_in_radians) + internal_pos.x * sin(angle_in_radians)) / (1.0 - eccentricity);
    float px_rot = internal_pos.x * cos(angle_in_radians) - internal_pos.y * sin(angle_in_radians);

    if(px_rot * px_rot + py_rot * py_rot >= 1) {
        discard;
    }
    
    float px_shear = internal_pos.x + shear_factor * internal_pos.y;
    float px_resize = px_shear / width_at_y0;
    float py_resize = internal_pos.y / bounding_box_height;

    float width_at_height = sqrt(1 - py_resize * py_resize);
    float px2 = asin(px_resize / width_at_height) * 2/pi;

    float light_angle2 = light_angle + 0.25;
    if(light_angle2 < 0) {
        light_angle2 += 1;
    }

    float light_xcoord = (light_angle2) * 4 - 2;
    float shadow_xcoord0 = light_xcoord - 1;
    float shadow_xcoord1 = light_xcoord + 1;

    if(px2 > shadow_xcoord0 && px2 < shadow_xcoord1) {
        gl_FragColor = vec4(color / 4, 1.0);
    } else {
        gl_FragColor = vec4(color, 1.0);
    }
}
)""";

const char* fragment_shader_c = R"""(
#version 460 core

in vec2 internal_pos;

layout(location = 7) uniform vec3 color;

void main() {
    if(internal_pos.x * internal_pos.x + internal_pos.y * internal_pos.y >= 1) {
        discard;
    }
    gl_FragColor = vec4(color, 1.0);
}
)""";

const char* vertex_shader_x = R"""(
#version 460 core
layout(location = 2) in vec2 pos;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / window_size * 2, z_value, 1.0);
}
)""";

const char* fragment_shader_x = R"""(
#version 460 core

layout(location = 7) uniform vec3 color;

void main() {
    gl_FragColor = vec4(color, 1.0);
}
)""";

const char* vertex_shader_b = R"""(
#version 460 core
layout(location = 0) in vec2 pos;

layout(location = 0) uniform float positions[300];

void main() {
    gl_Position = vec4((pos + vec2(positions[gl_InstanceID * 2], positions[gl_InstanceID * 2 + 1])), 0.9, 1.0);
}
)""";

const char* fragment_shader_b = R"""(
#version 460 core

void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)""";

const char* vertex_shader_i = R"""(
#version 460 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_f;
out vec2 internal_pos;

layout(location = 0) uniform float z_value;
layout(location = 1) uniform vec2 window_size;
layout(location = 3) uniform vec4 display_rect;
layout(location = 7) uniform vec4 texture_rect;

void main() {
    gl_Position = vec4((pos * display_rect.zw + display_rect.xy) / window_size * 2, z_value, 1.0);
    tex_coord_f = tex_coord * texture_rect.zw + texture_rect.xy;
    internal_pos = pos * 2 - 1;
}
)""";

const char* fragment_shader_i = R"""(
#version 460 core

in vec2 tex_coord_f;
in vec2 internal_pos;

layout(location = 11) uniform float light_angle;

layout(location = 12) uniform sampler2D input_texture;

void main() {
    float pi = 3.14159265358979;
    vec4 color = texture(input_texture, tex_coord_f);
    if(color.w == 0.0) discard;

    float width_at_height = sqrt(1 - internal_pos.y * internal_pos.y);
    float px2 = asin(internal_pos.x / width_at_height) * 2/pi;

    float light_angle2 = light_angle + 0.25;
    if(light_angle2 < 0) {
        light_angle2 += 1;
    }

    float light_xcoord = (light_angle2) * 4 - 2;
    float shadow_xcoord0 = light_xcoord - 1;
    float shadow_xcoord1 = light_xcoord + 1;

    if(px2 > shadow_xcoord0 && px2 < shadow_xcoord1) {
        gl_FragColor = vec4(color.xyz / 4, 1.0);
    } else {
        gl_FragColor = vec4(color);
    }
    
}
)""";

double gravitational_constant = 1;

unsigned int create_shader(const char* vertex_shader, const char* fragment_shader) {
    unsigned int v_shader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v_shader, 1, &vertex_shader, NULL);
    glShaderSource(f_shader, 1, &fragment_shader, NULL);
    glCompileShader(v_shader);
    glCompileShader(f_shader);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, v_shader);
    glAttachShader(shader, f_shader);
    glLinkProgram(shader);

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return shader;
}

//operators

bool collision(std::array<double, 4> a, std::array<double, 4> b) {
    return (a[0] <= b[0] + b[2] && a[0] + a[2] >= b[0] && a[1] <= b[1] + b[3] && a[1] + a[3] >= b[1]);
}

bool collision(std::array<int, 4> a, std::array<double, 4> b) {
    return (a[0] <= b[0] + b[2] && a[0] + a[2] >= b[0] && a[1] <= b[1] + b[3] && a[1] + a[3] >= b[1]);
}

bool point_collision(std::array<double, 4> a, std::array<double, 2> b) {
    return (b[0] >= a[0] && b[0] <= a[0] + a[2] && b[1] >= a[1] && b[1] <= a[1] + a[3]);
}

bool circle_point_collision(std::array<double, 2> a, std::array<double, 2> b, double radius) {
    return hypot(a[0] - b[0], a[1] - b[1]) <= radius;
}

bool collision(std::array<float, 4> a, std::array<float, 4> b) {
    return (a[0] <= b[0] + b[2] && a[0] + a[2] >= b[0] && a[1] <= b[1] + b[3] && a[1] + a[3] >= b[1]);
}

bool point_collision(std::array<float, 4> a, std::array<float, 2> b) {
    return (b[0] >= a[0] && b[0] <= a[0] + a[2] && b[1] >= a[1] && b[1] <= a[1] + a[3]);
}

bool circle_point_collision(std::array<float, 2> a, std::array<float, 2> b, float radius) {
    return hypot(a[0] - b[0], a[1] - b[1]) <= radius;
}

//

float vertices[] = {
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

float vertices_x[] = {
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f
};

std::array<double, 2> camera_pos;
float scale = 24;
double sqrt1_5 = std::sqrt(1.5);

std::map<int, bool> user_input_array = {
    {GLFW_KEY_W, false},
    {GLFW_KEY_S, false},
    {GLFW_KEY_A, false},
    {GLFW_KEY_D, false},
    {GLFW_KEY_SPACE, false},
    {GLFW_MOUSE_BUTTON_LEFT, false}
};

double add_shear = 0.0;
double add_height = 0.0;
double add_y0 = 0.0;
double add_const = 0.1;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
        else if(key == GLFW_KEY_EQUAL) scale *= 1.2;
        else if(key == GLFW_KEY_MINUS) scale /= 1.2;
        else if(user_input_array.contains(key)) {
            user_input_array[key] = true;
        }
    } else if(action == GLFW_RELEASE) {
       if(user_input_array.find(key) != user_input_array.end()) user_input_array[key] = false;
       else if(key == GLFW_KEY_N) add_shear -= add_const;
       else if(key == GLFW_KEY_M) add_shear += add_const;
       else if(key == GLFW_KEY_J) add_height -= add_const;
       else if(key == GLFW_KEY_K) add_height += add_const;
       else if(key == GLFW_KEY_I) add_y0 -= add_const;
       else if(key == GLFW_KEY_O) add_y0 += add_const;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if(yoffset > 0) scale *= sqrt1_5;
    else scale /= sqrt1_5;
}

std::array<double, 2> mouse_pos;
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if(user_input_array[GLFW_MOUSE_BUTTON_LEFT]) {
        double mouse_pos_change_x = xpos - mouse_pos[0];
        double mouse_pos_change_y = ypos - mouse_pos[1];

        camera_pos[0] -= mouse_pos_change_x / scale;
        camera_pos[1] += mouse_pos_change_y / scale;
    }
    mouse_pos = {xpos, ypos};
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
        if(action == GLFW_PRESS) {
            user_input_array[GLFW_MOUSE_BUTTON_LEFT] = true;
        } else if(action == GLFW_RELEASE) {
            user_input_array[GLFW_MOUSE_BUTTON_LEFT] = false;
        }
    }
}

unsigned int generate_texture(char address[], std::array<int, 3> info) {
    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    unsigned char *data = stbi_load(address, &info[0], &info[1], &info[2], 0); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info[0], info[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return texture_id;
}

bool check_point_inclusion(std::array<double, 4> intrect, std::array<double, 2> point) {
    if(point[0] >= intrect[0] && point[0] < intrect[0] + intrect[2] && point[1] >= intrect[1] && point[1] < intrect[1] + intrect[3]) return true;
    return false;
}

bool check_point_inclusion(std::array<int, 4> intrect, std::array<double, 2> point) {
    if(point[0] >= intrect[0] && point[0] < intrect[0] + intrect[2] && point[1] >= intrect[1] && point[1] < intrect[1] + intrect[3]) return true;
    return false;
}

//structs

enum shader_type{TEXTURE_SHADOW, COLOR_SHADOW, COLOR, IMAGE};
enum types{PLANEMO, BARYCENTER};

unsigned int x_shader;

struct Planemo;

struct Barycenter {
    std::array<double, 2> position;

    Planemo* member0;
    Planemo* member1;

    int parent_type = PLANEMO;
    Planemo* parent_p = NULL;
    Barycenter* parent_b = NULL;
    double orbital_period;
    double orbital_radius;
    double start_angle;
    bool retrograde;

    void construct(Planemo* pmember0, Planemo* pmember1);

    void set_orbit(int pparent_type, void* pparent, double porbital_radius, double pstart_angle, bool pretrograde);

    void set_binary_orbit(Planemo* plight_source, double porbital_radius, double pstart_angle, bool pretrograde);

    void set_position(clock_t time);

    void render(int window_width, int window_height, clock_t time);
};

struct Planemo {
    unsigned int shader;
    unsigned int shader_type;
    unsigned int texture;
    std::array<float, 3> color;

    std::array<double, 2> position;
    double mass;
    double radius;
    double density;
    std::array<double, 2> radii;
    float eccentricity = 0.0f;
    std::array<long double, 3> oblate_data;

    int parent_type = PLANEMO;
    Planemo* parent_p = NULL;
    Barycenter* parent_b = NULL;
    Planemo* light_source = NULL;
    double orbital_period;
    double orbital_radius;
    double start_angle;
    bool retrograde;
    double rotational_period;
    bool retrograde_rotation;
    double axial_tilt;

    bool small_marker = false;

    void construct(double pmass, double pdensity, double prot_period, bool pretrograde_rot, double paxial_tilt, bool psmall_marker = false)  {
        mass = pmass;
        density = pdensity;
        radius = cbrt((3 * pmass) / (4 * pdensity * M_PI)) * 10;

        rotational_period = prot_period;
        retrograde_rotation = pretrograde_rot;
        axial_tilt = paxial_tilt;
        small_marker = psmall_marker;
    }

    void set_orbit(int pparent_type, void* pparent, double porbital_radius, double pstart_angle, bool pretrograde) {
        orbital_radius = porbital_radius;
        start_angle = pstart_angle;
        retrograde = pretrograde;

        parent_type = pparent_type;
        if(parent_type == PLANEMO) {
            parent_p = (Planemo*)pparent;
            orbital_period = sqrt(orbital_radius * orbital_radius * orbital_radius / (gravitational_constant * parent_p->mass)) * 2 * M_PI;
        }
        else if(parent_type == BARYCENTER) {
            parent_b = (Barycenter*)pparent;
            orbital_period = sqrt(orbital_radius * orbital_radius * orbital_radius / (gravitational_constant * (parent_b->member0->mass + parent_b->member1->mass))) * 2 * M_PI;
        }
    }

    void set_shader(unsigned int pshader_type, unsigned int pshader, unsigned int ptexture, Planemo* plight_source, std::array<float, 3> pcolor) {
        shader = pshader;
        shader_type = pshader_type;
        texture = ptexture;
        light_source = plight_source;
        color = pcolor;
    }

    void set_position(clock_t time) {
        double angle = ((double(time) / (1000 * orbital_period)) * 2 * (1 - 2 * retrograde) + start_angle / 180) * M_PI;
        if(parent_type == PLANEMO) {
            if(parent_p != NULL) position = {parent_p->position[0] + cos(angle) * orbital_radius, parent_p->position[1] + sin(angle) * orbital_radius};
        } else if(parent_type == BARYCENTER) {
            position = {parent_b->position[0] + cos(angle) * orbital_radius, parent_b->position[1] + sin(angle) * orbital_radius};
        }
    }

    void set_oblate(float peccentricity) {
        double major_axis = cbrt(radius * radius * radius / (1 - peccentricity));
        radii = {major_axis, major_axis * (1 - peccentricity)};
        eccentricity = peccentricity;

        if(shader_type != IMAGE) {
            long double axial_tilt_radians = axial_tilt * M_PI/180;

            long double c = radii[0] * sin(axial_tilt_radians);
            long double d = radii[1] * sin(axial_tilt_radians + M_PI/2);
            long double h = -((radii[1] * radii[1] - radii[0] * radii[0]) * sin(2 * axial_tilt_radians)) / (2 * sqrt(radii[0] * radii[0] * sin(axial_tilt_radians) * sin(axial_tilt_radians) + radii[1] * radii[1] * cos(axial_tilt_radians) * cos(axial_tilt_radians)));

            long double bounding_box_height = sqrt(c * c + d * d);
            long double width_at_y0 = (radii[0] * radii[1]) / sqrt(radii[0] * radii[0] * sin(axial_tilt_radians) * sin(axial_tilt_radians) + radii[1] * radii[1] * cos(axial_tilt_radians) * cos(axial_tilt_radians));
            long double hyp = hypot(h, bounding_box_height);
            long double shear_factor = acos(bounding_box_height/hyp);
            printf("%f, %f, %f, %f, %f\n", radii[0], radii[1], shear_factor, bounding_box_height, width_at_y0);

            oblate_data = {shear_factor, bounding_box_height / radii[0], width_at_y0 / radii[0]};
        }
    }

    void tidal_lock() {
        rotational_period = orbital_period;
        retrograde_rotation = retrograde;
    }

    void render(int window_width, int window_height, clock_t time) {
        double scaled_diameter = radius * 2 * scale;

        if(scaled_diameter < 5) {
            std::array<int, 4> bounding_box;
            if(small_marker) bounding_box = {int((position[0] - camera_pos[0]) * scale) - 4, int((position[1] - camera_pos[1]) * scale) - 4, 7, 7};
            else bounding_box = {int((position[0] - camera_pos[0]) * scale) - 8, int((position[1] - camera_pos[1]) * scale) - 8, 15, 15};
            if(collision(bounding_box, std::array<double, 4>{-window_width * 0.5, -window_height * 0.5, (double)window_width, (double)window_height})) {
                glUseProgram(x_shader);

                glUniform1f(0, 0.5f);
                glUniform2f(1, window_width, window_height);
                glUniform4f(3, bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
                glUniform3f(7, color[0], color[1], color[2]);

                glDrawArrays(GL_LINES, 0, 4);
            }

        } else {
            std::array<double, 4> bounding_box;
            if(eccentricity != 0.0f) {
                if(shader_type == IMAGE) bounding_box = {(position[0] - radii[0] - camera_pos[0]) * scale, (position[1] - radii[1] - camera_pos[1]) * scale, radii[0] * 2 * scale, radii[1] * 2 * scale};
                else bounding_box = {(position[0] - radii[0] - camera_pos[0]) * scale, (position[1] - radii[0] - camera_pos[1]) * scale, radii[0] * 2 * scale, radii[0] * 2 * scale};
            } else {
                bounding_box = {(position[0] - radius - camera_pos[0]) * scale, (position[1] - radius - camera_pos[1]) * scale, radius * 2 * scale, radius * 2 * scale};
            }
            if(collision(bounding_box, std::array<double, 4>{-window_width * 0.5, -window_height * 0.5, (double)window_width, (double)window_height})) {
                glUseProgram(shader);
                double difference_x, difference_y;
                switch(shader_type) {
                    case TEXTURE_SHADOW:
                        glBindTexture(GL_TEXTURE_2D, texture);
                        glUniform1f(0, 0.5f);
                        glUniform2f(1, window_width, window_height);
                        glUniform4f(3, bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
                        glUniform4f(7, 0.0f, 0.0f, 1.0f, 1.0f);
                        glUniform1f(11, axial_tilt);
                        glUniform1f(12, (double(time) / 1000) / rotational_period * (-1 + 2 * retrograde_rotation));
                        difference_x = light_source->position[0] - position[0];
                        difference_y = light_source->position[1] - position[1];
                        glUniform1f(13, atan2(difference_y, difference_x) / (2 * M_PI));
                        /*glUniform1f(14, eccentricity);
                        glUniform1f(15, oblate_data[0] + add_shear);
                        glUniform1f(16, oblate_data[1] + add_height);
                        glUniform1f(17, oblate_data[2] + add_y0);*/

                        glDrawArrays(GL_TRIANGLES, 0, 6);
                        break;

                    case COLOR_SHADOW:
                        glUniform1f(0, 0.5f);
                        glUniform2f(1, window_width, window_height);
                        glUniform4f(3, bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
                        glUniform3f(7, color[0], color[1], color[2]);
                        difference_x = light_source->position[0] - position[0];
                        difference_y = light_source->position[1] - position[1];
                        glUniform1f(10, atan2(difference_y, difference_x) / (2 * M_PI));
                        glUniform1f(11, axial_tilt);
                        /*glUniform1f(12, eccentricity);
                        glUniform1f(13, oblate_data[0] + add_shear);
                        glUniform1f(14, oblate_data[1] + add_height);
                        glUniform1f(15, oblate_data[2] + add_y0);*/

                        glDrawArrays(GL_TRIANGLES, 0, 6);
                        break;


                    case COLOR:
                        glUniform1f(0, 0.5f);
                        glUniform2f(1, window_width, window_height);
                        glUniform4f(3, bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
                        glUniform3f(7, color[0], color[1], color[2]);

                        glDrawArrays(GL_TRIANGLES, 0, 6);
                        break;

                    case IMAGE:
                        glBindTexture(GL_TEXTURE_2D, texture);
                        glUniform1f(0, 0.5f);
                        glUniform2f(1, window_width, window_height);
                        glUniform4f(3, bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
                        glUniform4f(7, 0.0f, 0.0f, 1.0f, 1.0f);
                        difference_x = light_source->position[0] - position[0];
                        difference_y = light_source->position[1] - position[1];
                        glUniform1f(11, atan2(difference_y, difference_x) / (2 * M_PI));

                        glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }
    }
};

void Barycenter::construct(Planemo* pmember0, Planemo* pmember1) {
    member0 = pmember0;
    member1 = pmember1;
}

void Barycenter::set_orbit(int pparent_type, void* pparent, double porbital_radius, double pstart_angle, bool pretrograde) {
    orbital_radius = porbital_radius;
    start_angle = pstart_angle;
    retrograde = pretrograde;

    parent_type = pparent_type;
    if(parent_type == PLANEMO) {
        parent_p = (Planemo*)pparent;
        orbital_period = sqrt(orbital_radius * orbital_radius * orbital_radius / (gravitational_constant * parent_p->mass)) * 2 * M_PI;
    }
    else if(parent_type == BARYCENTER) {
        parent_b = (Barycenter*)pparent;
        orbital_period = sqrt(orbital_radius * orbital_radius * orbital_radius / (gravitational_constant * (parent_b->member0->mass + parent_b->member1->mass))) * 2 * M_PI;
    }
}

void Barycenter::set_binary_orbit(Planemo* plight_source, double porbital_radius, double pstart_angle, bool pretrograde) {
    member0->parent_type = BARYCENTER;
    member1->parent_type = BARYCENTER;
    member0->parent_b = this;
    member1->parent_b = this;
    
    member0->light_source = plight_source;
    member1->light_source = plight_source;
    member0->start_angle = pstart_angle;
    member1->start_angle = pstart_angle + 180;
    member0->retrograde = pretrograde;
    member1->retrograde = pretrograde;

    double o_r = porbital_radius / (1 + member0->mass / member1->mass);
    double o_p = sqrt(porbital_radius * porbital_radius * porbital_radius / (gravitational_constant * (member0->mass + member1->mass))) * 2 * M_PI;

    member0->orbital_radius = o_r;
    member1->orbital_radius = porbital_radius - o_r;
    member0->orbital_period = o_p;
    member1->orbital_period = o_p;
}

void Barycenter::set_position(clock_t time) {
    double angle = ((double(time) / (1000 * orbital_period)) * 2 * (1 - 2 * retrograde) + start_angle / 180) * M_PI;
    if(parent_type == PLANEMO) if(parent_p != NULL) position = {parent_p->position[0] + cos(angle) * orbital_radius, parent_p->position[1] + sin(angle) * orbital_radius};
    else if(parent_type == BARYCENTER) position = {parent_b->position[0] + cos(angle) * orbital_radius, parent_b->position[1] + sin(angle) * orbital_radius};
}

void Barycenter::render(int window_width, int window_height, clock_t time) {
    std::array<double, 4> bounding_box = {int((position[0] - camera_pos[0]) * scale) - 4, int((position[1] - camera_pos[1]) * scale) - 4, 7, 7};
    if(collision(bounding_box, std::array<double, 4>{-window_width * 0.5, -window_height * 0.5, (double)window_width, (double)window_height})) {
        glUseProgram(x_shader);

        glUniform1f(0, 0.5f);
        glUniform2f(1, window_width, window_height);
        glUniform4f(3, bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
        glUniform3f(7, 0.3, 0.3, 0.3);

        glDrawArrays(GL_LINES, 0, 4);
    }
}

enum direction{NORTH, EAST, SOUTH, WEST, NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST, NONE};
enum speed{NORMAL, FAST};

struct Camera {
    std::array<double, 2>* position;
    float* scale;
    int direction = NONE;
    int speed = NORMAL;

    void construct(std::array<double, 2>* pposition, float* pscale) {
        position = pposition;
        scale = pscale;
    }

    void tick(double delta) {
        if(direction != NONE) {
            double change_x = 0.0;
            double change_y = 0.0;
            
            switch(direction) {
                case NORTH:
                    change_y = 1.0 * delta;
                    break;
                case NORTHEAST:
                    change_x = 0.7071 * delta;
                    change_y = 0.7071 * delta;
                    break;
                case EAST:
                    change_x = 1.0 * delta;
                    break;
                case SOUTHEAST:
                    change_x = 0.7071 * delta;
                    change_y = -0.7071 * delta;
                    break;
                case SOUTH:
                    change_y = -1.0 * delta;
                    break;
                case SOUTHWEST:
                    change_x = -0.7071 * delta;
                    change_y = -0.7071 * delta;
                    break;
                case WEST:
                    change_x = -1.0 * delta;
                    break;
                case NORTHWEST:
                    change_x = -0.7071 * delta;
                    change_y = 0.7071 * delta;
                    break;
            }

            if(speed == NORMAL) {
                (*position)[0] += change_x / *scale;
                (*position)[1] += change_y / *scale;
            } else if(speed == FAST) {
                (*position)[0] += change_x * 2.5 / *scale;
                (*position)[1] += change_y * 2.5 / *scale;
            }
        }
    }

    void process_key_input() {
        if(user_input_array[GLFW_KEY_W]) {
            if(user_input_array[GLFW_KEY_D]) {
                direction = NORTHEAST;
            } else if(user_input_array[GLFW_KEY_S]) {
                direction = NONE;
            } else if(user_input_array[GLFW_KEY_A]) {
                direction = NORTHWEST;
            } else {
                direction = NORTH;
            }
        } else if(user_input_array[GLFW_KEY_D]) {
            if(user_input_array[GLFW_KEY_S]) {
                direction = SOUTHEAST;
            } else if(user_input_array[GLFW_KEY_A]) {
                direction = NONE;
            } else {
                direction = EAST;
            }
        } else if(user_input_array[GLFW_KEY_S]) {
            if(user_input_array[GLFW_KEY_A]) {
                direction = SOUTHWEST;
            } else {
                direction = SOUTH;
            }
        } else if(user_input_array[GLFW_KEY_A]) {
            direction = WEST;
        } else {
            direction = NONE;
        }

        if(user_input_array[GLFW_KEY_SPACE]) {
            speed = FAST;
        } else {
            speed = NORMAL;
        }

        /*if(user_input_array[GLFW_KEY_V]) {
            planemo->axial_tilt -= 0.1;
        } else if(user_input_array[GLFW_KEY_B]) {
            planemo->axial_tilt += 0.1;
        }*/
    }
};

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

void generate_stars(std::array<int, 2> map_size, unsigned char* map, int star_distance, std::vector<Planemo*>* planemo_v, std::vector<int>* order_v, unsigned int shader) {
    RNG rng;
    rng.set_seed(0.7923744, 67.0983648);
    int size = planemo_v->size();
    for(int y = 0; y < map_size[1]; y++) {
        for(int x = 0; x < map_size[0]; x++) {
            if(map[(x + y * map_size[0]) * 4 + 3] == 0xff) {
                unsigned char green_value = map[(x + y * map_size[0]) * 4 + 1];
                double remaining_star_amount;
                switch(green_value) {
                    case 0xae:
                        remaining_star_amount = 0.8;
                        break;

                    case 0xe7:
                        remaining_star_amount = 0.45;
                        break;

                    case 0x3b:
                        remaining_star_amount = 0.3;
                        break;
                        
                    case 0x26:
                        remaining_star_amount = 0.35;
                        break;

                    case 0x2b:
                        remaining_star_amount = 0.001;
                        break;

                    default:
                        remaining_star_amount = 0.0;
                }
                if(rng.retrieve() <= remaining_star_amount) {
                    double x_pos = (x) * star_distance;// + 0.2 + rng.retrieve() * 0.6
                    double y_pos = (y) * star_distance;// + 0.2 + rng.retrieve() * 0.6
                    planemo_v->push_back(new Planemo);
                    order_v->push_back(PLANEMO);
                    order_v->push_back(size);

                    planemo_v->at(size)->construct(50000, 10, 0, false, 0, true);
                    planemo_v->at(size)->set_shader(COLOR, shader, 0, NULL, {1.0f, 0.3f, 0.0f});
                    planemo_v->at(size)->position = {x_pos, y_pos};

                    size++;
                }
            }
        }
    }
    std::cout << planemo_v->size() << std::endl;
}

int main() {
    std::cout << "Gravitational constant: ";
    std::cin >> gravitational_constant;

    clock_t start_time = clock();
    clock_t time_storage = clock();
    clock_t time_storage_frames = clock();
    if(!glfwInit()) {
        printf("Catastrophic Failure! (init)");
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1000, 600, "test", NULL, NULL);
    if(!window) {
        printf("Catastrophic Failure! (window)");
        return 1;
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwMakeContextCurrent(window);

    gladLoadGL();

    glfwSwapInterval(0);

    unsigned int VBO, VAO, VBOx;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &VBOx);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, VBOx);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_x), vertices_x, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    int width, height, frames = 0;

    stbi_set_flip_vertically_on_load(true);
    unsigned int tex_terra = generate_texture("world_map_t101.png", {});
    unsigned int tex_shafur = generate_texture("world_map_shafur.png", {});
    unsigned int tex_gg0 = generate_texture("world_map_gg0.png", {});
    unsigned int tex_gg1 = generate_texture("world_map_gg1.png", {});
    unsigned int tex_a0 = generate_texture("world_map_a10.png", {});
    unsigned int tex_a1 = generate_texture("world_map_a1.png", {});
    unsigned int tex_a2 = generate_texture("world_map_a11.png", {});
    std::array<int, 3> galaxy_map_data;
    unsigned char* galaxy_map = stbi_load("galaxy_map.png", &galaxy_map_data[0], &galaxy_map_data[1], &galaxy_map_data[2], 4);

    unsigned int texture_shadow_shader = create_shader(vertex_shader_ts, fragment_shader_ts);
    unsigned int color_shadow_shader = create_shader(vertex_shader_c_cs, fragment_shader_cs);
    unsigned int color_shader = create_shader(vertex_shader_c_cs, fragment_shader_c);
    unsigned int background_shader = create_shader(vertex_shader_b, fragment_shader_b);
    unsigned int image_shader = create_shader(vertex_shader_i, fragment_shader_i);
    x_shader = create_shader(vertex_shader_x, fragment_shader_x);

    Camera cam; 
    cam.construct(&camera_pos, &scale);

    std::array<std::array<float, 2>, 150> background_v;
    for(int i = 0; i < 150; i++) {
        background_v[i] = {(rand() % 1000) * 0.002f - 1, (rand() % 1000) * 0.002f - 1};
    }

    std::vector<Planemo*> planemo_v;
    std::vector<Barycenter*> barycenter_v;
    //for(int i = 0; i < 9; i++) planemo_v.push_back(new Planemo);
    //for(int i = 0; i < 1; i++) barycenter_v.push_back(new Barycenter);
    std::vector<int> order_v;

    generate_stars({galaxy_map_data[0], galaxy_map_data[1]}, galaxy_map, 1000000, &planemo_v, &order_v, color_shader);
    
    planemo_v[100]->construct(189000, 1.5, 0, false, 0, true);
    planemo_v[100]->color = {1.0f, 0.8f, 0.0f};

    int size = planemo_v.size();
    planemo_v.push_back(new Planemo);
    planemo_v[size]->construct(20, 5.1, 60, false, -15, true);
    planemo_v[size]->set_shader(TEXTURE_SHADOW, texture_shadow_shader, tex_terra, planemo_v[100], {0.2f, 0.5f, 1.0f});
    planemo_v[size]->set_orbit(PLANEMO, (void*)planemo_v[100], 43000, 15, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(size);

    /*planemo_v[1]->construct(20, 5.1, 60, false, -15);
    planemo_v[1]->set_shader(TEXTURE_SHADOW, texture_shadow_shader, tex_terra, planemo_v[0], {0.2f, 0.5f, 1.0f});
    planemo_v[1]->set_orbit(PLANEMO, (void*)planemo_v[0], 13000, 15, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(1);
    //planemo_v[1]->set_oblate(0.3f);

    planemo_v[2]->construct(9, 4.7, 97, false, 3);
    planemo_v[2]->set_shader(TEXTURE_SHADOW, texture_shadow_shader, tex_shafur, planemo_v[0], {1.0f, 0.8f, 0.3f});
    planemo_v[2]->set_orbit(PLANEMO, (void*)planemo_v[0], 8000, -20, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(2);*/
    //planemo_v[2]->set_oblate(0.3f);

    /*barycenter_v[0]->construct(planemo_v[1], planemo_v[2]);
    barycenter_v[0]->set_orbit(PLANEMO, (void*)planemo_v[0], 10000, 20, false);
    barycenter_v[0]->set_binary_orbit(planemo_v[0], 50, 60, false);
    order_v.push_back(BARYCENTER);
    order_v.push_back(0);*/

    /*planemo_v[3]->construct(204, 0.5, 0, false, 0);
    planemo_v[3]->set_shader(TEXTURE_SHADOW, texture_shadow_shader, tex_gg1, planemo_v[0], {0.9f, 0.8f, 1.0f});
    planemo_v[3]->set_orbit(PLANEMO, (void*)planemo_v[0], 1000, 20, false);
    planemo_v[3]->tidal_lock();
    order_v.push_back(PLANEMO);
    order_v.push_back(3);
    //planemo_v[3]->set_oblate(0.45f);

    planemo_v[4]->construct(11640, 2.0, 187, false, 6);
    planemo_v[4]->set_shader(TEXTURE_SHADOW, texture_shadow_shader, tex_gg0, planemo_v[0], {0.6f, 0.2f, 0.3f});
    planemo_v[4]->set_orbit(PLANEMO, (void*)planemo_v[0], 25000, 10, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(4);

    planemo_v[5]->construct(45, 4.5, 0, false, 0);
    planemo_v[5]->set_shader(COLOR_SHADOW, color_shadow_shader, 0, planemo_v[0], {0.1f, 0.7f, 0.6f});
    planemo_v[5]->set_orbit(PLANEMO, (void*)planemo_v[4], 950, 70, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(5);

    planemo_v[6]->construct(0.001, 3.6, 0, false, 0, true);
    planemo_v[6]->set_shader(IMAGE, image_shader, tex_a0, planemo_v[0], {0.5f, 0.5f, 0.5f});
    planemo_v[6]->set_orbit(PLANEMO, (void*)planemo_v[2], 500, 250, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(6);

    planemo_v[7]->construct(0.0005, 2.8, 0, false, 0, true);
    planemo_v[7]->set_shader(IMAGE, image_shader, tex_a1, planemo_v[0], {0.5f, 0.5f, 0.5f});
    planemo_v[7]->set_orbit(PLANEMO, (void*)planemo_v[2], 500, 260, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(7);

    planemo_v[8]->construct(0.0002, 2.6, 0, false, 0, true);
    planemo_v[8]->set_shader(IMAGE, image_shader, tex_a2, planemo_v[0], {0.5f, 0.5f, 0.5f});
    planemo_v[8]->set_orbit(PLANEMO, (void*)planemo_v[2], 500, 255, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(8);
    planemo_v[8]->set_oblate(0.4f);*/

    /*planemo_v[6]->construct(0.5, 3.7, 0, false, 0);
    planemo_v[6]->set_shader(COLOR_SHADOW, color_shadow_shader, 0, planemo_v[0], {0.7f, 0.65f, 0.8f});
    planemo_v[6]->set_orbit(PLANEMO, (void*)planemo_v[4], 2000, 20, false);
    order_v.push_back(PLANEMO);
    order_v.push_back(6);*/

    while(!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glfwPollEvents();

        //keys

        cam.process_key_input();

        //math
        
        double delta_time = clock() - time_storage;
        time_storage = clock();

        cam.tick(delta_time);

        for(int i = 0; i < order_v.size() * 0.5; i++) {
            if(order_v[i * 2] == PLANEMO) {
                planemo_v[order_v[i * 2 + 1]]->set_position(clock() - start_time);
            } else if(order_v[i * 2] == BARYCENTER) {
                clock_t time0 = clock() - start_time;
                int index = order_v[i * 2 + 1];
                barycenter_v[index]->set_position(time0);
                barycenter_v[index]->member0->set_position(time0);
                barycenter_v[index]->member1->set_position(time0);
            }
        }

        //rendering
        //camera_pos = planemo_v[6]->position;

        //glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(background_shader);
        for(int i = 0; i < 150; i++) {
            glUniform1f(i * 2, background_v[i][0]);
            glUniform1f(i * 2 + 1, background_v[i][1]);
        }
        glDrawArraysInstanced(GL_POINTS, 0, 1, 150);

        for(int i = 0; i < order_v.size() * 0.5; i++) {
            if(order_v[i * 2] == PLANEMO) {
                planemo_v[order_v[i * 2 + 1]]->render(width, height, clock() - start_time);
            } else if(order_v[i * 2] == BARYCENTER) {
                clock_t time0 = clock() - start_time;
                int index = order_v[i * 2 + 1];
                //barycenter_v[index]->render(width, height, time0);
                barycenter_v[index]->member0->render(width, height, time0);
                barycenter_v[index]->member1->render(width, height, time0);
            }
        }

        //

        glfwSwapBuffers(window);
        frames++;
        
        time_t time_frames = clock();
        if(frames % 100 == 0) {
            printf("%f\n", (double)frames / (time_frames - time_storage_frames) * 1000);
            time_storage_frames = time_frames;
            frames = 0;
        }
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    for(Planemo* p : planemo_v) {
        delete p;
        p = nullptr;
    }
    for(Barycenter* p : barycenter_v) {
        delete p;
        p = nullptr;
    }

    return 0;
}