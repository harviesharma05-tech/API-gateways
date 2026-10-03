// C++ HTTP API Gateway with dynamic load balancing.
//   ./gateway --port 10000 --backends 127.0.0.1:8081 127.0.0.1:8082 127.0.0.1:8083
//             [--threads 16] [--rate 50] [--burst 100] [--queue 1024]
//
// Request path:  accept -> task queue (C linked list) -> worker thread pool
//                -> per-client token bucket (C hash map) -> Power-of-Two-Choices
//                   with score = (active + 1) x EWMA latency -> forward -> relay response
// Background:    health-check thread (removes dead nodes, brings them back on recovery)
//
// C data structures (dsa/): DynArray (node list), Queue (task queue), HashMap (rate limiter),
//                           MinHeap (stats ranking), TokenBucket.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include "net.hpp"
#include "dynarray.h"
#include "queue.h"
#include "hashmap.h"
#include "heap.h"
#include "tokenbucket.h"

using Clock = std::chrono::steady_clock;
static int64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}
static double now_sec() { return static_cast<double>(now_ns()) / 1e9; }

// ------------------------------------------------------------------ backend node
class Node {
public:
    std::string host;
    int port;
    std::string addr;
    size_t idx = 0;
    std::atomic<size_t> active{0};
    std::atomic<double> ewma_ms{1.0};
    std::atomic<bool> healthy{true};
    std::atomic<size_t> served{0};
    static constexpr double TAU_MS = 1000.0;

    Node(std::string h, int p) : host(std::move(h)), port(p), addr(host + ":" + std::to_string(port)), last_ns_(now_ns()) {}

    // Lower score = better. EWMA decays while the node is idle, so a recovered node is tried again.
    double score() const {
        double idle_ms = static_cast<double>(now_ns() - last_ns_.load(std::memory_order_relaxed)) / 1e6;
        double eff = std::max(ewma_ms.load(std::memory_order_relaxed) * std::exp(-idle_ms / TAU_MS), 1.0);
        return eff * static_cast<double>(active.load(std::memory_order_relaxed) + 1);
    }
    void update_latency(double rtt_ms) {
        std::lock_guard<std::mutex> g(m_);
        int64_t now = now_ns();
        double dt_ms = static_cast<double>(now - last_ns_.load()) / 1e6;
        last_ns_.store(now);
        double alpha = std::clamp(1.0 - std::exp(-dt_ms / TAU_MS), 0.01, 1.0);
        ewma_ms.store(alpha * rtt_ms + (1.0 - alpha) * ewma_ms.load(), std::memory_order_relaxed);
    }
private:
    std::mutex m_;
    std::atomic<int64_t> last_ns_;
};

static DynArray g_nodes;                 // C dynamic array of Node*
static std::atomic<bool> g_run{true};

// Power of Two Choices: pick two distinct healthy nodes at random, keep the lower score.
// `tried` is a bitmask of nodes already attempted for this request (used for failover).
static Node* pick(uint64_t tried) {
    Node* alive[64];
    size_t n = 0;
    for (size_t i = 0; i < da_size(&g_nodes) && n < 64; i++) {
        Node* x = static_cast<Node*>(da_get(&g_nodes, i));
        if (x->healthy.load() && !((tried >> x->idx) & 1ULL)) alive[n++] = x;
    }
    if (n == 0) return nullptr;
    if (n == 1) return alive[0];
    thread_local std::mt19937 rng{std::random_device{}()};
    size_t i = rng() % n, j = rng() % (n - 1);
    if (j >= i) ++j;                                  // two DISTINCT candidates
    return alive[i]->score() <= alive[j]->score() ? alive[i] : alive[j];
}

// ------------------------------------------------------------------ rate limiter (C hash map + token bucket)
class RateLimiter {
public:
    RateLimiter(double rate, double burst) : rate_(rate), burst_(burst) { hm_init(&map_, 64); }
    ~RateLimiter() { hm_free(&map_, free); }
    bool allow(const std::string& ip) {
        std::lock_guard<std::mutex> g(m_);
        double now = now_sec();
        auto* tb = static_cast<TokenBucket*>(hm_get(&map_, ip.c_str()));
        if (!tb) {
            if (hm_size(&map_) > 10000) hm_clear(&map_, free);     // simple memory cap
            tb = static_cast<TokenBucket*>(malloc(sizeof(TokenBucket)));
            tb_init(tb, burst_, rate_, now);
            hm_put(&map_, ip.c_str(), tb);
        }
        return tb_allow(tb, now) == 1;
    }
private:
    HashMap map_;
    std::mutex m_;
    double rate_, burst_;
};

// ------------------------------------------------------------------ thread pool (C linked-list queue)
struct Task { int fd; char ip[48]; };

class ThreadPool {
public:
    ThreadPool(int threads, size_t max_queue, void (*fn)(Task*)) : max_(max_queue), fn_(fn) {
        q_init(&q_);
        for (int i = 0; i < threads; i++) std::thread([this] { loop(); }).detach();
    }
    bool submit(Task* t) {
        std::lock_guard<std::mutex> g(m_);
        if (q_size(&q_) >= max_) return false;        // back-pressure: queue full
        if (q_push(&q_, t) != 0) return false;
        cv_.notify_one();
        return true;
    }
    size_t pending() { std::lock_guard<std::mutex> g(m_); return q_size(&q_); }
private:
    void loop() {
        while (true) {
            Task* t;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [this] { return q_size(&q_) > 0; });
                t = static_cast<Task*>(q_pop(&q_));
            }
            fn_(t);
            free(t);
        }
    }
    Queue q_;
    std::mutex m_;
    std::condition_variable cv_;
    size_t max_;
    void (*fn_)(Task*);
};

static RateLimiter* g_limiter;
static ThreadPool* g_pool;

// ------------------------------------------------------------------ request handling
static void send_text(int fd, int code, const char* reason, const std::string& body) {
    std::string r = "HTTP/1.1 " + std::to_string(code) + " " + reason +
                    "\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.size()) +
                    "\r\nConnection: close\r\n\r\n" + body;
    send_all(fd, r.data(), r.size());
}

static std::string stats_text() {
    MinHeap h;
    heap_init(&h, da_size(&g_nodes));
    for (size_t i = 0; i < da_size(&g_nodes); i++) {
        Node* n = static_cast<Node*>(da_get(&g_nodes, i));
        heap_push(&h, n->healthy.load() ? n->score() : 1e18, n);   // DOWN nodes rank last
    }
    std::string out = "rank  server              healthy  active  ewma(ms)   score      served\n";
    char line[200];
    int rank = 1;
    double key; void* item;
    while (heap_pop(&h, &key, &item) == 0) {
        (void)key;
        Node* n = static_cast<Node*>(item);
        std::snprintf(line, sizeof line, "%-5d %-19s %-8s %-7zu %-10.1f %-10.1f %zu\n", rank++, n->addr.c_str(),
                      n->healthy.load() ? "UP" : "DOWN", n->active.load(), n->ewma_ms.load(), n->score(), n->served.load());
        out += line;
    }
    heap_free(&h);
    return out;
}

static void proxy(int cfd, const Req& rq) {
    std::string out_req = std::string(rq.method) + " " + std::string(rq.path) +
                          " HTTP/1.1\r\nHost: backend\r\nConnection: close\r\n\r\n";
    uint64_t tried = 0;
    for (int attempt = 0; attempt < 3; ++attempt) {          // failover: up to 3 different nodes
        Node* n = pick(tried);
        if (!n) break;
        tried |= 1ULL << n->idx;
        n->active++;
        int64_t t0 = now_ns();
        bool got_data = false;
        int bfd = connect_to(n->host, n->port, 1000, 10000);
        if (bfd >= 0) {
            if (send_all(bfd, out_req.data(), out_req.size())) {
                char buf[4096];
                ssize_t r;
                while ((r = recv(bfd, buf, sizeof buf, 0)) > 0) {
                    got_data = true;
                    if (!send_all(cfd, buf, static_cast<size_t>(r))) break;
                }
            }
            close(bfd);
        }
        n->active--;
        if (got_data) {
            n->update_latency(static_cast<double>(now_ns() - t0) / 1e6);
            n->served++;
            return;
        }
        n->healthy = false;                                   // connect/IO failed -> try another node
    }
    send_text(cfd, 503, "Service Unavailable", "No healthy backend available\n");
}

static void handle(Task* t) {
    int fd = t->fd;
    set_io_timeout(fd, 5000);
    std::string head;
    Req rq;
    if (!read_head(fd, head) || !parse_request(head, rq)) {
        send_text(fd, 400, "Bad Request", "Bad request\n");
    } else if (rq.path == "/_stats") {
        send_text(fd, 200, "OK", stats_text());
    } else if (!g_limiter->allow(t->ip)) {
        send_text(fd, 429, "Too Many Requests", "Too Many Requests (rate limit)\n");
    } else {
        proxy(fd, rq);
    }
    close(fd);
}

// ------------------------------------------------------------------ health checker
static void health_loop() {
    while (g_run) {
        for (size_t i = 0; i < da_size(&g_nodes); i++) {
            Node* n = static_cast<Node*>(da_get(&g_nodes, i));
            bool ok = false;
            int fd = connect_to(n->host, n->port, 800, 1000);
            if (fd >= 0) {
                const char* rq = "GET /health HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n";
                char buf[64] = {0};
                if (send_all(fd, rq, std::strlen(rq)) && recv(fd, buf, sizeof buf - 1, 0) > 0) ok = std::strstr(buf, " 200") != nullptr;
                close(fd);
            }
            if (ok != n->healthy.load()) std::printf("[health] %s -> %s\n", n->addr.c_str(), ok ? "UP" : "DOWN");
            n->healthy = ok;
        }
        std::fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

static void on_signal(int) { g_run = false; }

int main(int argc, char** argv) {
    int port = 10000, threads = 16;
    double rate = 50, burst = 100;
    size_t qmax = 1024;
    std::vector<std::string> backends;
    for (int i = 1; i < argc; i++) {
        std::string k = argv[i];
        if (k == "--backends") { while (i + 1 < argc && argv[i + 1][0] != '-') backends.push_back(argv[++i]); }
        else if (i + 1 < argc) {
            if (k == "--port") port = std::atoi(argv[++i]);
            else if (k == "--threads") threads = std::atoi(argv[++i]);
            else if (k == "--rate") rate = std::atof(argv[++i]);
            else if (k == "--burst") burst = std::atof(argv[++i]);
            else if (k == "--queue") qmax = static_cast<size_t>(std::atoi(argv[++i]));
        }
    }
    if (backends.empty() || backends.size() > 64) { std::fprintf(stderr, "usage: %s --backends host:port ... [--port N]\n", argv[0]); return 1; }

    da_init(&g_nodes);
    for (auto& b : backends) {
        auto c = b.rfind(':');
        if (c == std::string::npos) { std::fprintf(stderr, "bad backend '%s' (use host:port)\n", b.c_str()); return 1; }
        Node* n = new Node(b.substr(0, c), std::atoi(b.c_str() + c + 1));
        n->idx = da_size(&g_nodes);
        da_push(&g_nodes, n);
    }

    struct sigaction sa{};
    sa.sa_handler = on_signal;                         // no SA_RESTART, so accept() returns on Ctrl-C
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    g_limiter = new RateLimiter(rate, burst);
    g_pool = new ThreadPool(threads, qmax, handle);
    int lfd = make_listener(port);
    if (lfd < 0) return 1;
    std::thread(health_loop).detach();
    std::printf("Gateway on :%d  backends=%zu  threads=%d  rate=%.0f/s burst=%.0f\n", port, backends.size(), threads, rate, burst);
    std::fflush(stdout);

    while (g_run) {
        sockaddr_in ca{};
        socklen_t l = sizeof ca;
        int cfd = accept(lfd, reinterpret_cast<sockaddr*>(&ca), &l);
        if (cfd < 0) { if (errno == EINTR) continue; break; }
        Task* t = static_cast<Task*>(malloc(sizeof(Task)));
        t->fd = cfd;
        inet_ntop(AF_INET, &ca.sin_addr, t->ip, sizeof t->ip);
        if (!g_pool->submit(t)) {                      // overloaded: reject quickly
            send_text(cfd, 503, "Service Unavailable", "Gateway busy\n");
            close(cfd);
            free(t);
        }
    }
    close(lfd);
    std::printf("Gateway stopped.\n");
    _exit(0);
}
