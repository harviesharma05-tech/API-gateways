// Mock backend server.   ./backend --port 8081 --name Server-1 [--delay 0]
// GET /health            -> {"status":"healthy"}
// GET /set_delay?s=1.5   -> makes this server slow (for the demo), s=0 resets
// GET anything else      -> {"message":"Hello from <name>. ..."} (after the delay)
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "net.hpp"

static std::atomic<double> g_delay{0.0};
static std::string g_name = "Server";
static int g_port = 8080;

static void reply(int fd, const std::string& body) {
    std::string r = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
                    std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
    send_all(fd, r.data(), r.size());
}

static void serve(int fd) {
    set_io_timeout(fd, 5000);
    std::string head;
    Req rq;
    if (read_head(fd, head) && parse_request(head, rq)) {
        if (rq.path == "/health") {
            reply(fd, "{\"status\":\"healthy\"}");
        } else if (rq.path.rfind("/set_delay", 0) == 0) {
            auto p = rq.path.find("s=");
            g_delay = p == std::string_view::npos ? 0.0 : std::atof(std::string(rq.path.substr(p + 2)).c_str());
            reply(fd, "{\"delay\":" + std::to_string(g_delay.load()) + "}");
        } else {
            double d = g_delay.load();
            if (d > 0) std::this_thread::sleep_for(std::chrono::duration<double>(d));
            reply(fd, "{\"message\":\"Hello from " + g_name + ". This server is working.\",\"port\":" +
                      std::to_string(g_port) + ",\"path\":\"" + std::string(rq.path) + "\"}");
        }
    }
    close(fd);
}

int main(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string k = argv[i];
        if (k == "--port") g_port = std::atoi(argv[i + 1]);
        else if (k == "--name") g_name = argv[i + 1];
        else if (k == "--delay") g_delay = std::atof(argv[i + 1]);
    }
    signal(SIGPIPE, SIG_IGN);
    int lfd = make_listener(g_port);
    if (lfd < 0) return 1;
    std::printf("%s listening on 0.0.0.0:%d (delay=%.2fs)\n", g_name.c_str(), g_port, g_delay.load());
    std::fflush(stdout);
    while (true) {
        int cfd = accept(lfd, nullptr, nullptr);
        if (cfd < 0) { if (errno == EINTR) continue; break; }
        std::thread(serve, cfd).detach();
    }
    return 0;
}
