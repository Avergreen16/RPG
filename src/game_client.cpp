#include "client_core.hpp"
#include "client_core.cpp"
#include "asio_client.hpp"
#include "asio_client.cpp"

const uint pos_send_per_sec = 45;
const uint pos_send_mil = 1000000000 / pos_send_per_sec;

void chunk_gen_thread(bool& game_running, Core& core) {
    while(game_running) {
        if(core.check_if_moved_chunk()) {
            //std::vector<uint> update_chunk_queue;
            std::array<uint, total_loaded_chunks> new_active_chunk_keys;
            std::array<int, 2> current_chunk_pos = {core.current_chunk % world_size_chunks[0], core.current_chunk / world_size_chunks[0]};
            std::vector<uint> chunk_requests;
            for(int x = 0; x <= chunk_load_x * 2; ++x) {
                for(int y = 0; y <= chunk_load_y * 2; ++y) {
                    uint chunk_key = core.current_chunk + (x - chunk_load_x) + world_size_chunks[0] * (y - chunk_load_y);
                    /*if(insert_chunk(core.loaded_chunks, world_size_chunks, chunk_key, worldgen)) {
                        update_chunk_queue.push_back(chunk_key);
                    }*/
                    if(!core.loaded_chunks.contains(chunk_key)) {
                        chunk_requests.push_back(chunk_key);
                    }
                    new_active_chunk_keys[x + y * (chunk_load_x * 2 + 1)] = chunk_key;
                }
            }
            if(chunk_requests.size() != 0) {
                std::vector<uint8_t> chunk_request_packet = netwk::make_packet(3, (void*)chunk_requests.data(), chunk_requests.size() * sizeof(uint));
                core.connection.send(chunk_request_packet);
            }

            core.active_chunk_keys = new_active_chunk_keys;
        }
    }
}

int main() {
    netwk::TCP_client connection(0xa50e);

    if(!glfwInit()) {
        std::cout << "glfw failure (init)\n";
        return 1;
    }

    std::string player_name;
    std::getline(std::cin, player_name);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true); // debug context

    GLFWwindow* window = glfwCreateWindow(0x400, 0x280, "test", NULL, NULL);
    if(!window) {
        std::cout << "glfw failure (window)\n";
        return 1;
    }

    glfwMakeContextCurrent(window);

    gladLoadGL();

    glfwSwapInterval(0);

    uint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1); 

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    bool game_running = true;

    Core core(window, connection, {8307.5, 8996.5}, player_name, game_running);
    glfwSetWindowUserPointer(window, &core);
    glfwSetKeyCallback(window, Core::key_callback);
    glfwSetCharCallback(window, Core::char_callback);
    glfwSetScrollCallback(window, Core::scroll_callback);

    // debug context
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    std::thread chunk_thread(
        [&game_running, &core]() {
            chunk_gen_thread(game_running, core);
        }
    );

    connection.start(core, game_running);
    core.send_join_packet();

    time_t delta_time_container = get_time();
    time_t packet_time_container = get_time();

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_MEDIUM, -1, "error message here"); 

    while(game_running) {
        uint current_time = get_time();
        uint packet_delta_time = current_time - packet_time_container;
        if(packet_delta_time >= pos_send_mil) {
            packet_time_container = current_time;
            core.send_positon_packet();
        }

        uint delta_time = current_time - delta_time_container;
        delta_time_container = current_time;
        core.game_math(delta_time);

        
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        core.render(VAO);
        core.render_gui();
        
        glfwSwapBuffers(window);

        game_running = !glfwWindowShouldClose(window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    for(auto& [key, chunk] : core.loaded_chunks) {
        chunk.delete_buffers();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    chunk_thread.join();

    return 0;
}