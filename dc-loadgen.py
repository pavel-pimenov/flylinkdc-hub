#!/usr/bin/env python3
"""DC++ continuous load generator - exercises all hub metrics."""
import socket
import threading
import time
import sys
import random
from dc_common import lock2key

stats_lock = threading.Lock()
stats = {"connected": 0, "errors": 0, "messages_sent": 0, "chats": 0, "searches": 0, "myinfo": 0}

def client_worker(client_id, host, port, duration):
    nick = f"Load{client_id}"
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)
        s.connect((host, port))

        buf = b""
        while b"$Lock " not in buf or b" Pk=" not in buf:
            chunk = s.recv(4096)
            if not chunk:
                raise ConnectionError("no data")
            buf += chunk

        lock = buf.split(b"$Lock ")[1].split(b" Pk=")[0].decode()
        key = lock2key(lock)

        s.sendall(b"$Key " + key + b"|")
        time.sleep(0.05)
        s.sendall(f"$ValidateNick {nick}|".encode())
        time.sleep(0.05)
        s.sendall(b"$Version 1,0091|")
        s.sendall(b"$Supports MinimumSearch|TTH|TTHSearch|")
        s.sendall(f"$MyINFO $ALL {nick} LoadBot<DC++ V:1.0, M:A, H:1/0/0, S:1>$\x01$100|".encode())
        s.sendall(b"$GetNickList|")

        time.sleep(1)

        got_hello = False
        buf2 = b""
        for _ in range(20):
            try:
                d = s.recv(4096)
                if not d:
                    break
                buf2 += d
                if b"$Hello" in buf2:
                    got_hello = True
                    break
            except socket.timeout:
                break

        if not got_hello:
            s.close()
            with stats_lock:
                stats["errors"] += 1
            return

        with stats_lock:
            stats["connected"] += 1

        end_time = time.time() + duration
        search_terms = ["test", "file", "music", "video", "data", "doc", "img", "app", "book", "movie"]
        chat_msgs = ["Hello everyone!", "What's up?", "Anyone online?", "Nice hub!", "Share files please"]

        while time.time() < end_time and g_running:
            try:
                action = random.choices(
                    ["search", "chat", "myinfo", "getinfo", "getnicklist"],
                    weights=[40, 30, 10, 10, 10]
                )[0]

                if action == "search":
                    term = random.choice(search_terms)
                    s.sendall(f"$Search 0:{term}|".encode())
                    with stats_lock:
                        stats["searches"] += 1
                        stats["messages_sent"] += 1

                elif action == "chat":
                    msg = random.choice(chat_msgs)
                    s.sendall(f"$To: *{nick} From: {nick} $<{nick}> {msg}|".encode())
                    with stats_lock:
                        stats["chats"] += 1
                        stats["messages_sent"] += 1

                elif action == "myinfo":
                    share = random.randint(100, 10000)
                    s.sendall(f"$MyINFO $ALL {nick} Bot<{share}>{nick}<DC++ V:1.0>$\x01${share}|".encode())
                    with stats_lock:
                        stats["myinfo"] += 1
                        stats["messages_sent"] += 1

                elif action == "getinfo":
                    s.sendall(f"$GetINFO {nick}|\r\n".encode())
                    with stats_lock:
                        stats["messages_sent"] += 1

                elif action == "getnicklist":
                    s.sendall(b"$GetNickList|")
                    with stats_lock:
                        stats["messages_sent"] += 1

                try:
                    d = s.recv(4096)
                    if not d:
                        break
                except socket.timeout:
                    pass

                time.sleep(random.uniform(0.3, 1.5))
            except (BrokenPipeError, ConnectionResetError, OSError):
                break

        s.close()
        with stats_lock:
            stats["connected"] -= 1

    except Exception as e:
        with stats_lock:
            stats["errors"] += 1

g_running = True

def run_load(host, port, num_clients, duration):
    global g_running
    print(f"Load generator: {num_clients} clients, {duration}s duration")
    print(f"Target: {host}:{port}")
    print("=" * 50)

    threads = []
    for i in range(num_clients):
        t = threading.Thread(target=client_worker, args=(i, host, port, duration), daemon=True)
        threads.append(t)
        t.start()
        time.sleep(0.05)

    start = time.time()
    while time.time() - start < duration and g_running:
        time.sleep(5)
        with stats_lock:
            print(f"  [{int(time.time()-start)}s] connected={stats['connected']} "
                  f"search={stats['searches']} chat={stats['chats']} "
                  f"myinfo={stats['myinfo']} total={stats['messages_sent']} "
                  f"errors={stats['errors']}")

    g_running = False
    for t in threads:
        t.join(timeout=5)

    print("=" * 50)
    print(f"Done. Total: {stats['messages_sent']} messages "
          f"(search={stats['searches']}, chat={stats['chats']}, myinfo={stats['myinfo']})")

if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "ptokax"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 4111
    num = int(sys.argv[3]) if len(sys.argv) > 3 else 10
    dur = int(sys.argv[4]) if len(sys.argv) > 4 else 3600
    run_load(host, port, num, dur)
