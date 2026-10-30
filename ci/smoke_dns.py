#!/usr/bin/env python3
"""Offline DNS wire-protocol smoke test for native or QEMU-executed static dig."""

import argparse
import socket
import struct
import subprocess
import threading


def serve_one_query(sock: socket.socket, failures: list[Exception]) -> None:
    try:
        packet, address = sock.recvfrom(4096)
        if len(packet) < 17:
            raise ValueError("DNS query too short")
        offset = 12
        while True:
            length = packet[offset]
            offset += 1
            if length == 0:
                break
            if length > 63:
                raise ValueError("Unexpected compressed question")
            offset += length
        question = packet[12 : offset + 4]
        if question[-4:] != b"\x00\x01\x00\x01":
            raise ValueError("Expected an IN A query")
        header = packet[:2] + struct.pack("!HHHHH", 0x8180, 1, 1, 0, 0)
        answer = b"\xc0\x0c" + struct.pack("!HHIH", 1, 1, 60, 4) + bytes((203, 0, 113, 7))
        sock.sendto(header + question + answer, address)
    except Exception as error:
        failures.append(error)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("--qemu-arm", action="store_true")
    args = parser.parse_args()

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
        server.bind(("127.0.0.1", 0))
        server.settimeout(10)
        port = server.getsockname()[1]
        failures: list[Exception] = []
        thread = threading.Thread(target=serve_one_query, args=(server, failures), daemon=True)
        thread.start()
        command = (["qemu-arm"] if args.qemu_arm else []) + [
            args.binary, "@127.0.0.1", "-p", str(port), "static-dig-ci.invalid", "A",
            "+time=3", "+tries=1",
        ]
        result = subprocess.run(command, capture_output=True, text=True, timeout=20, check=False)
        thread.join(timeout=1)
        if failures:
            raise RuntimeError(f"Mock DNS server failed: {failures[0]}")
        if result.returncode or "status: NOERROR" not in result.stdout or "203.0.113.7" not in result.stdout:
            raise RuntimeError(f"dig smoke test failed ({result.returncode}):\n{result.stdout}\n{result.stderr}")
        print(f"PASS: {command[0]} answered offline IN A query over UDP")


if __name__ == "__main__":
    main()
