#!/usr/bin/env bash
# Builds, then starts 3 backends (8081-8083) + the gateway on :10000
cd "$(dirname "$0")/.."
bash scripts/stop_demo.sh >/dev/null 2>&1
make -s || exit 1
mkdir -p .run
for i in 1 2 3; do
  ./bin/backend --port 808$i --name "Server-$i" > .run/b$i.log 2>&1 < /dev/null &
  echo $! > .run/b$i.pid
done
sleep 1
./bin/gateway --port 10000 --backends 127.0.0.1:8081 127.0.0.1:8082 127.0.0.1:8083 > .run/gw.log 2>&1 < /dev/null &
echo $! > .run/gw.pid
sleep 1
echo "Ready: http://127.0.0.1:10000   (live scores: http://127.0.0.1:10000/_stats)"
