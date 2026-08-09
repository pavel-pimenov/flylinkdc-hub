#!/usr/bin/env python3
"""Minimal DC++ client to debug hub connection protocol."""
import socket, sys, time
from dc_common import lock2key

host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
port = int(sys.argv[2]) if len(sys.argv) > 2 else 411
nick = sys.argv[3] if len(sys.argv) > 3 else "TestDC"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(10)
s.connect((host, port))
print(f"Connected to {host}:{port}")

buf = b""
while True:
    try:
        chunk = s.recv(4096)
        if not chunk:
            print("Connection closed by hub (EOF)")
            break
        buf += chunk
        while b"|" in buf:
            msg_bytes, buf = buf.split(b"|", 1)
            msg = msg_bytes.decode('utf-8', errors='replace')
            print(f"  RECV: {msg[:200]}")

            if msg.startswith("$Lock "):
                lock = msg.split("$Lock ")[1].split(" Pk=")[0]
                key = lock2key(lock)
                print(f"  Lock: {lock[:40]}... ({len(lock)} chars)")
                print(f"  Key:  {key[:60]}...")
                s.sendall(b"$Key " + key + b"|")
                print("  SENT: $Key")
                time.sleep(0.3)
                s.sendall(f"$ValidateNick {nick}|".encode())
                print(f"  SENT: $ValidateNick {nick}")
                s.sendall(b"$Version 1,0091|")
                print("  SENT: $Version")
                time.sleep(0.5)

            elif msg.startswith("$Hello "):
                print("  === REGISTERED! Sending MyINFO ===")
                s.sendall(b"$Supports MinimumSearch|TTH|TTHSearch|")
                s.sendall(b"$MyINFO $ALL " + nick.encode() + b" Test Client<DC++ V:1.0, M:A, H:1/0/0, S:1>\x01$100|")
                s.sendall(b"$GetNickList|")
                print("  SENT: $Supports + $MyINFO + $GetNickList")
                time.sleep(2)

            elif msg.startswith("$Quit"):
                print("  !!! HUB KICKED US !!!")
                s.close()
                sys.exit(1)

    except socket.timeout:
        print("Timeout waiting for data")
        break
    except ConnectionResetError:
        print("Connection reset by hub")
        break
    except Exception as e:
        print(f"Error: {e}")
        break

s.close()
print("Done")
