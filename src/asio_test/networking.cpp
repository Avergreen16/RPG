#include "net_include.cpp"

namespace msg {
    struct message_header {
        uint16_t ID;
        uint32_t size;
    };

    struct message {
        message_header header;
        std::vector<uint8_t> body;

        uint32_t get_size() {
            return sizeof(message_header) + body.size();
        }
    };
}