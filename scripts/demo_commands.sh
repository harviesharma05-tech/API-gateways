#!/usr/bin/env bash
# Usage: bash scripts/demo_commands.sh <normal|slow|fast|failover|recover|burst|stats>
cd "$(dirname "$0")/.."
GW=${GW:-http://127.0.0.1:10000}
who() { curl -s -m 10 "$GW/" | grep -o 'Server-[0-9]'; }
case "$1" in
  normal)   echo "20 requests:"; for i in $(seq 1 20); do who; done | sort | uniq -c ;;
  slow)     echo "Making Server-3 slow (1.5 s); sending 40 requests ..."
            curl -s "http://127.0.0.1:8083/set_delay?s=1.5" >/dev/null
            for i in $(seq 1 40); do who & sleep 0.1; done | sort | uniq -c; wait
            curl -s "$GW/_stats" ;;
  fast)     curl -s "http://127.0.0.1:8083/set_delay?s=0" >/dev/null; echo "Server-3 back to normal (wait ~10 s for traffic to return)" ;;
  failover) echo "Stopping Server-2 ..."; kill "$(cat .run/b2.pid)"; sleep 4
            echo "12 requests:"; for i in $(seq 1 12); do who; done | sort | uniq -c
            curl -s "$GW/_stats" ;;
  recover)  ./bin/backend --port 8082 --name Server-2 > .run/b2.log 2>&1 < /dev/null & echo $! > .run/b2.pid
            sleep 4; echo "20 requests:"; for i in $(seq 1 20); do who; done | sort | uniq -c ;;
  burst)    echo "300 parallel requests (HTTP status codes):"
            for i in $(seq 1 300); do curl -s -m 8 -o /dev/null -w "%{http_code}\n" "$GW/" & done | sort | uniq -c; wait ;;
  stats)    curl -s "$GW/_stats" ;;
  *) echo "usage: $0 normal|slow|fast|failover|recover|burst|stats" ;;
esac
