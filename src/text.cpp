#pragma once
#include <unordered_map>
#include <map>
#include <string>
#include <array>
#include <sstream>
#include <vector>

#include "global.cpp"
#include "render.cpp"

std::map<char, int> standard_chars = {
    {'A', 0},
    {'B', 1},
    {'C', 2},
    {'D', 3},
    {'E', 4},
    {'F', 5},
    {'G', 6},
    {'H', 7},
    {'I', 8},
    {'J', 9},
    {'K', 10},
    {'L', 11},
    {'M', 12},
    {'N', 13},
    {'O', 14},
    {'P', 15},
    {'Q', 16},
    {'R', 17},
    {'S', 18},
    {'T', 19},
    {'U', 10},
    {'V', 21},
    {'W', 22},
    {'X', 23},
    {'Y', 24},
    {'Z', 25},

    {'a', 26},
    {'b', 27},
    {'c', 28},
    {'d', 29},
    {'e', 30},
    {'f', 31},
    {'g', 32},
    {'h', 33},
    {'i', 34},
    {'j', 35},
    {'k', 36},
    {'l', 37},
    {'m', 38},
    {'n', 39},
    {'o', 40},
    {'p', 41},
    {'q', 42},
    {'r', 43},
    {'s', 44},
    {'t', 45},
    {'u', 46},
    {'v', 47},
    {'w', 48},
    {'x', 49},
    {'y', 50},
    {'z', 51},

    {'0', 52},
    {'1', 53},
    {'2', 54},
    {'3', 55},
    {'4', 56},
    {'5', 57},
    {'6', 58},
    {'7', 59},
    {'8', 60},
    {'9', 61},

    {'.', 68},
    {'!', 69},
    {'?', 70},
    {':', 71},
    {';', 72},
    {',', 73},
    {'\'', 74},
    {'`', 75},
    {'\"', 76},
    {'(', 77},
    {')', 78},
    {'[', 79},
    {']', 80},
    {'{', 81},
    {'}', 82},
    {'+', 83},
    {'-', 84},
    {'=', 85},
    {'*', 86},
    {'<', 87},
    {'>', 88},
    {'^', 89},
    {'%', 90},
    {'_', 91},
    {'/', 92},
    {'\\', 93},
    {'&', 94},
    {'|', 95},
    {'@', 96},
    {'~', 97},
    {'#', 98},
    {'$', 99},
    {' ', 100}
};

std::map<char, int> hex_chars = {
    {'-', 84},
    {'0', 52},
    {'1', 53},
    {'2', 54},
    {'3', 55},
    {'4', 56},
    {'5', 57},
    {'6', 58},
    {'7', 59},
    {'8', 60},
    {'9', 61},
    {'a', 62},
    {'b', 63},
    {'c', 64},
    {'d', 65},
    {'e', 66},
    {'f', 67}
};

std::unordered_map<char, int> hex_to_dec = {
    {'0', 0},
    {'1', 1},
    {'2', 2},
    {'3', 3},
    {'4', 4},
    {'5', 5},
    {'6', 6},
    {'7', 7},
    {'8', 8},
    {'9', 9},
    {'a', 10},
    {'b', 11},
    {'c', 12},
    {'d', 13},
    {'e', 14},
    {'f', 15}
};

std::array<char, 11> valid_numbers = {'-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
std::array<char, 17> valid_hex_numbers = {'-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

int get_string_length(std::string str, int text_scale) {
    int collector = 0;
    for(char c : str) {
        if(standard_chars.contains(c)) {
            collector += 6;
        }
    }
    return collector * text_scale;
}

struct text_struct {
    std::string str;
    int size;
    int wrap;
    int len;
    int lines;
    std::vector<int> first_chars = {00};
    bool data_generated = false;

    void set_values(std::string str, int wrap, int size) {
        this->str = str;
        this->wrap = wrap;
        this->size = size;
        data_generated = false;
    }

    int generate_data() {
        if(data_generated == false) {
            data_generated = true;
            lines = 1;
            len = 0;

            bool hex_mode = false;
            int current_len = 0;
            int str_size = str.size();

            for(int i = 0; i < str_size; i++) {
                char c = str[i];
                if(i < str_size - 1 && c == '\\') {
                    switch(str[i + 1]) {
                        case 'x':
                            hex_mode = true;
                            i++;
                            continue;
                        
                        case 'n':
                            i++;
                            c = str[i + 1];
                            len = std::max(len, current_len);
                            current_len = 0;
                            if(c == ' ') {
                                int j = i + 1;
                                for(; j < str_size; ++j) {
                                    if(str[j] == ' ') ++i;
                                    else break;
                                }
                                if(i != str_size - 1) {
                                    ++lines;
                                    first_chars.push_back(i + 1);
                                }
                                continue;
                            } else {
                                ++lines;
                                first_chars.push_back(i + 1);
                            }

                        case 'd':
                            hex_mode = false;
                            i++;
                            continue;

                        case 'c':
                            if(i < str_size - 5 && hex_chars.contains(str[i + 2]) && hex_chars.contains(str[i + 3]) && hex_chars.contains(str[i + 4])) {
                                i += 4;
                                continue;
                            }
                            break;
                    }
                }
                if(hex_mode && hex_chars.contains(c)) {
                    int width = 6;
                    if((current_len + width) * size > wrap) {
                        len = std::max(len, current_len);
                        current_len = 0;
                        ++lines;
                        first_chars.push_back(i);
                    }
                    current_len += width;
                } else if(standard_chars.contains(c)) {
                    hex_mode = false;
                    int width = 6;
                    if((current_len + width) * size > wrap) {
                        len = std::max(len, current_len);
                        current_len = 0;
                        if(c == ' ') {
                            int j = i + 1;
                            for(; j < str_size; ++j) {
                                if(str[j] == ' ') ++i;
                                else break;
                            }
                            if(i != str_size - 1) {
                                ++lines;
                                first_chars.push_back(i);
                            }
                            continue;
                        } else {
                            ++lines;
                            first_chars.push_back(i);
                        }
                    }
                    current_len += width;
                }
            }
            len = std::max(len, current_len);
        }
        return len;
    }
};

int render_text(unsigned int shader, unsigned int source_img, std::array<int, 3> source_img_info, std::array<int, 2> window_size, std::array<float, 2> lower_left, std::string str, double text_scale, double wrap_around) {
    int lines = 1;
    bool hex_mode = false;
    std::array<float, 2> current_pos = lower_left;
    std::array<float, 3> color = {1.0f, 1.0f, 1.0f};
    int str_size = str.size();
    for(int i = 0; i < str_size; ++i) {
        char c = str[i];
        if(i < str_size - 1 && c == '\\') {
            switch(str[i + 1]) {
                case 'x':
                    hex_mode = true;
                    i++;
                    continue;
                
                case 'n':
                    current_pos[0] = lower_left[0];
                    current_pos[1] -= 13 * text_scale;
                    i++;
                    lines++;
                    continue;

                case 'd':
                    hex_mode = false;
                    i++;
                    continue;
                
                case 'c':
                    if(i < str_size - 5 && hex_chars.contains(str[i + 2]) && hex_chars.contains(str[i + 3]) && hex_chars.contains(str[i + 4])) {
                        float r = hex_to_dec[str[i + 2]];
                        float g = hex_to_dec[str[i + 3]];
                        float b = hex_to_dec[str[i + 4]];
                        color = {r / 15, g / 15, b / 15};
                        i += 4;
                        continue;
                    }
                    break;
            }
        }

        if(hex_mode && hex_chars.contains(c)) {
            double text_width = 6 * text_scale;
            
            glUseProgram(shader);

            glBindTexture(GL_TEXTURE_2D, source_img);
            glUniform1f(0, 0.0f);
            glUniform2f(1, (double)window_size[0], (double)window_size[1]);
            glUniform4f(3, current_pos[0], current_pos[1], float(text_width), float(10 * text_scale));
            glUniform4f(7,  (float)hex_chars[c] * 6 / source_img_info[0], 0.0f, 6.0f / source_img_info[0], 1.0f);
            glUniform4f(11, color[0], color[1], color[2], 1.0f);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            current_pos[0] += text_width;
            if(current_pos[0] - lower_left[0] > wrap_around) {
                current_pos[0] = lower_left[0];
                current_pos[1] -= 11 * text_scale;
                ++lines;
            }
        } else if(standard_chars.contains(c)) {
            hex_mode = false;
            double text_width = 6 * text_scale;

            if(current_pos[0] - lower_left[0] + text_width > wrap_around) {
                current_pos[0] = lower_left[0];
                current_pos[1] -= 11 * text_scale;
                if(c == ' ') {
                    int j = i + 1;
                    for(; j < str_size; ++j) {
                        if(str[j] == ' ') ++i;
                        else break;
                    }
                    if(i != str_size - 1) ++lines;
                    continue;
                }
                ++lines;
            }


            glUseProgram(shader);

            glBindTexture(GL_TEXTURE_2D, source_img);
            glUniform1f(0, 0.0f);
            glUniform2f(1, (double)window_size[0], (double)window_size[1]);
            glUniform4f(3, current_pos[0], current_pos[1], float(text_width), float(10 * text_scale));
            glUniform4f(7, (float)standard_chars[c] * 6 / source_img_info[0], 0.0f, 6.0f / source_img_info[0], 1.0f);
            glUniform4f(11, color[0], color[1], color[2], 1.0f);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            current_pos[0] += text_width;
        }
    }
    return lines;
}

enum esc_seq_enum{NON, HEX, NLINE, DEC, COL1, COL2};

void render_text_type(unsigned int shader, unsigned int source_img, std::array<int, 3> source_img_info, std::array<int, 2> window_size, std::array<float, 2> lower_left, std::string str, double text_scale, int type_pos, int screen_width) {
    bool hex_mode = false;
    std::array<float, 2> current_pos = lower_left;
    std::array<float, 3> color = {1.0f, 1.0f, 1.0f};
    int esc_seq = 0;
    esc_seq_enum prev_esc_seq = NON;
    if(str.size() == 0) {
        glBindTexture(GL_TEXTURE_2D, source_img);
        glUniform1f(0, 0.0f);
        glUniform2f(1, (double)window_size[0], (double)window_size[1]);
        glUniform4f(3, current_pos[0], current_pos[1], float(text_scale), float(10 * text_scale));
        glUniform4f(7, 359.0f / source_img_info[0], 0.0f, 1.0f / source_img_info[0], 1.0f);

        glUniform4f(11, 1.0f, 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    for(int i = 0; i < str.size(); i++) {
        if(prev_esc_seq != NON && esc_seq == 0) {
            switch(prev_esc_seq) {
                case HEX:
                    hex_mode = true;
                    prev_esc_seq = NON;
                    break;

                case NLINE:
                    prev_esc_seq = NON;
                    break;

                case DEC:
                    hex_mode = false;
                    prev_esc_seq = NON;
                    break;
                
                case COL1:
                    hex_mode = true;
                    prev_esc_seq = COL2;
                    esc_seq = 3;

                    break;
                
                case COL2:
                    hex_mode = false;
                    prev_esc_seq = NON;
                    break;
            }
        }
        
        char c = str[i];
        if(i < str.size() - 1 && c == '\\') {
            switch(str[i + 1]) {
                case 'x':
                    prev_esc_seq = HEX;
                    esc_seq = 2;
                    break;
                
                case 'n':
                    prev_esc_seq = NLINE;
                    esc_seq = 2;
                    break;

                case 'd':
                    prev_esc_seq = DEC;
                    esc_seq = 2;
                    break;
                
                case 'c':
                    if(i < str.size() - 4 && hex_chars.contains(str[i + 2]) && hex_chars.contains(str[i + 3]) && hex_chars.contains(str[i + 4])) {
                        esc_seq = 2;
                        prev_esc_seq = COL1;
                        float r = hex_to_dec[str[i + 2]];
                        float g = hex_to_dec[str[i + 3]];
                        float b = hex_to_dec[str[i + 4]];
                        color = {r / 15, g / 15, b / 15};
                    }
                    break;
            }
        }
        if(hex_mode && hex_chars.contains(c)) {
            double text_width = 6.0f * text_scale;
            
            glUseProgram(shader);

            glBindTexture(GL_TEXTURE_2D, source_img);
            glUniform1f(0, 0.00001f);
            glUniform2f(1, (double)window_size[0], (double)window_size[1]);
            glUniform4f(3, current_pos[0], current_pos[1], float(text_width), float(10 * text_scale));
            glUniform4f(7, (float)hex_chars[c] * 6 / source_img_info[0], 0.0f, 6.0f / source_img_info[0], 1.0f);

            if(esc_seq == 0) glUniform4f(11, color[0], color[1], color[2], 1.0f);
            else glUniform4f(11, color[0], color[1], color[2], 0.4f);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            if(i == type_pos) {
                glBindTexture(GL_TEXTURE_2D, source_img);
                glUniform1f(0, 0.0f);
                glUniform2f(1, (double)window_size[0], (double)window_size[1]);
                glUniform4f(3, current_pos[0], current_pos[1], float(text_scale), float(10 * text_scale));
                glUniform4f(7, 359.0f / source_img_info[0], 0.0f, 1.0f / source_img_info[0], 1.0f);

                glUniform4f(11, 1.0f, 1.0f, 1.0f, 1.0f);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            current_pos[0] += text_width;

            if(esc_seq > 0) --esc_seq;
        } else if(standard_chars.contains(c)) {
            hex_mode = false;

            double text_width = 6.0f * text_scale;
            
            glUseProgram(shader);

            glBindTexture(GL_TEXTURE_2D, source_img);
            glUniform1f(0, 0.00001f);
            glUniform2f(1, (double)window_size[0], (double)window_size[1]);
            glUniform4f(3, current_pos[0], current_pos[1], float(text_width), float(10 * text_scale));
            glUniform4f(7, (float)standard_chars[c] * 6 / source_img_info[0], 0.0f, 6.0f / source_img_info[0], 1.0f);

            if(esc_seq == 0) glUniform4f(11, color[0], color[1], color[2], 1.0f);
            else glUniform4f(11, color[0], color[1], color[2], 0.4f);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            if(i == type_pos) {
                glBindTexture(GL_TEXTURE_2D, source_img);
                glUniform1f(0, 0.0f);
                glUniform2f(1, (double)window_size[0], (double)window_size[1]);
                glUniform4f(3, current_pos[0], current_pos[1], float(text_scale), float(10 * text_scale));
                glUniform4f(7, 359.0f / source_img_info[0], 0.0f, 1.0f / source_img_info[0], 1.0f);

                glUniform4f(11, 1.0f, 1.0f, 1.0f, 1.0f);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            
            current_pos[0] += text_width;

            if(esc_seq > 0) --esc_seq;
        }
        if(current_pos[0] >= screen_width) return;
    }
    if(type_pos == str.size()) {
        glBindTexture(GL_TEXTURE_2D, source_img);
        glUniform1f(0, 0.0f);
        glUniform2f(1, (double)window_size[0], (double)window_size[1]);
        glUniform4f(3, current_pos[0], current_pos[1], float(text_scale), float(10 * text_scale));
        glUniform4f(7, 359.0f / source_img_info[0], 0.0f, 1.0f / source_img_info[0], 1.0f);

        glUniform4f(11, 1.0f, 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

std::string to_hex_string(int i) {
    std::stringstream s;
    s << "\\x" << std::hex << i;
    return s.str();
}

int get_len(std::string str) {
    bool hex_mode = false;
    int len = -1;
    for(int i = 0; i < str.size(); i++) {
        char c = str[i];
        if(i < str.size() - 2 && c == '\\') {
            switch(str[i + 1]) {
                case 'x':
                    hex_mode = true;
                    i++;
                    continue;

                case 'd':
                    hex_mode = false;
                    i++;
                    continue;
                
                case 'c':
                    if(i < str.size() - 4 && hex_chars.contains(str[i + 2]) && hex_chars.contains(str[i + 3]) && hex_chars.contains(str[i + 4])) {
                        i += 4;
                        continue;
                    }
                    break;
            }
        }
        if(hex_mode && hex_chars.contains(c)) {
            len += 6;
        } else if(standard_chars.contains(c)) {
            hex_mode = false;
            len += 6;
        }
    }
    return len;
}

