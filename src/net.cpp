//
// Created by volund on 8/12/22.
//

#include "shenron/net.h"
#include "shenron/ecs.h"
#include <fcntl.h>
#include <arpa/inet.h>
#include <set>
#include <iostream>
#include "fmt/format.h"
#include "shenron/telnet.h"
#include "async++.h"


namespace shenron::net {
    using namespace shenron::ecs;
    int epoll_fd = -1;
    std::unordered_map<int, entt::entity> sock_map;
    std::unordered_map<int, boost::asio::streambuf> in_buffers, out_buffers;
    std::vector<epoll_event> events;

    std::optional<int> create_v4_listen(std::string &addr, uint16_t port) {
        sockaddr_in sa{};
        if(!inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr))) return {};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if(sock < 0) return {};
        if(bind(sock, (sockaddr*)&sa, sizeof(sa)) < 0) {
            close(sock);
            return {};
        }
        return sock;

    }

    std::optional<int> create_v6_listen(std::string &addr, uint16_t port) {
        sockaddr_in6 sa{};
        if(!inet_pton(AF_INET6, addr.c_str(), &(sa.sin6_addr))) return {};
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(port);
        int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if(sock < 0) return {};
        if(bind(sock, (sockaddr*)&sa, sizeof(sa)) < 0) {
            close(sock);
            return {};
        }
        return sock;
    }

    void create_listener(std::string &addr, uint16_t port, GameProtocol gprot) {
        int version = AF_INET6;
        auto socket = create_v6_listen(addr, port);
        if(!socket) {
            version = AF_INET;
            socket = create_v4_listen(addr, port);
        }
        if(!socket) return;

        auto sock = socket.value();
        auto l = listen(sock, 20);

        auto conn_ent = reg.create();
        epoll_event ev{};
        ev.data.fd = sock;
        ev.events = EPOLLIN | EPOLLOUT;
        sock_map[sock] = conn_ent;
        events.reserve(sock_map.size());
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

        auto &li = reg.get_or_emplace<Listener>(conn_ent, sock, version, gprot, TCP);
    }


    void poll() {
        auto count = epoll_wait(epoll_fd, events.data(), events.capacity(), 0);
        if(count < 1) return;
        for(std::size_t i = 0; i < count; i++) {
            auto &ev = events[i];
            auto ent = sock_map[ev.data.fd];

            if(ev.events & EPOLLIN) {
                reg.emplace_or_replace<ReadReady>(ent);
            }
            if(ev.events & EPOLLOUT) {
                reg.emplace_or_replace<WriteReady>(ent);
            }
        }
    }

    void start_game_connections() {
        auto view = reg.view<GameConnection, NewGameConn>();

        // TODO: Start initial parser.

    }

    void send_outgoing_data() {
        auto view = reg.view<Connection, WriteReady, OutDataReady>();

        for(auto ent : view) {
            auto &conn = view.get<Connection>(ent);
            auto &obuf = out_buffers[conn.socket];

            while(true) {
                if(!obuf.size()) {
                    break;
                }
                auto res = write(conn.socket, obuf.data().data(), obuf.size());
                if (res < 0) {
                    if((errno = EWOULDBLOCK) || ((errno == EAGAIN))) {
                        reg.remove<WriteReady>(ent);
                    } else {
                        // this is an actual error. handle properly!
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn.socket, nullptr);
                        reg.emplace<ErrorClose>(ent, errno);
                    }
                    break;
                } else {
                    obuf.consume(res);
                }
            }
            if(obuf.size() == 0) {
                reg.remove<OutDataReady>(ent);
            }

        }

    }

    void accept_pending_connections() {

        auto view = reg.view<Listener, ReadReady>();

        for(auto ent : view) {
            auto li = view.get<Listener>(ent);
            reg.remove<ReadReady>(ent);

            while(true) {
                auto res = accept4(li.socket, nullptr, nullptr, SOCK_NONBLOCK);
                if (res < 0) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) break;
                }
                auto conn_ent = reg.create();
                epoll_event ev{};
                ev.data.fd = res;
                ev.events = EPOLLIN | EPOLLOUT;
                sock_map[res] = conn_ent;
                events.reserve(sock_map.size());
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, res, &ev);

                auto &conn = reg.get_or_emplace<Connection>(conn_ent, res);
                //reg.emplace_or_replace<WriteReady>(conn_ent); // Not truly polled, but ready by default
                switch(li.gprot) {
                    case GameProtocol::Telnet:
                        reg.emplace<telnet::Telnet>(conn_ent);
                        reg.emplace_or_replace<telnet::TelnetStart>(conn_ent);
                        break;
                    default:
                        break;
                }
            }
        }

    }

    void setup() {
        epoll_fd = epoll_create(1);
    }

    void restore(nlohmann::json &j) {

    }

    void run_net_incoming() {
        poll();
        accept_pending_connections();
        read_incoming_data();
        telnet::handle_telnet_start();
        telnet::handle_telnet_pending();
        telnet::read_incoming_telnet();

    }

    void run_net_outgoing() {
        telnet::write_outgoing_telnet();
        send_outgoing_data();
    }

    void read_incoming_data() {
        auto view = reg.view<Connection, ReadReady>();

        async::parallel_for(view, [&](entt::entity ent) {
            auto &con = view.get<Connection>(ent);
            auto &r_buf = in_buffers[con.socket];
            std::size_t bytes_received = 0;

            while(true) {
                auto prep = r_buf.prepare(1024);
                auto res = read(con.socket, prep.data(), 1024);
                if(res < 0) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        reg.remove<ReadReady>(ent);
                    } else {
                        std::cout << "errno on " << con.socket << " is " << errno << std::endl;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, con.socket, nullptr);
                        reg.emplace<ErrorClose>(ent, errno);
                    }
                    break;
                } else {
                    std::cout << "socket " << con.socket << " got " << res << " bytes!" << std::endl;
                    r_buf.commit(res);
                    bytes_received += res;
                }
            }
            if(bytes_received > 0) {
                reg.get_or_emplace<InDataReady>(ent);
            }
        });
    }

    void Connection::write(void *data, std::size_t len) const {
        auto &obuf = out_buffers[socket];
        auto prep = obuf.prepare(len);
        memcpy(prep.data(), data, len);
        obuf.commit(len);
    }

    void Connection::write(const std::vector<uint8_t> &data) const {
        write((void*)data.data(), data.size());
    }

}