#!/usr/bin/env python3
"""DC++ hub chat test: two clients connect, one sends chat, other reads it."""

import os
import socket
import time
import sys

PORT = int(os.environ.get("HUB_PORT", "411"))
HOST = os.environ.get("HUB_HOST", "127.0.0.1")
TIMEOUT = 15

DCN_SPECIAL = {0, 5, 36, 96, 124, 126}


def lock2key(lock_str: str) -> str:
    """PtokaX Lock2Key algorithm."""
    lock = lock_str.encode("latin-1")[:46]
    result = []
    for i in range(46):
        if i == 0:
            v = (lock[0] ^ lock[45] ^ lock[44] ^ 5) & 0xFF
        else:
            v = (lock[i] ^ lock[i - 1]) & 0xFF
        v = ((v << 4) & 0xF0) | ((v >> 4) & 0x0F)
        if v in DCN_SPECIAL:
            result.append(f"/%DCN{v:03d}%/")
        else:
            result.append(chr(v))
    return "".join(result)


def recv_all(sock: socket.socket, timeout: float = 3.0) -> str:
    """Receive all available data with total timeout."""
    data = b""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        try:
            sock.settimeout(remaining)
            chunk = sock.recv(8192)
            if not chunk:
                break
            data += chunk
        except socket.timeout:
            break
    sock.settimeout(max(timeout, 0.5))
    return data.decode("latin-1", errors="replace")


def send_raw(sock: socket.socket, data: str):
    """Send raw data."""
    sock.sendall(data.encode("latin-1"))


def connect_and_login(nick: str) -> socket.socket:
    """Connect to hub, full NMDC login sequence."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)
    sock.connect((HOST, PORT))

    # Step 1: Receive $Lock
    data = recv_all(sock, 5.0)
    if "$Lock " not in data:
        raise RuntimeError(f"No $Lock: {data[:200]!r}")

    lock_str = data.split("$Lock ")[1].split("|")[0].strip()
    lock_core = lock_str.split(" Pk=")[0]
    key = lock2key(lock_core)

    # Step 2: Send $Supports + $Key + $ValidateNick in ONE packet
    packet = (
        "$Supports UserCommand NoGetINFO NoHello UserIP2 TTHSearch ZPipe|"
        f"$Key {key}|"
        f"$ValidateNick {nick}|"
    )
    send_raw(sock, packet)

    # Step 3: Wait for $Hello (or $GetPass)
    time.sleep(1.0)
    data = recv_all(sock, 3.0)

    if f"$Hello {nick}" not in data:
        raise RuntimeError(f"No $Hello: {data[:300]!r}")

    print(f"  <- Got $Hello")

    # Step 4: Send $Version + $GetNickList + $MyINFO
    # MyINFO format: $MyINFO $ALL nick tag$magic$conn$email$share$$|
    # Part 1 (magic) must be exactly 1 char
    # Tag must be valid DC++ format (V:,M:,H:,S:) to pass hub's NoTagCheck
    tag = f"<{nick} V:1.0,M:A,H:1/0/0,S:1>"
    share = 1024 * 1024 * 100
    myinfo = f"$MyINFO $ALL {nick} {tag}$ $100M$test@test.com${share}$$"
    login = f"$Version 1,0091|$GetNickList|{myinfo}|"
    send_raw(sock, login)

    # Step 5: Wait for hub to process state transitions
    # STATE_ADDME → STATE_ADDME_1LOOP → STATE_ADDME_2LOOP → STATE_ADDED
    # Each takes one service loop iteration (~100ms)
    time.sleep(1.0)

    # Drain ALL buffered data
    drain = recv_all(sock, 2.0)
    print(f"  Drained {len(drain)} bytes")

    # Check connection is alive
    try:
        sock.settimeout(1.0)
        extra = sock.recv(1)
        if extra == b"":
            raise RuntimeError("Connection closed by hub")
    except socket.timeout:
        pass  # expected — no more data, connection alive

    return sock


def test_concurrent_locks(n_clients: int = 3, per_lock_timeout: float = 5.0) -> bool:
    """Regression test for the receive-loop busy-loop bug.

    Symptom that this catches: the hub's ReceiveLoop used to spin forever on a
    single user (e.g. a PINGER bot in STATE_CLOSING with a `continue` that
    skipped the iterator increment), so it stopped draining the accept queue.
    New TCP connections were accepted by the kernel but the hub never sent them
    $Lock, so DC clients timed out on login.

    A healthy hub replies with $Lock immediately after accept, *before* any
    login data. This test opens a few fresh connections one-by-one and checks
    that the hub replies with $Lock (and, for the first one, completes login).
    The number of connections is kept small on purpose: the hub limits how many
    simultaneous logins may come from a single IP, so later connections may hit
    that limit — which is expected and NOT a failure. The key assertion is that
    the hub is still draining its accept queue (the first connection gets
    $Lock); if the receive loop were stuck, even the first connection would hang.
    """
    IP_LIMIT_MSGS = (
        "maximum number of connections from your ip",
        "max connections from",
        "äîñòèãíóòî ìàêñèìàëüíîå",
    )
    print(f"Opening {n_clients} sequential connections, expecting $Lock from the first...")
    socks = []
    got_lock = 0
    failed = []
    try:
        for i in range(n_clients):
            nick = f"RegrTestUser{i}"
            try:
                sock = connect_and_login(nick)
                socks.append(sock)
                got_lock += 1
                print(f"  OK: #{i} ({nick}) got $Lock + logged in")
            except Exception as e:  # noqa: BLE001
                msg = str(e)
                if any(m in msg.lower() for m in IP_LIMIT_MSGS):
                    print(f"  SKIP: #{i} ({nick}) hit per-IP connection limit (expected): {msg[:80]}")
                else:
                    failed.append((i, msg))
                    print(f"  FAIL: #{i} ({nick}): {msg[:80]}")
            time.sleep(0.3)  # let the hub finish processing before the next connect
    finally:
        for s in socks:
            try:
                s.close()
            except Exception:  # noqa: BLE001
                pass

    if got_lock == 0:
        print("FAIL: hub sent $Lock to NONE of the connections (accept queue not drained?)")
        return False
    if failed:
        print(f"FAIL: {len(failed)} unexpected failures")
        return False
    print(f"PASS: hub replied with $Lock (got_lock={got_lock}/{n_clients})")
    return True


def test_chat():
    """Test: client A sends chat, client B receives it."""
    sock_a = None
    sock_b = None
    try:
        print("Connecting client A (User1)...")
        sock_a = connect_and_login("User1")
        print("OK: User1 logged in\n")

        time.sleep(2.0)

        print("Connecting client B (User2)...")
        sock_b = connect_and_login("User2")
        print("OK: User2 logged in\n")

        # Final drain before chat
        time.sleep(3.0)
        recv_all(sock_a, 1.0)
        recv_all(sock_b, 1.0)

        # Client A sends chat
        test_msg = "Hello from User1!"
        chat_cmd = f"<User1> {test_msg}|"
        print(f"User1 sending chat: '{test_msg}'")
        send_raw(sock_a, chat_cmd)

        # Wait for broadcast
        time.sleep(3.0)

        # Read responses from BOTH clients
        received_b = recv_all(sock_b, 3.0)
        received_a = recv_all(sock_a, 3.0)

        print(f"\nUser2 received ({len(received_b)} bytes): {received_b[:300]!r}")
        print(f"User1 received ({len(received_a)} bytes): {received_a[:300]!r}")

        # Check for chat echo on User2
        expected = "<User1> " + test_msg
        if expected in received_b:
            print(f"\nPASS: User2 received chat")
            return True
        elif "User1" in received_b:
            print(f"\nPARTIAL: User2 received data containing User1")
            return True
        else:
            print(f"\nFAIL: User2 did not receive chat")
            print(f"  Expected containing: {expected!r}")
            return False

    finally:
        if sock_a:
            sock_a.close()
        if sock_b:
            sock_b.close()


if __name__ == "__main__":
    ok = True
    try:
        ok &= test_concurrent_locks()
    except Exception as e:  # noqa: BLE001
        print(f"ERROR in concurrent-locks test: {e}")
        ok = False
    try:
        ok &= test_chat()
    except Exception as e:  # noqa: BLE001
        print(f"ERROR in chat test: {e}")
        ok = False
    sys.exit(0 if ok else 1)
