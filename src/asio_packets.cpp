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
        states state;
    };

    struct entity_movement_packet_toclient {
        uint64_t entity_id;
        std::array<double, 2> position;
        directions direction;
        states state;
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
    struct packet {
        packet_header header = {UINT16_MAX, sizeof(type)};
        type body;

        packet(uint16_t id, type body) {
            this->header.type = id;
            this->body = body;
        }
    };

    template<typename type> 
    type from_byte_vector(uint8_t* input_ptr) {
        type return_object;
        memcpy(&return_object, input_ptr, sizeof(type));
        return return_object;
    }

    template<typename type>
    std::vector<uint8_t> to_byte_vector(type& input) {
        std::vector<uint8_t> return_vector(sizeof(type));
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