#!/usr/bin/env python3
"""DC++ hub load tester - spawns multiple concurrent clients."""
import socket
import threading
import time
import sys
import statistics
from dc_common import lock2key

results_lock = threading.Lock()
results = {"success": 0, "fail": 0, "latencies": []}

def client_worker(client_id, host, port):
    nick = f"LoadTest{client_id}"
    t0 = time.time()
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)
        s.connect((host, port))

        buf = b""
        while b"$Lock " not in buf or b" Pk=" not in buf:
            chunk = s.recv(4096)
            if not chunk:
                raise ConnectionError("No data from hub")
            buf += chunk

        lock = buf.split(b"$Lock ")[1].split(b" Pk=")[0].decode()
        key = lock2key(lock)

        s.sendall(b"$Key " + key + b"|")
        time.sleep(0.1)
        s.sendall(f"$ValidateNick {nick}|".encode())
        time.sleep(0.1)
        s.sendall(b"$Version 1,0091|")
        time.sleep(0.1)
        s.sendall(b"$Supports MinimumSearch|TTH|TTHSearch|")
        s.sendall(f"$MyINFO $ALL {nick} LoadTester<DC++ V:1.0>$\x01$0|".encode())
        s.sendall(b"$GetNickList|")

        time.sleep(0.5)

        got_hello = False
        buf = b""
        for _ in range(20):
            try:
                d = s.recv(4096)
                if not d:
                    break
                buf += d
                if b"$Hello" in buf:
                    got_hello = True
                    break
            except socket.timeout:
                break
            except Exception as e:
                print(f"  Client {client_id} recv error: {e}")
                break

        latency = time.time() - t0
        with results_lock:
            if got_hello:
                results["success"] += 1
                results["latencies"].append(latency)
            else:
                results["fail"] += 1

        time.sleep(2)

        s.sendall(f'$Search 0:{nick}|'.encode())
        time.sleep(0.5)

        s.close()
    except Exception as e:
        with results_lock:
            results["fail"] += 1
        print(f"  Client {client_id} error: {e}", flush=True)

def run_load_test(host, port, num_clients, ramp_delay=0.1):
    print(f"Load test: {num_clients} clients -> {host}:{port}")
    print(f"Ramp delay: {ramp_delay}s between connections")
    print("=" * 50)

    threads = []
    t_start = time.time()

    for i in range(num_clients):
        t = threading.Thread(target=client_worker, args=(i, host, port), daemon=True)
        threads.append(t)
        t.start()
        time.sleep(ramp_delay)

    for t in threads:
        t.join(timeout=30)

    t_total = time.time() - t_start

    print("=" * 50)
    print(f"Results ({t_total:.1f}s total):")
    print(f"  Success: {results['success']}")
    print(f"  Failed:  {results['fail']}")
    if results["latencies"]:
        lats = results["latencies"]
        print(f"  Latency (connect+register):")
        print(f"    Min:    {min(lats)*1000:.0f}ms")
        print(f"    Max:    {max(lats)*1000:.0f}ms")
        print(f"    Mean:   {statistics.mean(lats)*1000:.0f}ms")
        print(f"    Median: {statistics.median(lats)*1000:.0f}ms")
        if len(lats) > 1:
            print(f"    Stdev:  {statistics.stdev(lats)*1000:.0f}ms")
        print(f"  Throughput: {results['success']/t_total:.1f} clients/s")

if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 4111
    num = int(sys.argv[3]) if len(sys.argv) > 3 else 50
    ramp = float(sys.argv[4]) if len(sys.argv) > 4 else 0.1
    run_load_test(host, port, num, ramp)
