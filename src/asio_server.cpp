#include "asio_lib.cpp"

/*uint __attribute__((always_inline)) per_sec(uint val) {
    return 1000000000 * (1.0 / val);
}*/

int main() {
    netwk::TCP_server server(0xa50e);

    netwk::enemy_map.emplace(0xfffe, netwk::Server_enemy({23472.5, 10378.5}, SOUTH));

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
        if(delta_time >= 15625000) { // max 64 times per second
            tick_container = current_time;
            for(auto& [id, enemy] : netwk::enemy_map) {
                enemy.tick(delta_time / 1000000); // turns delta_time into milliseconds
            }
        }
    }
}