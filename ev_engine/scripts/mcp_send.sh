#!/usr/bin/env bash
# mcp_send.sh — Send a Python script to Blender on Mac Mini via MCP socket
# Usage: ./scripts/mcp_send.sh script.py
#
# Copies script to Mini, executes via socket, returns result.

set -euo pipefail

MINI_HOST="${MINI_HOST:-mini-ts}"
PORT="${BLENDER_PORT:-9877}"

if [[ ! -f "${1:-}" ]]; then
    echo "Usage: $0 script.py"
    exit 1
fi

REMOTE_CODE="/tmp/ev_blender_code.py"
REMOTE_SENDER="/tmp/ev_blender_sender.py"

# Copy user script to Mini
scp -q "$1" "$MINI_HOST:$REMOTE_CODE"

# Ensure the sender exists on Mini (idempotent)
ssh -o ConnectTimeout=5 "$MINI_HOST" "test -f $REMOTE_SENDER" 2>/dev/null || \
ssh "$MINI_HOST" "cat > $REMOTE_SENDER" << 'EOF'
import socket, json, sys

with open(sys.argv[1]) as f:
    code = f.read()

port = int(sys.argv[2]) if len(sys.argv) > 2 else 9877

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(120)
s.connect(("localhost", port))
payload = json.dumps({"type": "execute_code", "params": {"code": code}})
s.sendall(payload.encode())
data = b""
while True:
    try:
        chunk = s.recv(8192)
        if not chunk:
            break
        data += chunk
    except:
        break
s.close()

resp = json.loads(data)
if resp.get("status") == "success":
    result = resp.get("result", {})
    if isinstance(result, dict) and result.get("result"):
        print(result["result"].rstrip())
    else:
        print(json.dumps(result, indent=2))
else:
    print("ERROR:", json.dumps(resp, indent=2))
    sys.exit(1)
EOF

# Execute with generous timeout
ssh -o ServerAliveInterval=15 -o ServerAliveCountMax=8 "$MINI_HOST" \
    "python3 $REMOTE_SENDER $REMOTE_CODE $PORT"
