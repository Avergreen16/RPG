#include "client_setting.hpp"
#include "client_setting.cpp"
#include "asio_client.hpp"
#include "asio_client.cpp"

const uint pos_send_per_sec = 45;
const uint pos_send_mil = 1000000000 / pos_send_per_sec;

void chunk_gen_thread(bool& game_running, Setting& setting) {
    while(game_running) {
        if(setting.check_if_moved_chunk()) {
            //std::vector<uint> update_chunk_queue;
            std::array<uint, total_loaded_chunks> new_active_chunk_keys;
            std::array<int, 2> current_chunk_pos = {setting.current_chunk % world_size_chunks[0], setting.current_chunk / world_size_chunks[0]};
            std::vector<uint> chunk_requests;
            for(int x = 0; x <= chunk_load_x * 2; ++x) {
                for(int y = 0; y <= chunk_load_y * 2; ++y) {
                    uint chunk_key = setting.current_chunk + (x - chunk_load_x) + world_size_chunks[0] * (y - chunk_load_y);
                    /*if(insert_chunk(setting.loaded_chunks, world_size_chunks, chunk_key, worldgen)) {
                        update_chunk_queue.push_back(chunk_key);
                    }*/
                    if(!setting.loaded_chunks.contains(chunk_key)) {
                        chunk_requests.push_back(chunk_key);
                    }
                    new_active_chunk_keys[x + y * (chunk_load_x * 2 + 1)] = chunk_key;
                }
            }
            if(chunk_requests.size() != 0) {
                std::vector<uint8_t> chunk_request_packet = netwk::make_packet(3, (void*)chunk_requests.data(), chunk_requests.size() * sizeof(uint));
                setting.connection.send(chunk_request_packet);
            }

            setting.active_chunk_keys = new_active_chunk_keys;
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

    GLFWwindow* window = glfwCreateWindow(1000, 600, "test", NULL, NULL);
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

    Setting setting(window, connection, {23460.5, 10378.5}, player_name);
    glfwSetWindowUserPointer(window, &setting);
    glfwSetKeyCallback(window, Setting::key_callback);
    glfwSetCharCallback(window, Setting::char_callback);
    glfwSetScrollCallback(window, Setting::scroll_callback);


    bool game_running = true;

    std::thread chunk_thread(
        [&game_running, &setting]() {
            chunk_gen_thread(game_running, setting);
        }
    );

    connection.start(setting, game_running);
    setting.send_join_packet();

    time_t delta_time_container = get_time();
    time_t packet_time_container = get_time();

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while(game_running) {
        uint current_time = get_time();
        uint packet_delta_time = current_time - packet_time_container;
        if(packet_delta_time >= pos_send_mil) {
            packet_time_container = current_time;
            setting.send_positon_packet();
        }

        uint delta_time = current_time - delta_time_container;
        delta_time_container = current_time;
        setting.game_math(delta_time);

        
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        setting.render();
        setting.render_gui();
        
        glfwSwapBuffers(window);

        game_running = !glfwWindowShouldClose(window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    chunk_thread.join();

    return 0;
}