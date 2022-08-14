//
// Created by volund on 8/12/22.
//

#include "shenron/modules.h"
#include "shenron/utils.h"
#include <filesystem>
#include <iostream>
#include "async++.h"


namespace shenron::modules {

    std::unordered_map<std::string, Module> modules;

    nlohmann::json Map::get_data() {
        if(data) return data.value();
        data = shenron::utils::json_from_file(path);
        return data.value();
    }


    void setup() {
        init_modules();
        load_modules();
    }

    void init_modules() {
        auto mdir = std::filesystem::path("modules");
        std::cout << "Loading modules from " << mdir << std::endl;

        for(const auto &d : std::filesystem::directory_iterator(mdir)) {
            if(!d.is_directory()) continue;
            auto name = std::string(d.path().filename().string());
            auto &m = modules[name];
            m.key = name;
            m.path = d.path();
        }
    }

    void load_modules() {
        std::cout << "loading " << modules.size() << " modules!" << std::endl;
        async::parallel_for(modules, [&](auto &m) {
            m.second.load();
        });
        std::cout << "Finished loading modules!" << std::endl;
    }

    void Module::load() {
        load_prototypes();
        load_maps();
        load_entities_initial();
    }

    void Module::load_prototypes() {
        auto di = path / "prototypes";
        if(!std::filesystem::exists(di)) return;
        for(const auto &fp : std::filesystem::directory_iterator(di)) {
            if(fp.path().extension().string() == ".json") {
                auto pkey = std::string(fp.path().stem().filename().string());
                auto &pro = prototypes[pkey];
                pro.key = pkey;
                pro.path = fp;
            }
        }

    }

    void Module::load_maps() {
        auto di = path / "maps";
        if(!std::filesystem::exists(di)) return;
        for(const auto &fp : std::filesystem::directory_iterator(di)) {
            if(fp.path().extension().string() == ".json") {
                auto pkey = std::string(fp.path().stem().filename().string());
                auto &pro = maps[pkey];
                pro.key = pkey;
                pro.path = fp;
            }
        }
    }

    void Module::load_entities_initial() {

    }

    void Module::load_entities_finalize() {

    }

    void zone_reset() {

    }

}
