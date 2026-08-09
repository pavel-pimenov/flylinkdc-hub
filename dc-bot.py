#!/usr/bin/env python3
import socket
import threading
import os
import time
import hashlib
import sys
from dc_common import lock2key

SHARE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "share")
NICK = "TestBot"
DESCRIPTION = "Test file sharing bot"
TAG = "<DC++ V:0.881, M:A, H:1/0/0, S:0>"
HUB_PORT = 4411

g_users = {}
g_lock = ""
g_key = ""
g_sock = None
g_running = True

def get_share_size():
    total = 0
    for root, dirs, files in os.walk(SHARE_DIR):
        for f in files:
            total += os.path.getsize(os.path.join(root, f))
    return total

def get_files():
    files = []
    for root, dirs, fnames in os.walk(SHARE_DIR):
        for f in fnames:
            fpath = os.path.join(root, f)
            rel = os.path.relpath(fpath, SHARE_DIR)
            size = os.path.getsize(fpath)
            files.append((rel, size))
    return files

def send(msg):
    global g_sock
    if g_sock:
        try:
            g_sock.sendall(msg.encode('utf-8', errors='replace'))
        except Exception:
            pass

def handle_search(data):
    parts = data.split("$Search ", 1)
    if len(parts) < 2:
        return
    query = parts[1]
    qparts = query.split("?", 2)
    if len(qparts) < 3:
        return
    search_str = qparts[2] if len(qparts) > 2 else ""
    search_lower = search_str.lower()

    for rel, size in get_files():
        fname = os.path.basename(rel).lower()
        if search_lower in fname or search_lower == "":
            tth = hashlib.md5(rel.encode()).hexdigest()
            send(f"$SR {NICK}|{rel}\x05{size}|{tth}?\x05{NICK} (0.881)\r\n")
            return

def handle_connect_to_me(data):
    parts = data.split("$ConnectToMe ", 1)
    if len(parts) < 2:
        return
    target = parts[1]
    if NICK in target:
        return

    ip_port = target.split("|")
    if len(ip_port) < 1:
        return

    addr_parts = ip_port[0].split(":")
    if len(addr_parts) < 2:
        return

    remote_nick = target.split(" ")[0] if " " in target else ""
    host = addr_parts[0]
    try:
        port = int(addr_parts[1])
    except ValueError:
        return

    def serve_download(host, port, remote_nick):
        try:
            dsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            dsock.settimeout(10)
            dsock.connect((host, port))
            dsock.sendall(b"$Lock EXTENDEDPROTOCOLABCDEF Pk=PtokaX|")
            time.sleep(0.2)
            data = dsock.recv(4096).decode('utf-8', errors='replace')
            if "$Supports" in data:
                dsock.sendall(b"$Supports MinimumSearch|TTH|TTHSearch|\r\n")
            dsock.sendall(b"$Direction Download 1|")

            fdata = dsock.recv(4096).decode('utf-8', errors='replace')
            if "$FileLength " in fdata:
                fparts = fdata.split("$FileLength ", 1)
                if len(fparts) > 1:
                    flen = fparts[1].split("|")[0]
                    fname_parts = fparts[1].split("|", 1)
                    if len(fname_parts) > 1:
                        fname = fname_parts[1].split("|")[0].strip()
                        for rel, size in get_files():
                            if rel == fname or os.path.basename(rel) == fname:
                                fpath = os.path.join(SHARE_DIR, rel)
                                with open(fpath, 'rb') as f:
                                    while True:
                                        chunk = f.read(65536)
                                        if not chunk:
                                            break
                                        dsock.sendall(chunk)
                                break

            time.sleep(0.5)
            dsock.close()
        except Exception as e:
            pass

    t = threading.Thread(target=serve_download, args=(host, port, remote_nick), daemon=True)
    t.start()

def handle_msg(data):
    global g_lock, g_key, g_running

    if "$Lock " in data:
        g_lock = data.split("$Lock ", 1)[1].split(" Pk=")[0]
        g_key = lock2key(g_lock)
        print(f"[bot] Lock received ({len(g_lock)} chars), sending key", flush=True)
        send(b"$Key " + g_key + b"|")
        send(f"$ValidateNick {NICK}|".encode())
        return

    if "$HubName " in data:
        return

    if "$Search " in data:
        handle_search(data)
        return

    if "$ConnectToMe " in data:
        handle_connect_to_me(data)
        return

    if "$Quit" in data:
        return

    if "$Get " in data or "$UGet " in data:
        return

    if "$Hello " in data:
        send(b"$Version 1,0091|")
        send(b"$Supports MinimumSearch|TTH|TTHSearch|")
        share_size = get_share_size()
        myinfo = f"$MyINFO $ALL {NICK} {DESCRIPTION}{TAG}".encode() + b"$" + bytes([1]) + f"${share_size}$".encode()
        send(myinfo)
        print(f"[bot] Registered as {NICK} ({share_size} bytes shared)", flush=True)
        return

def hub_thread(host, port):
    global g_sock, g_running

    while g_running:
        try:
            g_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            g_sock.settimeout(30)
            g_sock.connect((host, port))
            print(f"[bot] Connected to {host}:{port}")

            buf = b""
            while g_running:
                try:
                    chunk = g_sock.recv(4096)
                    if not chunk:
                        print("[bot] Connection closed by hub")
                        break
                    buf += chunk
                    while b"|" in buf:
                        msg_bytes, buf = buf.split(b"|", 1)
                        msg = msg_bytes.decode('utf-8', errors='replace')
                        if msg.strip():
                            handle_msg(msg)
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"[bot] Error: {e}")
                    break

        except Exception as e:
            print(f"[bot] Connection error: {e}")

        if g_running:
            print("[bot] Reconnecting in 5s...")
            time.sleep(5)

def setup_share():
    os.makedirs(SHARE_DIR, exist_ok=True)
    for i in range(5):
        fpath = os.path.join(SHARE_DIR, f"test-file-{i}.txt")
        if not os.path.exists(fpath):
            with open(fpath, 'w') as f:
                f.write(f"Test file {i}\n" * 100)
    print(f"[bot] Share dir: {SHARE_DIR} ({get_share_size()} bytes, {len(get_files())} files)")

def main():
    global g_running
    host = sys.argv[1] if len(sys.argv) > 1 else "ptokax"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else HUB_PORT

    setup_share()
    print(f"[bot] Starting as {NICK}, sharing {get_share_size()} bytes")

    t = threading.Thread(target=hub_thread, args=(host, port), daemon=True)
    t.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        g_running = False
        print("[bot] Stopped")

if __name__ == "__main__":
    main()
