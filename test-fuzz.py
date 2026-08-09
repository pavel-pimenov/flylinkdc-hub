#!/usr/bin/env python3
"""DC++ hub protocol fuzzer — covers all 25 parsed protocol commands.

For each command, sends edge-case payloads (max length, empty, special chars,
binary, wrong state, etc.) and verifies the hub stays alive after every case.
"""

import os
import socket
import sys
import time

PORT = int(os.environ.get("HUB_PORT", "411"))
HOST = os.environ.get("HUB_HOST", "127.0.0.1")
TIMEOUT = 10

DCN_SPECIAL = {0, 5, 36, 96, 124, 126}
SKIP_LOGIN = "SKIP_LOGIN"


def lock2key(lock_str: str) -> str:
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


def recv_all(sock: socket.socket, timeout: float = 3.0) -> bytes:
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
    return data


def send_raw(sock: socket.socket, data: bytes):
    sock.sendall(data)


def check_hub_alive(retries: int = 3, delay: float = 0.5) -> bool:
    for attempt in range(retries):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(TIMEOUT)
            sock.connect((HOST, PORT))
            text = recv_some(sock, 1.0)
            sock.close()
            if "$Lock " in text:
                return True
        except Exception:
            pass
        if attempt < retries - 1:
            time.sleep(delay)
    return False


def recv_some(sock: socket.socket, timeout: float = 0.3) -> str:
    """Fast single recv (no loop) to grab $Lock."""
    try:
        sock.settimeout(timeout)
        data = sock.recv(8192)
        return data.decode("latin-1", errors="replace")
    except socket.timeout:
        return ""
    finally:
        sock.settimeout(max(timeout, 1.0))


def send_post_login(sock: socket.socket, nick: str, extra: str = ""):
    """Read $Lock quickly, send login + extra in one batch."""
    text = recv_some(sock, 1.0)
    if "$Lock " not in text:
        return
    lock_str = text.split("$Lock ")[1].split("|")[0].strip()
    lock_core = lock_str.split(" Pk=")[0]
    key = lock2key(lock_core)
    tag = f"<{nick} V:1.0,M:A,H:1/0/0,S:1>"
    share = 1024 * 1024 * 100
    send_raw(sock, (
        "$Supports UserCommand NoGetINFO NoHello UserIP2 TTHSearch ZPipe|"
        f"$Key {key}|"
        f"$ValidateNick {nick}|"
        "$Version 1,0091|$GetNickList|"
        f"$MyINFO $ALL {nick} {tag}$ $100M$test@test.com${share}$$|"
        f"{extra}"
    ).encode("latin-1"))


class FuzzCase:
    def __init__(self, desc: str, fn, needs_login=True):
        self.desc = desc
        self.fn = fn
        self.needs_login = needs_login

    def run(self) -> bool:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(TIMEOUT)
        try:
            sock.connect((HOST, PORT))
            if self.needs_login:
                send_post_login(sock, "FTester")
                time.sleep(0.05)
            self.fn(sock)
        except Exception as e:
            print(f"      EXN: {e}")
        finally:
            try:
                sock.close()
            except Exception:
                pass
        time.sleep(0.05)
        if not check_hub_alive():
            print(f"      CRASH: hub not responding")
            return False
        return True


def fuzz_login_helper(sock: socket.socket, cmds: list[str]):
    text = recv_some(sock, 1.0)
    if "$Lock " not in text:
        return
    lock_str = text.split("$Lock ")[1].split("|")[0].strip()
    lock_core = lock_str.split(" Pk=")[0]
    key = lock2key(lock_core)
    prefix = f"$Key {key}|$ValidateNick FTester|$Version 1,0091|$GetNickList|"
    for c in cmds:
        prefix += c + "|"
    send_raw(sock, prefix.encode("latin-1"))


def group(name: str, cases: list):
    result = []
    for c in cases:
        if len(c) == 3:
            d, f, n = c
        else:
            d, f = c
            n = True
        result.append(FuzzCase(f"{name}: {d}", f, n))
    return result


# ============================================================
# Pre-login commands (need only $Lock from hub)
# ============================================================

PRE_LOGIN = group("pre-login", [
    ("binary garbage before $Lock",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"\x00" * 4096)), False),
    ("format string before $Lock",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"%s%s%s%n%n%n")), False),
    ("long garbage before $Lock",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"A" * 65536)), False),
])

# ============================================================
# 1. $MyNick
# ============================================================

MYNICK = group("$MyNick", [
    ("max length (64)",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick " + b"A" * 64 + b"|")), False),
    ("overflow (256)",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick " + b"B" * 256 + b"|")), False),
    ("empty",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick |")), False),
    ("special chars",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick A B|$MyNick A|B|$MyNick A$B|")), False),
    ("binary",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick " + bytes(range(1, 32)) + b"|")), False),
    ("missing pipe",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick NoPipe")), False),
    ("only dollar",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$MyNick |")), False),
])

# ============================================================
# 2. $Key
# ============================================================

KEY = group("$Key", [
    ("empty",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key |")), False),
    ("very long (4096)",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key " + b"C" * 4096 + b"|")), False),
    ("binary in key",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key " + bytes(range(0, 256)) * 4 + b"|")), False),
    ("special DCN sequences",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key /%DCN000%/|")), False),
    ("missing pipe",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key xyz")), False),
    ("no space after dollar",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key|")), False),
])

# ============================================================
# 3. $Supports
# ============================================================

SUPPORTS = group("$Supports", [
    ("empty list",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Supports |")), False),
    ("very long list (4096)",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Supports " + b"FeatureX/" * 200 + b"|")), False),
    ("binary features",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Supports " + bytes(range(1, 128)) + b"|")), False),
    ("pipe in features",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Supports Feat|ure Another|")), False),
    ("repeat send",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Supports ZPipe|$Supports NoZPipe|")), False),
])

# ============================================================
# 4. $ValidateNick
# ============================================================

VALIDATENICK = group("$ValidateNick", [
    ("max valid (64 chars)",
     lambda s: fuzz_login_helper(s, []), False),
    ("overflow (256 chars)",
     lambda s: fuzz_login_helper(s, [f"$ValidateNick {'B' * 256}"]), False),
    ("64-char nick",
     lambda s: fuzz_login_helper(s, [f"$ValidateNick {'A' * 64}"]), False),
    ("empty",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick |")), False),
    ("space in nick",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick A B|")), False),
    ("pipe in nick",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick A|B|")), False),
    ("dollar in nick",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick A$B|")), False),
    ("null byte in nick",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick A\x00B|")), False),
    ("binary nick",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick " + bytes(range(1, 64)) + b"|")), False),
    ("double ValidateNick",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick A|$ValidateNick B|")), False),
    ("missing pipe",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Key x|$ValidateNick X")), False),
])

# ============================================================
# 5. $Version
# ============================================================

VERSION = group("$Version", [
    ("empty",
     lambda s: fuzz_login_helper(s, ["$Version |"]), False),
    ("very long (1024)",
     lambda s: fuzz_login_helper(s, [f"$Version {'A' * 1024}"]), False),
    ("binary",
     lambda s: fuzz_login_helper(s, ["$Version " + "".join(chr(i) for i in range(1, 128))]), False),
    ("format string",
     lambda s: fuzz_login_helper(s, ["$Version %s%s%n"]), False),
    ("no pipe",
     lambda s: fuzz_login_helper(s, ["$Version 1,0091 without pipe"]), False),
])

# ============================================================
# 6. $MyPass
# ============================================================

MYPASS = group("$MyPass", [
    ("empty",
     lambda s: fuzz_login_helper(s, ["$MyPass |"]), False),
    ("very long (4096)",
     lambda s: fuzz_login_helper(s, [f"$MyPass {'A' * 4096}"]), False),
    ("binary",
     lambda s: fuzz_login_helper(s, ["$MyPass " + "".join(chr(i) for i in range(256))]), False),
    ("pipe in password",
     lambda s: fuzz_login_helper(s, ["$MyPass secret|extra"]), False),
])

# ============================================================
# Post-login commands (need full login)
# ============================================================

# ============================================================
# 7. $GetNickList
# ============================================================

GETNICKLIST = group("$GetNickList", [
    ("with extra parameters",
     lambda s: send_raw(s, b"$GetNickList extra|")),
    ("empty",
     lambda s: send_raw(s, b"$GetNickList|")),
    ("repeated 50x",
     lambda s: send_raw(s, b"$GetNickList|" * 50)),
])

# ============================================================
# 8. $MyINFO
# ============================================================

MYINFO = group("$MyINFO", [
    ("64-char nick",
     lambda s: send_raw(s, f"$MyINFO $ALL {'A' * 64} <T V:1.0>$ $100M$$$|".encode())),
    ("512-byte tag",
     lambda s: send_raw(s, f"$MyINFO $ALL FuzzTester <{'B' * 512}>$ $100M$$$|".encode())),
    ("empty tag",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester $ $100M$$$|")),
    ("minimal fields",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester|")),
    ("pipe in tag",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester <Fuzz|Tester>$ $100M$$$|")),
    ("pipe in description",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester <T V:1.0>$ $100M$email$share$hello|world$")),
    ("null in email",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester <T V:1.0>$ $100M$test\x00test.com$100$$|")),
    ("max share value",
     lambda s: send_raw(s, f"$MyINFO $ALL FuzzTester <T V:1.0>$ $100M$test@test.com${2**64 - 1}$$|".encode())),
    ("negative share",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester <T V:1.0>$ $100M$test@test.com$-100$$|")),
    ("very long connection",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester <T V:1.0>$ $" + b"1" * 256 + b"M$test@test.com$100$$|")),
    ("re-send MyINFO 100x",
     lambda s: send_raw(s, b"$MyINFO $ALL FuzzTester <T V:1.0>$ $100M$$$|" * 100)),
])

# ============================================================
# 9. $GetINFO
# ============================================================

GETINFO = group("$GetINFO", [
    ("self",
     lambda s: send_raw(s, b"$GetINFO FuzzTester|")),
    ("non-existent user",
     lambda s: send_raw(s, b"$GetINFO NoSuchUser|")),
    ("empty nick",
     lambda s: send_raw(s, b"$GetINFO |")),
    ("very long nick",
     lambda s: send_raw(s, b"$GetINFO " + b"A" * 256 + b"|")),
    ("pipe in nick",
     lambda s: send_raw(s, b"$GetINFO A|B|")),
    ("multiple nicks",
     lambda s: send_raw(s, b"$GetINFO FuzzTester OtherUser ThirdUser|")),
])

# ============================================================
# 10. $Search
# ============================================================

SEARCH = group("$Search", [
    ("empty term",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/|")),
    ("max length (150)",
     lambda s: send_raw(s, f"$Search FuzzTester:TTH/{'A' * 150}|".encode())),
    ("overflow (512)",
     lambda s: send_raw(s, f"$Search FuzzTester:TTH/{'B' * 512}|".encode())),
    ("binary in term",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/" + bytes(range(1, 128)) + b"|")),
    ("null in term",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/test\x00file|")),
    ("path traversal",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/../../../etc/passwd|")),
    ("special chars",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/%00%ff%n%s|")),
    ("pipe in search",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/|extra|")),
    ("malformed type",
     lambda s: send_raw(s, b"$Search FuzzTester:UNKNOWN/term|")),
    ("repeated 50x",
     lambda s: send_raw(s, b"$Search FuzzTester:TTH/test|" * 50)),
])

# ============================================================
# 11. $MultiSearch
# ============================================================

MULTISEARCH = group("$MultiSearch", [
    ("empty",
     lambda s: send_raw(s, b"$MultiSearch |")),
    ("max length",
     lambda s: send_raw(s, f"$MultiSearch {'A' * 512}|".encode())),
    ("binary",
     lambda s: send_raw(s, b"$MultiSearch " + bytes(range(1, 128)) + b"|")),
    ("pipe in payload",
     lambda s: send_raw(s, b"$MultiSearch A|B|C|")),
])

# ============================================================
# 12. $ConnectToMe
# ============================================================

CONNECTTOME = group("$ConnectToMe", [
    ("self-connect",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester 127.0.0.1:1024|")),
    ("non-existent user",
     lambda s: send_raw(s, b"$ConnectToMe NoUser 10.0.0.1:1411|")),
    ("empty nick",
     lambda s: send_raw(s, b"$ConnectToMe  10.0.0.1:1411|")),
    ("very long nick (256)",
     lambda s: send_raw(s, b"$ConnectToMe " + b"A" * 256 + b" 127.0.0.1:1411|")),
    ("IPv6 address",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester [::1]:1411|")),
    ("very long IP (512)",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester " + b"B" * 512 + b"|")),
    ("no port",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester 10.0.0.1|")),
    ("port overflow",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester 10.0.0.1:999999|")),
    ("negative port",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester 10.0.0.1:-1|")),
    ("binary in address",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester " + bytes(range(128, 200)) + b"|")),
    ("CTM with pipe",
     lambda s: send_raw(s, b"$ConnectToMe FuzzTester 10.0.0.1:1411|$Kick FuzzTester|")),
])

# ============================================================
# 13. $MultiConnectToMe
# ============================================================

MULTICONNECTTOME = group("$MultiConnectToMe", [
    ("empty",
     lambda s: send_raw(s, b"$MultiConnectToMe |")),
    ("max length",
     lambda s: send_raw(s, f"$MultiConnectToMe {'A' * 512}|".encode())),
    ("binary",
     lambda s: send_raw(s, b"$MultiConnectToMe " + bytes(range(1, 128)) + b"|")),
    ("pipe in payload",
     lambda s: send_raw(s, b"$MultiConnectToMe A|B|")),
])

# ============================================================
# 14. $RevConnectToMe
# ============================================================

REVCONNECTTOME = group("$RevConnectToMe", [
    ("self",
     lambda s: send_raw(s, b"$RevConnectToMe FuzzTester FuzzTester|")),
    ("non-existent user",
     lambda s: send_raw(s, b"$RevConnectToMe NoUser FuzzTester|")),
    ("empty sender",
     lambda s: send_raw(s, b"$RevConnectToMe  FuzzTester|")),
    ("empty target",
     lambda s: send_raw(s, b"$RevConnectToMe FuzzTester |")),
    ("very long names",
     lambda s: send_raw(s, b"$RevConnectToMe " + b"A" * 256 + b" " + b"B" * 256 + b"|")),
    ("pipe in nick",
     lambda s: send_raw(s, b"$RevConnectToMe A|B FuzzTester|")),
])

# ============================================================
# 15. $SR (search result)
# ============================================================

SR = group("$SR", [
    ("empty",
     lambda s: send_raw(s, b"$SR |")),
    ("max length",
     lambda s: send_raw(s, f"$SR {'A' * 1024}|".encode())),
    ("binary",
     lambda s: send_raw(s, b"$SR " + bytes(range(1, 128)) + b"|")),
    ("pipe in result",
     lambda s: send_raw(s, b"$SR A|B|C|")),
    ("malformed result",
     lambda s: send_raw(s, b"$SR FuzzTester 1 0 TTH:AAAA|")),
    ("repeated 50x",
     lambda s: send_raw(s, b"$SR FuzzTester|" * 50)),
])

# ============================================================
# 17. $Close
# ============================================================

CLOSE = group("$Close", [
    ("self-close",
     lambda s: send_raw(s, b"$Close FuzzTester|")),
    ("non-existent user",
     lambda s: send_raw(s, b"$Close NoSuchUser|")),
    ("empty nick",
     lambda s: send_raw(s, b"$Close |")),
    ("very long nick",
     lambda s: send_raw(s, b"$Close " + b"A" * 256 + b"|")),
    ("pipe in nick",
     lambda s: send_raw(s, b"$Close A|B|")),
    ("repeated 50x",
     lambda s: send_raw(s, b"$Close FuzzTester|" * 50)),
])

# ============================================================
# 18. $Kick
# ============================================================

KICK = group("$Kick", [
    ("self-kick",
     lambda s: send_raw(s, b"$Kick FuzzTester|")),
    ("non-existent user",
     lambda s: send_raw(s, b"$Kick NoSuchUser|")),
    ("empty",
     lambda s: send_raw(s, b"$Kick |")),
    ("very long nick",
     lambda s: send_raw(s, b"$Kick " + b"A" * 256 + b"|")),
    ("pipe in nick",
     lambda s: send_raw(s, b"$Kick A|B|")),
    ("repeated 50x",
     lambda s: send_raw(s, b"$Kick FuzzTester|" * 50)),
])

# ============================================================
# 19. $OpForceMove
# ============================================================

OPFORCEMOVE = group("$OpForceMove", [
    ("empty",
     lambda s: send_raw(s, b"$OpForceMove |")),
    ("adcs redirect",
     lambda s: send_raw(s, b"$OpForceMove adcs://other.hub:411|")),
    ("very long address",
     lambda s: send_raw(s, b"$OpForceMove " + b"A" * 512 + b"|")),
    ("binary",
     lambda s: send_raw(s, b"$OpForceMove " + bytes(range(1, 64)) + b"|")),
    ("pipe injection",
     lambda s: send_raw(s, b"$OpForceMove adcs://hub|$Kick FuzzTester|")),
])

# ============================================================
# 20. $To: (private message)
# ============================================================

TO = group("$To:", [
    ("max content (2048)",
     lambda s: send_raw(s, f"$To: FuzzTester $From: FuzzTester ${'A' * 2048}|".encode())),
    ("overflow (65535)",
     lambda s: send_raw(s, f"$To: FuzzTester $From: FuzzTester ${'B' * 65535}|".encode())),
    ("empty",
     lambda s: send_raw(s, b"$To: FuzzTester $From: FuzzTester $|")),
    ("missing From",
     lambda s: send_raw(s, b"$To: FuzzTester $ $text|")),
    ("pipe injection in content",
     lambda s: send_raw(s, b"$To: FuzzTester $From: FuzzTester $hello|$Kick FuzzTester|")),
    ("binary content",
     lambda s: send_raw(s, b"$To: FuzzTester $From: FuzzTester $" + bytes(range(0, 256)) + b"|")),
    ("null bytes",
     lambda s: send_raw(s, b"$To: FuzzTester $From: FuzzTester $hi\x00there|")),
    ("To non-existent user",
     lambda s: send_raw(s, b"$To: NoSuchUser $From: FuzzTester $hello|")),
    ("format string",
     lambda s: send_raw(s, b"$To: FuzzTester $From: FuzzTester $%s%s%n%d|")),
    ("repeated 50x",
     lambda s: send_raw(s, b"$To: FuzzTester $From: FuzzTester $hello|" * 50)),
])

# ============================================================
# 21. $BotINFO
# ============================================================

BOTINFO = group("$BotINFO", [
    ("empty",
     lambda s: send_raw(s, b"$BotINFO |")),
    ("very long (4096)",
     lambda s: send_raw(s, f"$BotINFO {'A' * 4096}|".encode())),
    ("binary",
     lambda s: send_raw(s, b"$BotINFO " + bytes(range(1, 128)) + b"|")),
    ("pipe in info",
     lambda s: send_raw(s, b"$BotINFO Hello|World|")),
])

# ============================================================
# 22. $ExtJSON (FlylinkDC extension)
# ============================================================

EXTJSON = group("$ExtJSON", [
    ("empty",
     lambda s: send_raw(s, b"$ExtJSON |")),
    ("malformed JSON",
     lambda s: send_raw(s, b'$ExtJSON {"cmd":"test", invalid json}|')),
    ("very long JSON",
     lambda s: send_raw(s, f'$ExtJSON {{"data":"{"A" * 4096}"}}|'.encode())),
    ("binary",
     lambda s: send_raw(s, b"$ExtJSON " + bytes(range(1, 64)) + b"|")),
    ("SQL injection in JSON",
     lambda s: send_raw(s, b'$ExtJSON {"nick":"\\"; DROP TABLE users; --"}|')),
    ("nested objects",
     lambda s: send_raw(s, b'$ExtJSON {"a":{"b":{"c":{"d":"e"}}}}|')),
    ("array bomb",
     lambda s: send_raw(s, b"$ExtJSON " + b"[1]" * 1000 + b"|")),
])

# ============================================================
# 23. <chat> (chat messages)
# ============================================================

CHAT = group("<chat>", [
    ("max length (2048)",
     lambda s: send_raw(s, f"<FuzzTester> {'A' * 2048}|".encode())),
    ("overflow (65535)",
     lambda s: send_raw(s, f"<FuzzTester> {'B' * 65535}|".encode())),
    ("empty message",
     lambda s: send_raw(s, b"<FuzzTester> |")),
    ("empty nick",
     lambda s: send_raw(s, b"<> hello|")),
    ("pipe in message",
     lambda s: send_raw(s, b"<FuzzTester> hello | world |")),
    ("dollar in message",
     lambda s: send_raw(s, b"<FuzzTester> cost is $100|")),
    ("binary in message",
     lambda s: send_raw(s, b"<FuzzTester> " + bytes(range(1, 128)) + b"|")),
    ("null byte",
     lambda s: send_raw(s, b"<FuzzTester> hi\x00there|")),
    ("format string",
     lambda s: send_raw(s, b"<FuzzTester> %s%s%n%d|")),
    ("command injection via chat",
     lambda s: send_raw(s, b"<FuzzTester> |$Kick FuzzTester|")),
    ("20 lines burst",
     lambda s: send_raw(s, b"".join(b"<FuzzTester> line %d|" % i for i in range(20)))),
    ("100 lines burst",
     lambda s: send_raw(s, b"".join(b"<FuzzTester> line %d|" % i for i in range(100)))),
])

# ============================================================
# 24. Unknown / malformed commands
# ============================================================

UNKNOWN = group("unknown", [
    ("random unknown commands x100",
     lambda s: send_raw(s, b"".join(b"$UNKNOWN_CMD_%d_%s|" % (i, b"A" * 64) for i in range(100)))),
    ("just dollar",
     lambda s: send_raw(s, b"$" * 1000)),
    ("just pipes",
     lambda s: send_raw(s, b"|" * 8192)),
    ("empty packet",
     lambda s: send_raw(s, b"")),
    ("no pipe terminator",
     lambda s: send_raw(s, b"$Hello test")),
    ("multiple dollars at start",
     lambda s: send_raw(s, b"$$$$$$$$$$$Hello|")),
    ("space after dollar",
     lambda s: send_raw(s, b"$ |")),
    ("very long unknown (8192)",
     lambda s: send_raw(s, b"$" + b"X" * 8192 + b"|")),
])

# ============================================================
# 25. State machine confusion (commands in wrong states)
# ============================================================

STATEMACHINE = group("state-machine", [
    ("login cmds after login",
     lambda s: send_raw(s, b"$Key x|$ValidateNick X|$MyNick X|$Version 1|")),
    ("chat before login",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"<NoLogin> hello|")), False),
    ("$Kick before login",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$Kick Someone|")), False),
    ("$To: before login",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$To: A $From: B $hi|")), False),
    ("$OpForceMove before login",
     lambda s: (recv_some(s, 0.3), send_raw(s, b"$OpForceMove adcs://x|")), False),
    ("sequential 2 connect-disconnect",
     lambda s: None, True),  # handled specially
])

# ============================================================
# All cases combined
# ============================================================

ALL_CASES = (
    PRE_LOGIN + MYNICK + KEY + SUPPORTS + VALIDATENICK + VERSION + MYPASS
    + GETNICKLIST + MYINFO + GETINFO + SEARCH + MULTISEARCH
    + CONNECTTOME + MULTICONNECTTOME + REVCONNECTTOME + SR
    + CLOSE + KICK + OPFORCEMOVE + TO + BOTINFO + EXTJSON
    + CHAT + UNKNOWN + STATEMACHINE
)


def run_fuzz_tests() -> bool:
    # Pre-check
    if not check_hub_alive():
        print("FAIL: hub not reachable at start")
        return False

    total = len(ALL_CASES) + 1
    print(f"=== Fuzz test: {total} protocol edge cases ===")

    # Bulk: sequential connect-disconnect with delay
    print(f"  [1/{total}] state-machine: Sequential 2 connect-disconnect")
    for i in range(2):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect((HOST, PORT))
            s.close()
        except Exception:
            pass
        time.sleep(0.5)
    if not check_hub_alive():
        print("      CRASH after connect-disconnect")
        return False

    failed = []
    for idx, case in enumerate(ALL_CASES, 2):
        print(f"  [{idx}/{total}] {case.desc}")
        if not case.run():
            failed.append(case.desc)

    if failed:
        print(f"\nFAIL: {len(failed)}/{total} fuzz cases triggered hub failure:")
        for c in failed:
            print(f"  - {c}")
        return False
    print(f"\nOK: All {total} fuzz cases passed")
    return True


STRESS_ROUNDS = 6  # ~2 hours at ~20 min/round


def run_stress(duration_minutes: int = 120):
    deadline = time.monotonic() + duration_minutes * 60
    round_num = 0
    failures = 0
    print(f"=== Stress fuzz: {duration_minutes} min ===")
    while time.monotonic() < deadline:
        round_num += 1
        remaining = (deadline - time.monotonic()) / 60
        print(f"\n--- Round {round_num} ({remaining:.1f} min remaining) ---")
        if not run_fuzz_tests():
            failures += 1
            print(f"WARN: Round {round_num} failed ({failures} total failures)")
            if not check_hub_alive(retries=10, delay=1.0):
                print(f"FATAL: Hub unreachable after round {round_num}")
                return False
    print(f"\n=== Stress fuzz complete: {round_num} rounds, {failures} failures ===")
    return failures == 0


if __name__ == "__main__":
    if "--stress" in sys.argv:
        duration = 120
        for i, a in enumerate(sys.argv):
            if a == "--stress" and i + 1 < len(sys.argv):
                try:
                    duration = int(sys.argv[i + 1])
                except ValueError:
                    pass
        ok = run_stress(duration)
    else:
        ok = run_fuzz_tests()
    sys.exit(0 if ok else 1)
