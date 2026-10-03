#!/usr/bin/env bash
# Automatic end-to-end check. Prints the demo output and fails (exit 1) if anything misbehaves.
cd "$(dirname "$0")/.."
fail=0
check() { if eval "$2"; then echo "  PASS: $1"; else echo "  FAIL: $1"; fail=1; fi; }
counts() { grep -E '^ *[0-9]+ Server-' ; }          # keep only the "count Server-N" lines
cnt() { echo "$1" | counts | grep "Server-$2" | awk '{print $1}'; }

bash scripts/start_demo.sh || exit 1

echo "== 1. Normal routing"
out=$(bash scripts/demo_commands.sh normal); echo "$out"
check "requests reach at least 2 different servers" '[ "$(echo "$out" | counts | wc -l)" -ge 2 ]'

echo "== 2. Slow server (Server-3 made slow)"
out=$(bash scripts/demo_commands.sh slow); echo "$out"
s3=$(cnt "$out" 3); s3=${s3:-0}
check "Server-3 got <= 8 of 40 requests (traffic shifted away)" '[ "$s3" -le 8 ]'
bash scripts/demo_commands.sh fast >/dev/null

echo "== 3. Failover (Server-2 stopped)"
out=$(bash scripts/demo_commands.sh failover); echo "$out"
check "no request went to the dead Server-2" '[ -z "$(cnt "$out" 2)" ]'
check "all 12 requests still succeeded" '[ "$(echo "$out" | counts | awk "{s+=\$1} END{print s}")" -eq 12 ]'

echo "== 4. Recovery (Server-2 restarted)"
out=$(bash scripts/demo_commands.sh recover); echo "$out"
check "Server-2 serves traffic again" '[ -n "$(cnt "$out" 2)" ]'

echo "== 5. Rate limit"
sleep 2
out=$(bash scripts/demo_commands.sh burst); echo "$out"
check "some requests were rejected with 429" 'echo "$out" | grep -q " 429"'
check "some requests succeeded with 200" 'echo "$out" | grep -q " 200"'

bash scripts/stop_demo.sh >/dev/null
[ $fail -eq 0 ] && echo "ALL CHECKS PASSED" || echo "SOME CHECKS FAILED"
exit $fail
