#pragma once
#include <array>
#include <string>
#include <vector>

#include "global.cpp"

namespace netwk {
    struct packet_header {
        uint16_t type;
        uint32_t size_bytes;
    };

    struct entity_movement_packet_toserv {
        std::array<double, 2> position;
        directions direction;
    };

    struct entity_movement_packet_toclient {
        uint64_t entity_id;
        std::array<double, 2> position;
        directions direction;
    };

    struct player_join_packet_toserv {
        std::array<char, 64> name;
        entity_movement_packet_toserv entity_packet;
    };

    struct player_join_packet_toclient {
        std::array<char, 64> name;
        entity_movement_packet_toclient entity_packet;
    };

    template<typename type> 
    type convert_byte_vector(uint8_t* input_ptr) {
        type return_object;
        memcpy(&return_object, input_ptr, sizeof(type));
        return return_object;
    }

    template<typename type>
    std::vector<uint8_t> to_byte_vector(type& input) {
        std::vector<uint8_t> return_vector;
        memcpy(return_vector.data(), &input, sizeof(type));
        return return_vector;
    }
    
    std::string make_string(char* input_ptr, size_t size_bytes, bool terminator_char = true) {
        std::string return_string(input_ptr, size_bytes);
        if(terminator_char) return_string += '\0';
        return return_string;
    }

    std::string make_string_array(char* input_ptr, size_t size_bytes) {
        std::string return_string;
        for(int i = 0; i < size_bytes; i++) {
            char c = input_ptr[i];
            return_string += c;
            if(c == '\0') break;
        }
        return return_string;
    }
}