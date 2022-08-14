//
// Created by volund on 8/12/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_ECS_H
#define SHENRON_INCLUDE_SHENRON_ECS_H

#include "entt/entt.hpp"
#include "nlohmann/json.hpp"
#include <any>
#include <map>
#include <unordered_map>
#include <optional>

namespace shenron::ecs {
    extern entt::registry reg;

    using SpaceCoordinates = std::array<float, 3>;
    using GridCoordinates = std::array<int16_t, 3>;

    struct PlayerCharacter {
        int64_t player_id = -1;
    };

    struct LegacyRoom {
        uint32_t vnum;
    };

    struct LegacyMob {
        uint32_t vnum;
    };

    struct LegacyObj {
        uint32_t vnum;
    };

    struct NPC {};
    struct Item {};
    struct Character {};
    struct Room {};

    struct StringBase {
        std::string color;
        std::string plain;
    };

    struct Name : StringBase {};
    struct Description : StringBase {};
    struct RoomDescription : StringBase {};
    struct ShortDescription : StringBase {};

    struct Triggers {
        std::vector<uint32_t> vnums;
        // TODO: add Scripts.
        std::unordered_map<uint8_t, std::unordered_map<std::string, std::any>> variables;
    };

    struct Holder {
        entt::entity entity = entt::null;
        std::optional<std::string> eq_slot;
    };

    struct Contents {
        std::vector<entt::entity> inventory;
        std::unordered_map<std::string, entt::entity> equipment;
    };


    // Serialize/Deserialize Routines
    entt::entity deserialize(const nlohmann::json &j);


    nlohmann::json serialize(entt::entity ent);

}






#endif //SHENRON_INCLUDE_SHENRON_ECS_H
