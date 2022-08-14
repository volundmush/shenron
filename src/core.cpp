//
// Created by volund on 8/12/22.
//

#include "shenron/core.h"
#include "shenron/net.h"
#include "shenron/modules.h"
#include <chrono>
#include <iostream>
#include "fmt/format.h"

namespace shenron {

    std::bitset<3> game_state;

    namespace op_codes {
        const uint8_t NET = 0;
        const uint8_t GAME = 1;
        const uint8_t COPYOVER = 2;
    }

    void start() {
        setup();
        init_db();
        run();
    }

    void setup() {
        net::setup();
        std::string addr("0.0.0.0");
        net::create_listener(addr, 7999, net::Telnet);
        game_state[op_codes::GAME] = true;
        game_state[op_codes::NET] = true;
    }

    void restore(nlohmann::json &j) {

    }

    void init_db() {
        modules::setup();
    }

    void run() {
        while(game_state.any()) {
            auto start_time = std::chrono::steady_clock::now();

            if(game_state[op_codes::NET]) {
                net::run_net_incoming();
            }

            // Stick game logic here.

            if(game_state[op_codes::NET]) {
                net::run_net_outgoing();
            }

            auto end_time = std::chrono::steady_clock::now();

            auto delta = end_time - start_time;
            sleep(std::clamp(100L - delta.count(),1L,100L));
        }

    }

}