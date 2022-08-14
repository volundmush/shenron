//
// Created by volund on 8/13/22.
//

#include "shenron/telnet.h"
#include "shenron/net.h"
#include "shenron/ecs.h"
#include "shenron/utils.h"
#include "boost/algorithm/string.hpp"


namespace shenron::telnet {
    using namespace shenron::ecs;
    using namespace shenron::net;
    namespace codes {

        const uint8_t NUL = 0, BEL = 7, CR = 13, LF = 10, SGA = 3, TELOPT_EOR = 25, NAWS = 31;
        const uint8_t LINEMODE = 34, EOR = 239, SE = 240, NOP = 241, GA = 249, SB = 250;
        const uint8_t WILL = 251, WONT = 252, DO = 253, DONT = 254, IAC = 255, MNES = 39;
        const uint8_t MXP = 91, MSSP = 70, MCCP2 = 86, MCCP3 = 87, GMCP = 201, MSDP = 69;
        const uint8_t MTTS = 24;
    }

    std::unordered_map<uint8_t, TelnetOpCompatible> tel_compat = {
            {codes::LINEMODE, {.remote =  true}},
            {codes::SGA, {.local = true}},
            {codes::NAWS, {.local = true}},
            {codes::MTTS, {.remote = true}},
            //{codes::MCCP2, {.local = true}},
            {codes::MSSP, {.local = true}}
    };

    TelnetMessage::TelnetMessage(TelnetMsgType m_type) {
        msg_type = m_type;
    }

    std::optional<TelnetMessage> parse_message(boost::asio::streambuf &buf) {
        using namespace shenron::telnet::codes;
        // return early if nothing to do.
        auto available = buf.size();
        if(!available) return {};

        // So we do have some data?
        auto box = buf.data();
        auto begin = boost::asio::buffers_begin(box), end = boost::asio::buffers_end(box);
        std::optional<TelnetMessage> response;
        bool escaped = false, match1 = false, match2 = false;

        // first, we read ahead
        if((uint8_t)*begin == IAC) {
            // If it begins with an IAC, then it's a Command, Negotiation, or Subnegotiation
            if(available < 2) {
                return {}; // not enough bytes available - do nothing;
            }
            // we have 2 or more bytes!
            auto b = begin;
            b++;
            auto sub = b;
            uint8_t option = 0;

            switch((uint8_t)*b) {
                case WILL:
                case WONT:
                case DO:
                case DONT:
                    // This is a negotiation.
                    if(available < 3) return {}; // negotiations require at least 3 bytes.
                    response.emplace(TelnetMsgType::Negotiation);
                    response.value().codes[0] = *b;
                    response.value().codes[1] = *(++b);
                    buf.consume(3);
                    return response;
                case SB:
                    // This is a subnegotiation. We need at least 5 bytes for it to work.
                    if(available < 5) return {};

                    option = *(++b);
                    b++;
                    sub = b;
                    // we must seek ahead until we have an unescaped IAC SE. If we don't have one, do nothing.

                    while(b != end) {
                        if(escaped) {
                            escaped = false;
                            b++;
                            continue;
                        }
                        if((uint8_t)*b == IAC) {
                            b++;
                            if(b != end && (uint8_t)*b == SE) {
                                // we have a winner!
                                response.emplace(TelnetMsgType::Subnegotiation);
                                response.value().codes[0] = option;
                                b--;
                                auto &vec = response.value().data;
                                std::copy(sub, b, std::back_inserter(vec));
                                buf.consume(5 + vec.size());
                                return response;
                            } else {
                                escaped = true;
                                b--;
                                continue;
                            }

                        } else {
                            b++;
                        }
                    }
                    // if we finished the while loop, we don't have enough data, so...
                    return {};
                default:
                    // if it's any other kind of IAC, it's a Command.
                    response.emplace(TelnetMsgType::Command);
                    response.value().data.push_back((uint8_t)*(++b));
                    buf.consume(2);
                    return response;
            };
        } else {
            // Data begins on something that isn't an IAC. Scan ahead until we reach one...
            // Send all data up to an IAC, or everything if there is no IAC, as data.
            response.emplace(TelnetMsgType::AppData);
            auto check = std::find(begin, end, IAC);
            auto &vec = response.value().data;
            std::copy(begin, check, std::back_inserter(vec));
            buf.consume(vec.size());
            return response;
        }
    }

    void handle_telnet_start() {
        auto view = reg.view<Connection, Telnet, TelnetStart>();

        for(auto ent : view) {
            auto &tel = view.get<Telnet>(ent);
            std::vector<uint8_t> out;
            for(auto &o : tel_compat) {
                if(o.second.local) {
                    out.push_back(codes::IAC);
                    out.push_back(codes::WILL);
                    out.push_back(o.first);
                    auto &state = tel.op_states[o.first];
                    state.local.negotiating = true;
                }
                if(o.second.remote) {
                    out.push_back(codes::IAC);
                    out.push_back(codes::DO);
                    out.push_back(o.first);
                    auto &state = tel.op_states[o.first];
                    state.remote.negotiating = true;
                }
            }
            if(!out.empty()) telnet_send(ent, out);
            reg.remove<TelnetStart>(ent);
            auto &t = reg.get_or_emplace<TelnetPending>(ent, shenron::utils::Timer(300L));
            t.timer.start();
            reg.get_or_emplace<OutDataReady>(ent);

        }
    }

    void handle_telnet_pending() {
        auto view = reg.view<TelnetPending>();

        for(auto ent : view) {
            auto &tp = view.get<TelnetPending>(ent);
            if(tp.timer.expired()) {
                reg.remove<TelnetPending>(ent);
                auto &gr = reg.get_or_emplace<GameConnection>(ent);
                reg.get_or_emplace<NewGameConn>(ent);
            }
        }
    }

    void read_incoming_telnet() {
        auto view = reg.view<Connection, Telnet, InDataReady>();

        for(auto ent : view) {
            auto &conn = view.get<Connection>(ent);
            auto &tel = view.get<Telnet>(ent);
            auto &ibuf = in_buffers[conn.socket];


            while(auto msg_maybe = parse_message(ibuf)) {
                process_telnet_msg(ent, msg_maybe.value());
            }

            reg.remove<InDataReady>(ent);

        }
    }

    void telnet_send(entt::entity ent, std::vector<uint8_t> &out) {
        auto &conn = reg.get<Connection>(ent);
        conn.write(out);

    }

    void telnet_appdata(entt::entity ent, TelnetMessage &msg) {
        auto &tel = reg.get<Telnet>(ent);
        std::copy(msg.data.begin(), msg.data.end(), std::back_inserter(tel.app_data));
        auto &gc = reg.get_or_emplace<GameConnection>(ent);

        while(true) {
            auto find = tel.app_data.find("\r\n");
            if(find != std::string::npos) {
                gc.in_lines.push_back(tel.app_data.substr(0, find-1));
                tel.app_data = tel.app_data.substr(find+1, tel.app_data.size());
            } else {
                break;
            }
        }
        if(!gc.in_lines.empty()) reg.get_or_emplace<InGameDataReady>(ent);
    }

    void telnet_sendsub(entt::entity ent, uint8_t op, std::vector<uint8_t> &data) {
        using namespace codes;
        std::vector<uint8_t> out({IAC, SB, op});
        std::copy(data.begin(), data.end(), std::back_inserter(out));
        out.push_back(IAC);
        out.push_back(SE);
        telnet_send(ent, out);
    }

    void telnet_sub_mtts(entt::entity ent, const TelnetMessage &msg) {
        if(msg.data.empty()) return; // we need data to be useful.
        if(msg.data[0] != 0) return; // this is invalid MTTS.
        if(msg.data.size() < 2) return; // we need at least some decent amount of data to be useful.

        std::string mtts = boost::algorithm::to_upper_copy(std::string(msg.data.begin(), msg.data.end()).substr(1));

        auto &tel = reg.get<Telnet>(ent);


        if(mtts == tel.mtts_last) // there is no more data to be gleaned from asking...
            return;

        switch(tel.mtts_count) {
            case 0:
                telnet_sub_mtts_0(ent, mtts);
                break;
            case 1:
                telnet_sub_mtts_1(ent, mtts);
                break;
            case 2:
                telnet_sub_mtts_2(ent, mtts);
                break;
        }

        tel.mtts_count++;
        // cache the results and request more info.
        tel.mtts_last = mtts;
        if(tel.mtts_count >= 2) return; // there is no more info to request.
        std::vector<uint8_t> s({1});
        telnet_sendsub(ent, codes::MTTS, s);

    }

    void telnet_sub_mtts_0(entt::entity ent, const std::string& mtts) {
        auto &gc = reg.get_or_emplace<GameConnection>(ent);

        std::vector<std::string> namecheck;
        auto to_check = boost::algorithm::to_upper_copy(mtts);
        boost::algorithm::split(namecheck, to_check, boost::algorithm::is_space());
        switch(namecheck.size()) {
            case 2:
                gc.clientVersion = namecheck[1];
            case 1:
                gc.clientName = namecheck[0];
                break;
        }

        auto &name = gc.clientName;
        auto &version = gc.clientVersion;

        if((name == "ATLANTIS") || (name == "CMUD") || (name == "KILDCLIENT") || (name == "MUDLET") ||
           (name == "PUTTY") || (name == "BEIP") || (name == "POTATO") || (name == "TINYFUGUE") || (name == "MUSHCLIENT")) {
            gc.colorType = std::max(gc.colorType, XtermColor);
        }

        // all clients that support MTTS probably support ANSI...
        gc.colorType = std::max(gc.colorType, StandardColor);
    }

    void telnet_sub_mtts_1(entt::entity ent, const std::string& mtts) {
        auto &gc = reg.get_or_emplace<GameConnection>(ent);
        std::vector<std::string> splitcheck;
        auto to_check = boost::algorithm::to_upper_copy(mtts);
        boost::algorithm::split(splitcheck, to_check, boost::algorithm::is_any_of("-"));


        switch(splitcheck.size()) {
            case 2:
                if(splitcheck[1] == "256COLOR") {
                    gc.colorType = std::max(gc.colorType, XtermColor);
                } else if (splitcheck[1] == "TRUECOLOR") {
                    gc.colorType = std::max(gc.colorType, TrueColor);
                }
            case 1:
                if(splitcheck[0] == "ANSI") {
                    gc.colorType = std::max(gc.colorType, StandardColor);
                } else if (splitcheck[0] == "VT100") {
                    gc.colorType = std::max(gc.colorType, StandardColor);
                    gc.vt100 = true;
                } else if(splitcheck[0] == "XTERM") {
                    gc.colorType = std::max(gc.colorType, XtermColor);
                    gc.vt100 = true;
                }
                break;
        }
    }

    void telnet_sub_mtts_2(entt::entity ent, const std::string &mtts) {
        std::vector<std::string> splitcheck;
        auto &gc = reg.get_or_emplace<GameConnection>(ent);
        auto to_check = boost::algorithm::to_upper_copy(mtts);
        boost::algorithm::split(splitcheck, to_check, boost::algorithm::is_space());

        if(splitcheck.size() < 2) return;

        if(splitcheck[0] != "MTTS") return;

        int v = atoi(splitcheck[1].c_str());

        // ANSI
        if(v & 1) {
            gc.colorType = std::max(gc.colorType, StandardColor);
        }

        // VT100
        if(v & 2) {
            gc.vt100 = true;
        }

        // UTF8
        if(v & 4) {
            gc.utf8 = true;
        }

        // XTERM256 colors
        if(v & 8) {
            gc.colorType = std::max(gc.colorType, XtermColor);
        }

        // MOUSE TRACKING - who even uses this?
        if(v & 16) {
            gc.mouse_tracking = true;
        }

        // On-screen color palette - again, is this even used?
        if(v & 32) {
            gc.osc_color_palette = true;
        }

        // client uses a screen reader - this is actually somewhat useful for blind people...
        // if the game is designed for it...
        if(v & 64) {
            gc.screen_reader = true;
        }

        // PROXY - I don't think this actually works?
        if(v & 128) {
            gc.proxy = true;
        }

        // TRUECOLOR - support for this is probably rare...
        if(v & 256) {
            gc.colorType = std::max(gc.colorType, TrueColor);
        }

        // MNES - Mud New Environment Standard support.
        if(v & 512) {
            gc.mnes = true;
        }

        // mud server link protocol ???
        if(v & 1024) {
            gc.mslp = true;
        }

    }

    void telnet_sub_naws(entt::entity ent, TelnetMessage &msg) {

    }

    void telnet_command(entt::entity ent, TelnetMessage &msg) {

    }

    void telnet_subnegotiation(entt::entity ent, TelnetMessage &msg) {
        using namespace codes;
        auto op = msg.codes[0];
        if(!tel_compat.contains(op)) return;
        switch(op) {
            case MTTS:
                telnet_sub_mtts(ent, msg);
                break;
            case NAWS:
                telnet_sub_naws(ent, msg);
                break;
        }
    }

    void telnet_negotiation(entt::entity ent, TelnetMessage &msg) {
        using namespace codes;
        auto code = msg.codes[0];
        auto op = msg.codes[1];
        std::vector<uint8_t> out;
        out.push_back(IAC);
        if((code == WILL) || (code == DO)) {
            // Not supported!
            if(!tel_compat.contains(op)) {
                switch(code) {
                    case WILL:
                        out.push_back(DONT);
                        break;
                    case DO:
                        out.push_back(WONT);
                        break;
                    default:// shouldn't be possible!
                        break;
                }
            } else {
                auto compat = tel_compat[op];
                auto &tel = reg.get<Telnet>(ent);
                auto &state = tel.op_states[op];
                switch(code) {
                    case WILL:
                        if(compat.remote) {
                            state.remote.enabled = true;
                            if(!state.remote.negotiating) out.push_back(DO);
                            telnet_enable_remote(ent, op);
                        } else {
                            out.push_back(DONT);
                        }
                        break;
                    case DO:
                        if(compat.local) {
                            state.local.enabled = true;
                            if(!state.local.negotiating) out.push_back(WILL);
                            telnet_enable_local(ent, op);
                        } else {
                            out.push_back(WONT);
                        }
                        break;
                    case WONT:
                        if(!state.remote.negotiating) out.push_back(DONT);
                        if(state.remote.enabled) telnet_disable_remote(ent, op);
                        state.remote.negotiating = false;
                        state.remote.enabled = false;
                        break;
                    case DONT:
                        if(!state.local.negotiating) out.push_back(WONT);
                        if(state.local.enabled) telnet_disable_local(ent, op);
                        state.local.negotiating = false;
                        state.local.enabled = false;
                        break;
                    default: // shouldn't happen
                        break;
                }
            }
        }

        out.push_back(op);
        if(out.size() == 3) telnet_send(ent, out);
    }

    void process_telnet_msg(entt::entity ent, TelnetMessage &msg) {
        switch(msg.msg_type) {
            case TelnetMsgType::AppData:
                telnet_appdata(ent, msg);
                break;
            case TelnetMsgType::Command:
                telnet_command(ent, msg);
                break;
            case TelnetMsgType::Negotiation:
                telnet_negotiation(ent, msg);
                break;
            case TelnetMsgType::Subnegotiation:
                telnet_subnegotiation(ent, msg);
                break;
        }
    }

    void telnet_enable_remote(entt::entity ent, uint8_t op) {
        std::vector<uint8_t> s({1});
        switch(op) {
            case codes::MTTS:
                telnet_sendsub(ent, op, s);
                break;
        }
    }

    void telnet_disable_remote(entt::entity ent, uint8_t op) {

    }

    void telnet_enable_local(entt::entity ent, uint8_t op) {

    }

    void telnet_disable_local(entt::entity ent, uint8_t op) {

    }

    void telnet_sendtext(entt::entity ent, const std::string &txt) {
        if(txt.empty()) return;
        std::vector<uint8_t> data;

        // standardize outgoing linebreaks for telnet.
        for(const auto &c : txt) {
            switch(c) {
                case '\r':
                    break;
                case '\n':
                    data.push_back('\r');
                    data.push_back('\n');
                    break;
                default:
                    data.push_back(c);
                    break;
            }
        }

        telnet_send(ent, data);
    }

    void write_outgoing_telnet() {
        auto view = reg.view<Connection, GameConnection, OutGameDataReady, Telnet>();

        for(auto ent : view) {
            auto &tel = view.get<Telnet>(ent);
            auto &gc = view.get<GameConnection>(ent);

            for(auto &ol : gc.out_lines) {
                auto &t_type = std::get<0>(ol);
                auto &t_data = std::get<1>(ol);

                std::string out;

                switch(t_type) {
                    case net::Line:
                        if(t_data.ends_with("\r\n")) {
                            out = t_data;
                        } else {
                            out = t_data + "\r\n";
                        };
                        break;
                    case net::Text:
                        out = t_data;
                        break;
                    default:
                        break;
                }
                if(!out.empty()) {

                }
            }

        }
    }

}