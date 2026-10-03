# Dynamic Load Balancer - API Gateway (sirf C++ aur C data structures)
Team DSCPP-III-2026-T035

Koi Python/Envoy nahi. Gateway aur backend dono **C++17** mein hain, aur saari data structures **C** mein (`dsa/`).

## Kaun sa DSA kahan use hua
| Data structure (C, `dsa/`) | Kahan use hua |
|---|---|
| Dynamic array (`dynarray`) | Backend servers ki list |
| Linked-list queue (`queue`) | Thread-pool ki task queue (accept hui connections) |
| Hash table, chaining + FNV-1a (`hashmap`) | Rate limiter: client IP -> token bucket |
| Min-heap (`heap`) | `/_stats` mein servers ko score se rank karna (DOWN sabse last) |
| Token bucket (`tokenbucket`) | Per-client rate limiting (limit se zyada -> HTTP 429) |

## Algorithm (src/gateway.cpp)
1. `accept` -> task queue -> worker thread pool (queue full ho toh 503, back-pressure)
2. Per-client token bucket check (429 agar limit cross)
3. **Power of Two Choices**: 2 alag healthy servers random chuno, jiska score kam woh jeete
4. Score = `(active requests + 1) x EWMA latency` (idle node ka EWMA time ke saath ghatta hai, taaki recover hone par wapas try ho)
5. Request forward, response client ko relay. Fail ho toh agle node pe failover (max 3)
6. Background health check har 2 sec (`/health`): dead node nikalta hai, wapas aane par add karta hai
7. HTTP request line ka parsing `std::string_view` se (zero-copy)

## Build aur run (Linux / WSL / Mac; gcc, g++, make, curl chahiye)
```bash
make test                         # C data structures ke unit tests
bash scripts/start_demo.sh        # build + 3 backends (8081-8083) + gateway :10000
bash scripts/demo_commands.sh normal      # 20 requests, servers mein bant jaati hain
bash scripts/demo_commands.sh slow        # Server-3 slow -> traffic uspe se hat jata hai
bash scripts/demo_commands.sh fast        # Server-3 wapas normal (~10 sec mein traffic wapas)
bash scripts/demo_commands.sh failover    # Server-2 band -> baaki servers jawab dete hain
bash scripts/demo_commands.sh recover     # Server-2 wapas start -> pool mein lautta hai
bash scripts/demo_commands.sh burst       # 300 parallel requests -> kuch 429
bash scripts/demo_commands.sh stats       # servers ka ranking table
bash scripts/stop_demo.sh
```
Browser mein: `http://127.0.0.1:10000/` aur `http://127.0.0.1:10000/_stats`.

## EC2 par chalana
Har EC2 par `bin/backend --port 8080 --name Server-1` chalao (Security Group mein 8080 open), phir apne laptop par:
```bash
./bin/gateway --port 10000 --backends <EC2-IP-1>:8080 <EC2-IP-2>:8080 <EC2-IP-3>:8080
```
(EC2 ka public IP restart par badal jata hai.)

## Options
`./bin/gateway --port 10000 --backends h:p h:p ... [--threads 16] [--rate 50] [--burst 100] [--queue 1024]`

## Limitations (sir poochhe toh)
- Abhi sirf bina-body wale requests (GET) forward hote hain; POST body forwarding Phase-II.
- Thread-pool based hai (blocking I/O). epoll-based fully async version future work.
- DBMS (MySQL/PostgreSQL) abhi connect nahi hai: Phase-II mein metrics store honge.

## Tested
- `make test`: saare C data structure tests pass.
- Demo: normal, slow server, failover, recovery, rate limit (429) chale.
- Gateway ThreadSanitizer ke saath load (300 requests, slow node, node band) mein chala, koi data race warning nahi.
