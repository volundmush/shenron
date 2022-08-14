//
// Created by volund on 8/12/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_UTILS_H
#define SHENRON_INCLUDE_SHENRON_UTILS_H

#include <string>
#include <set>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <list>


namespace shenron::utils {

    extern const char RANDOM_COLORS[];
    extern const char CCODE[];
    extern char *ANSI[];

    std::string processColors(const std::string &txt, int parse, char **choices);

    nlohmann::json json_from_file(const std::filesystem::path &p);

    std::string random_string(std::size_t length);

    std::string generate_id(const std::string &prf, std::size_t length, std::set<std::string> &existing);

    int64_t random_number(int64_t from, int64_t to);

    std::string tabularTable(const std::list<std::string> &elem, int field_width, int line_length, const std::string &out_sep);

    class Timer {
    public:
        explicit Timer(long ms);
        bool expired();
        void start();
        long remaining() const;
        void reset(long ms);
    private:
        long dur = 0;
        std::chrono::time_point<std::chrono::steady_clock> start_time;
    };

}

#endif //SHENRON_INCLUDE_SHENRON_UTILS_H
