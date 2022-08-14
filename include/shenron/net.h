//
// Created by volund on 8/12/22.
//

#ifndef SHENRON_INCLUDE_SHENRON_NET_H
#define SHENRON_INCLUDE_SHENRON_NET_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include "entt/entt.hpp"
#include "boost/asio.hpp"
#include "nlohmann/json.hpp"

namespace shenron::net {

    extern int epoll_fd;
    extern std::unordered_map<int, entt::entity> sock_map;
    extern std::unordered_map<int, boost::asio::streambuf> in_buffers, out_buffers;

    extern std::vector<epoll_event> events;

    enum GameProtocol : uint8_t {
        Telnet = 0,
        WebSocket = 1
    };

    enum Encryption : uint8_t {
        TCP = 0,
        TLS = 1
    };

    struct ConnStart {
        GameProtocol prot = Telnet;
    };

    struct Connection {
        int socket = -1;
        void write(void *data, std::size_t len) const;
        void write(const std::vector<uint8_t> &data) const;
    };

    struct Listener {
        int socket = -1;
        int inet_prot = AF_INET;
        GameProtocol gprot = Telnet;
        Encryption encryption = TCP;
    };

    struct ReadReady {};
    struct WriteReady {};
    struct InDataReady {};
    struct OutDataReady {};
    struct OutGameDataReady {};
    struct InGameDataReady {};

    struct ErrorClose {
        int err = -1;
    };

    enum ColorType : uint8_t {
        NoColor = 0,
        StandardColor = 1,
        XtermColor = 2,
        TrueColor = 3
    };

    enum TextType : uint8_t {
        Text = 0,
        Line = 1,
        Prompt = 2
    };

    enum MsgType : uint8_t {
        Command = 0,
        GMCP = 1,
        MSSP = 2
    };


    struct ActiveGameConn{};
    struct NewGameConn{};

    struct GameConnection {
        std::list<std::string> in_lines;
        std::list<std::tuple<TextType, std::string>> out_lines;

        GameProtocol gameProtocol = Telnet;
        Encryption encryption = TCP;
        ColorType colorType = NoColor;
        std::string clientName = "UNKNOWN", clientVersion = "UNKNOWN";
        std::string hostIp = "UNKNOWN", hostName = "UNKNOWN";
        int width = 78, height = 24;
        bool utf8 = false, screen_reader = false, proxy = false, osc_color_palette = false;
        bool vt100 = false, mouse_tracking = false, naws = false, msdp = false, gmcp = false;
        bool mccp2 = false, mccp2_active = false, mccp3 = false, mccp3_active = false, telopt_eor = false;
        bool mtts = false, ttype = false, mnes = false, suppress_ga = false, mslp = false;
        bool force_endline = false, linemode = false, mssp = false, mxp = false, mxp_active = false;
    };

    std::optional<int> create_v4_listen(std::string &addr, uint16_t port);
    std::optional<int> create_v6_listen(std::string &addr, uint16_t port);

    void setup();
    void restore(nlohmann::json &j);

    void create_listener(std::string &addr, uint16_t port, GameProtocol gprot);

    void start_game_connections();

    void accept_pending_connections();

    void run_net_incoming();
    void run_net_outgoing();

    void read_incoming_data();

    void send_outgoing_data();

}




#endif //SHENRON_INCLUDE_SHENRON_NET_H
