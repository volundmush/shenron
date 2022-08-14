//
// Created by volund on 8/12/22.
//

#include "shenron/api.h"
#include "fmt/format.h"

namespace shenron::api {
    using namespace shenron::ecs;

    void addToInventory(entt::entity ent, entt::entity dest) {
        auto &c = reg.get_or_emplace<Contents>(dest);
        c.inventory.push_back(ent);
        reg.emplace_or_replace<Holder>(ent, dest);
    }

    void addToInventory(const std::vector<entt::entity>& ents, entt::entity dest) {
        auto &c = reg.get_or_emplace<Contents>(dest);
        c.inventory.insert(c.inventory.end(), ents.begin(), ents.end());
        reg.insert(ents.begin(), ents.end(), Holder{dest});
    }

    void removeFromInventory(entt::entity ent) {
        if(auto i = reg.try_get<Holder>(ent)) {
            auto &c = reg.get_or_emplace<Contents>(i->entity);
            c.inventory.erase(std::find(c.inventory.begin(), c.inventory.end(), ent));
            if(i->eq_slot) {
                c.equipment.erase(i->eq_slot.value());
            }
            if(c.inventory.empty()) reg.remove<Contents>(i->entity);
            reg.remove<Holder>(ent);
        }
    }

    void equip(entt::entity ent, const std::string &slot) {
        if(auto i = reg.try_get<Holder>(ent)) {
            auto &e = reg.get_or_emplace<Contents>(i->entity);
            e.equipment[slot] = ent;
            i->eq_slot = slot;
        }
    }

    void unequip(entt::entity ent) {
        if(auto i = reg.try_get<Holder>(ent)) {
            auto &e = reg.get<Contents>(i->entity);
            e.equipment.erase(i->eq_slot.value());
            i->eq_slot.reset();
        }
    }

    std::string get_display_name(entt::entity viewer, entt::entity target, bool plain) {
        if(auto n = reg.try_get<Name>(target)) {
            if(plain) return n->plain;
            return n->color;
        }
        return fmt::format("Unknown Entity {}", (uint32_t)target);
    }

    std::string get_build_details(entt::entity viewer, entt::entity target) {
        return fmt::format("@g[ENT: {}]@n", (uint32_t)target);
    }

    std::string get_room_pose(entt::entity viewer, entt::entity target) {
        if(auto r = reg.try_get<RoomDescription>(target)) {
            return r->color;
        }
        return fmt::format("{} is here.", get_display_name(viewer, target));
    }

    bool can_see(entt::entity viewer, entt::entity target) {
        return true;
    }



}