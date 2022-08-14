//
// Created by volund on 8/12/22.
//

#include "shenron/ecs.h"

namespace shenron::ecs {
    entt::registry reg;


    entt::entity deserialize(const nlohmann::json &j) {
        auto ent = reg.create();

        if(j.contains("PlayerCharacter")) {
            auto &pc = reg.get_or_emplace<PlayerCharacter>(ent, j["PlayerCharacter"]);
            reg.get_or_emplace<Character>(ent);
        }

        if(j.contains("LegacyRoom")) {
            auto &l = reg.get_or_emplace<LegacyRoom>(ent, j["LegacyRoom"]);
            reg.get_or_emplace<Room>(ent);
        }

        if(j.contains("LegacyObj")) {
            auto &l = reg.get_or_emplace<LegacyObj>(ent, j["LegacyObj"]);
            reg.get_or_emplace<Item>(ent);
        }

        if(j.contains("LegacyMob")) {
            auto &l = reg.get_or_emplace<LegacyMob>(ent, j["LegacyMob"]);
            reg.get_or_emplace<Character>(ent);
        }

        if(j.contains("NPC")) reg.get_or_emplace<NPC>(ent);
        if(j.contains("Item")) reg.get_or_emplace<Item>(ent);
        if(j.contains("Room")) reg.get_or_emplace<Room>(ent);
        if(j.contains("Character")) reg.get_or_emplace<Character>(ent);


        return ent;
    }

    nlohmann::json serialize(entt::entity ent) {
        nlohmann::json j;
        if(auto c = reg.try_get<PlayerCharacter>(ent)) j["PlayerCharacter"] = c->player_id;
        if(auto c = reg.try_get<LegacyRoom>(ent)) j["LegacyRoom"] = c->vnum;
        if(auto c = reg.try_get<LegacyObj>(ent)) j["LegacyObj"] = c->vnum;
        if(auto c = reg.try_get<LegacyMob>(ent)) j["LegacyMob"] = c->vnum;

        if(reg.all_of<NPC>(ent)) j["NPC"] = true;
        if(reg.all_of<Item>(ent)) j["item"] = true;
        if(reg.all_of<Character>(ent)) j["Character"] = true;
        if(reg.all_of<Room>(ent)) j["Room"] = true;



        return j;
    }

}
