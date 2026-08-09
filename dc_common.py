"""Shared DC++ protocol utilities for hub testing scripts."""

DCN_MAP = {
    0: b'/%DCN000%/',
    5: b'/%DCN005%/',
    36: b'/%DCN036%/',
    96: b'/%DCN096%/',
    124: b'/%DCN124%/',
    126: b'/%DCN126%/',
}


def lock2key(lock):
    """Convert a DC++ hub $Lock string to a $Key response (bytes)."""
    lock46 = lock[:46]
    result = bytearray()
    for i in range(len(lock46)):
        if i == 0:
            v = (ord(lock46[0]) ^ ord(lock46[-1]) ^ ord(lock46[-2]) ^ 5) & 0xFF
        else:
            v = (ord(lock46[i]) ^ ord(lock46[i - 1])) & 0xFF
        v = ((v << 4) & 0xF0) | ((v >> 4) & 0x0F)
        result.extend(DCN_MAP.get(v, bytes([v])))
    return bytes(result)
