//
// Created by volund on 8/12/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_CORE_H
#define SHENRON_INCLUDE_SHENRON_CORE_H

#include "nlohmann/json.hpp"
#include <bitset>

namespace shenron {

    extern std::bitset<3> game_state;

    namespace op_codes {
        extern const uint8_t NET;
        extern const uint8_t GAME;
        extern const uint8_t COPYOVER;
    }

    void start();
    void setup();
    void restore(nlohmann::json &j);

    void init_db();
    void run();

}

#endif //SHENRON_INCLUDE_SHENRON_CORE_H
