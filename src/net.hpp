// Small socket helpers shared by the gateway and the backend (C++17, POSIX sockets).
#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <string>
#include <string_view>

inline int make_listener(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(static_cast<uint16_t>(port));
    if (bind(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) < 0 || listen(fd, 1024) < 0) {
        perror("bind/listen"); close(fd); return -1;
    }
    return fd;
}

inline void set_io_timeout(int fd, int ms) {
    timeval tv{ms / 1000, (ms % 1000) * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
}

// Connect with a timeout. Returns a blocking socket with I/O timeout set, or -1.
inline int connect_to(const std::string& host, int port, int connect_ms, int io_ms) {
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) return -1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { freeaddrinfo(res); return -1; }
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    int rc = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc < 0 && errno == EINPROGRESS) {
        pollfd p{fd, POLLOUT, 0};
        if (poll(&p, 1, connect_ms) == 1) {
            int err = 0; socklen_t l = sizeof err;
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &l);
            rc = err == 0 ? 0 : -1;
        } else rc = -1;
    }
    if (rc < 0) { close(fd); return -1; }
    fcntl(fd, F_SETFL, flags);
    set_io_timeout(fd, io_ms);
    return fd;
}

inline bool send_all(int fd, const char* p, size_t n) {
    while (n) {
        ssize_t w = send(fd, p, n, MSG_NOSIGNAL);
        if (w <= 0) return false;
        p += w; n -= static_cast<size_t>(w);
    }
    return true;
}

// Reads until the end of the HTTP header block ("\r\n\r\n").
inline bool read_head(int fd, std::string& buf) {
    char tmp[2048];
    while (buf.find("\r\n\r\n") == std::string::npos) {
        if (buf.size() > 16384) return false;
        ssize_t r = recv(fd, tmp, sizeof tmp, 0);
        if (r <= 0) return false;
        buf.append(tmp, static_cast<size_t>(r));
    }
    return true;
}

// Zero-copy request-line parser: the returned views point into the original buffer.
struct Req { std::string_view method, path; };
inline bool parse_request(std::string_view head, Req& r) {
    auto e = head.find("\r\n");
    if (e == std::string_view::npos) return false;
    auto line = head.substr(0, e);
    auto s1 = line.find(' ');
    if (s1 == std::string_view::npos) return false;
    auto s2 = line.find(' ', s1 + 1);
    if (s2 == std::string_view::npos) return false;
    r.method = line.substr(0, s1);
    r.path = line.substr(s1 + 1, s2 - s1 - 1);
    return !r.method.empty() && !r.path.empty();
}
