//
// Created by volund on 8/12/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_MODULES_H
#define SHENRON_INCLUDE_SHENRON_MODULES_H

#include <filesystem>
#include <unordered_map>
#include "entt/entt.hpp"
#include "nlohmann/json.hpp"
#include <optional>

namespace shenron::modules {

    struct Module;
    struct Prototype;

    extern std::unordered_map<std::string, Module> modules;

    struct Map {
        std::filesystem::path path;
        std::string key;
        std::optional<nlohmann::json> data;
        std::set<std::string> ent_keys;
        nlohmann::json get_data();
    };

    struct Prototype : public Map {
        std::set<std::string> ent_keys;
    };

    struct Module {
        std::filesystem::path path;
        std::string key;
        std::unordered_map<std::string, Prototype> prototypes;
        std::unordered_map<std::string, Map> maps;
        std::unordered_map<std::string, entt::entity> entities;
        void load();
        void load_prototypes();
        void load_maps();
        void load_entities_initial();
        void load_entities_finalize();
        void zone_reset();
    };


    void setup();

    void init_modules();
    void load_modules();
}

#endif //SHENRON_INCLUDE_SHENRON_MODULES_H
