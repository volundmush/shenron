//
// Created by volund on 8/13/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_TELNET_H
#define SHENRON_INCLUDE_SHENRON_TELNET_H

#include <cstdint>
#include <bitset>
#include <unordered_map>
#include <vector>
#include <optional>
#include "boost/asio.hpp"
#include "shenron/utils.h"
#include "entt/entt.hpp"

namespace shenron::telnet {

    namespace codes {
        extern const uint8_t NUL;
        extern const uint8_t BEL;
        extern const uint8_t CR;
        extern const uint8_t LF;
        extern const uint8_t SGA;
        extern const uint8_t TELOPT_EOR;
        extern const uint8_t NAWS;
        extern const uint8_t LINEMODE;
        extern const uint8_t EOR;
        extern const uint8_t SE;
        extern const uint8_t NOP;
        extern const uint8_t GA;
        extern const uint8_t SB;
        extern const uint8_t WILL;
        extern const uint8_t WONT;
        extern const uint8_t DO;
        extern const uint8_t DONT;
        extern const uint8_t IAC;

        extern const uint8_t MNES;
        extern const uint8_t MXP;
        extern const uint8_t MSSP;
        extern const uint8_t MCCP2;
        extern const uint8_t MCCP3;

        extern const uint8_t GMCP;
        extern const uint8_t MSDP;
        extern const uint8_t MTTS;
    }

    struct TelnetOpCompatible {
        bool local = false;
        bool remote = false;
    };

    extern std::unordered_map<uint8_t, TelnetOpCompatible> tel_compat;

    enum TelnetMsgType : uint8_t {
        AppData = 0, // random telnet bytes
        Command = 1, // an IAC <something>
        Negotiation = 2, // an IAC WILL/WONT/DO/DONT
        Subnegotiation = 3 // an IAC SB <code> <data> IAC SE
    };

    struct TelnetMessage {
        explicit TelnetMessage(TelnetMsgType m_type);
        TelnetMsgType msg_type;
        std::vector<uint8_t> data;
        uint8_t codes[2] = {0, 0};
    };

    std::optional<TelnetMessage> parse_message(boost::asio::streambuf &buf);

    struct TelOptState {
        bool negotiating = false;
        bool enabled = false;
    };

    struct TelnetPerspective {
        TelOptState local;
        TelOptState remote;
    };

    enum TelnetState : uint8_t {
        Negotiating = 0,
        Active = 1
    };

    struct Telnet {
        std::unordered_map<uint8_t, TelnetPerspective> op_states;
        std::string mtts_last;
        uint8_t mtts_count;
        TelnetState state = Negotiating;
        std::string app_data;
    };

    struct TelnetStart {};

    struct TelnetPending {
        utils::Timer timer;
    };



    void handle_telnet_start();
    void handle_telnet_pending();
    void read_incoming_telnet();
    void write_outgoing_telnet();
    void telnet_send(entt::entity ent, std::vector<uint8_t> &out);
    void telnet_sendsub(entt::entity ent, uint8_t op, std::vector<uint8_t> &data);
    void telnet_appdata(entt::entity ent, TelnetMessage &msg);
    void telnet_sub_naws(entt::entity ent, TelnetMessage &msg);
    void telnet_sub_mtts(entt::entity ent, const TelnetMessage &msg);
    void telnet_sub_mtts_0(entt::entity ent, const std::string &mtts);
    void telnet_sub_mtts_1(entt::entity ent, const std::string &mtts);
    void telnet_sub_mtts_2(entt::entity ent, const std::string &mtts);
    void telnet_command(entt::entity ent, TelnetMessage &msg);
    void telnet_subnegotiation(entt::entity ent, TelnetMessage &msg);
    void telnet_negotiation(entt::entity ent, TelnetMessage &msg);
    void process_telnet_msg(entt::entity ent, TelnetMessage &msg);
    void telnet_enable_remote(entt::entity ent, uint8_t op);
    void telnet_disable_remote(entt::entity ent, uint8_t op);
    void telnet_enable_local(entt::entity ent, uint8_t op);
    void telnet_disable_local(entt::entity ent, uint8_t op);


}

#endif //SHENRON_INCLUDE_SHENRON_TELNET_H
