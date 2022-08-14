//
// Created by volund on 8/12/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_API_H
#define SHENRON_INCLUDE_SHENRON_API_H

#include <string>
#include <vector>
#include <optional>
#include "entt/entt.hpp"
#include "shenron/ecs.h"


namespace shenron::api {
    void addToInventory(entt::entity ent, entt::entity dest);
    void addToInventory(const std::vector<entt::entity>& ents, entt::entity dest);
    void removeFromInventory(entt::entity ent);

    void equip(entt::entity ent, const std::string &slot);
    void unequip(entt::entity ent);

    void addToRoom(entt::entity ent, entt::entity dest);
    void addToRoom(const std::vector<entt::entity>& ents, entt::entity dest);
    void removeFromRoom(entt::entity ent);

    void addToSpace(entt::entity ent, entt::entity dest, ecs::SpaceCoordinates coor = {0, 0, 0});
    void removeFromSpace(entt::entity ent);

    std::vector<entt::entity> dumpInventory(entt::entity ent);
    std::vector<entt::entity> dumpEquipment(entt::entity ent);
    std::vector<entt::entity> dumpRoom(entt::entity ent);
    //std::vector<entt::entity> dumpSpace(entt::entity ent);

    template<typename F>
    const std::vector<entt::entity> filterByComponent(const std::vector<entt::entity>& to_filter) {
        std::vector<entt::entity> out;
        std::copy_if(to_filter.begin(), to_filter.end(), out.begin(), [&](auto &x){
            return shenron::ecs::reg.all_of<F>(x);});
        return out;
    }

    template<typename F>
    bool checkFlag(entt::entity ent, int flag) {
        if(auto f = shenron::ecs::reg.try_get<F>(ent)) {
            return f->flags.test(flag);
        }
        return false;
    }

    template<typename F>
    void setFlag(entt::entity ent, int flag, bool value = true) {
        auto &f = shenron::ecs::reg.get_or_emplace<F>(ent);
        f.flags.set(flag, value);
    }

    template<typename F>
    void toggleFlag(entt::entity ent, int flag, bool value = true) {
        auto &f = shenron::ecs::reg.get_or_emplace<F>(ent);
        f.flags.toggle(flag);
    }

    std::string get_display_name(entt::entity viewer, entt::entity target, bool plain = false);

    std::string get_build_details(entt::entity viewer, entt::entity target);

    std::string get_room_pose(entt::entity viewer, entt::entity target);

    bool can_see(entt::entity viewer, entt::entity target);



}

#endif //SHENRON_INCLUDE_SHENRON_API_H
