#include "asio_lib.cpp"

int main() {
    netwk::TCP_server server(0xa50e);

    netwk::enemy_map.emplace(0xfffe, netwk::Server_enemy({8317.5, 9003.5}, SOUTH));

    std::thread server_thread(
        [&server]() {
            server.run();
        }
    );

    bool run_server = true;

    time_t tick_container = get_time();
    while(run_server) {
        time_t current_time = get_time();
        uint delta_time = current_time - tick_container;
        if(delta_time >= ticks_mil) {
            tick_container = current_time;
            for(auto& [id, enemy] : netwk::enemy_map) {
                enemy.tick(delta_time);
            }
        }
    }
}