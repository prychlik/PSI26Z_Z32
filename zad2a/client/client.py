#!/usr/bin/env python3
import socket
import struct
import sys
import time
import csv
import statistics
import matplotlib.pyplot as plt

DEFAULT_SRV_IP = "172.21.32.10"
DEFAULT_PORT = 5000
TIMEOUT_S = 1.0
RETRIES = 3
REPEATS = 25

# Nagłówek datagramu: nr sekwencyjny (u32), znacznik czasu [us] (u64), deklarowany rozmiar (u32)
HDR = struct.Struct("!IQI")
# Odpowiedź serwera: "OK", nr sekwencyjny, odesłany znacznik czasu, odebrany rozmiar
REPLY = struct.Struct("!2sIQI")
MIN_SIZE = HDR.size  # 16 B


def build_datagram(seq: int, size: int) -> bytes:
    ts_us = time.time_ns() // 1000
    return HDR.pack(seq, ts_us, size) + b"A" * (size - HDR.size)


def exchange(sock, addr, seq, size):
    data = build_datagram(seq, size)
    try:
        t0 = time.perf_counter_ns()
        sock.sendto(data, addr)
    except OSError as e:
        print(f"  sendto({size}B) nieudane: {e}")
        return None

    while True:
        try:
            resp, _ = sock.recvfrom(1024)
        except socket.timeout:
            return None
        t1 = time.perf_counter_ns()
        tag, r_seq, r_ts, r_len = REPLY.unpack(resp)
        if tag != b"OK" or r_seq != seq:
            continue               # spóźniona odpowiedź na wcześniejszy datagram
        if r_len != size:
            return None            # serwer odebrał uciętą wiadomość
        return (t1 - t0) / 1000.0


def try_size(sock, addr, seq_counter, size, repeats=REPEATS):
    rtts = []
    for _ in range(repeats):
        rtt = None
        for _ in range(RETRIES):
            seq_counter[0] += 1
            rtt = exchange(sock, addr, seq_counter[0], size)
            if rtt is not None:
                break
        if rtt is None:
            return None
        rtts.append(rtt)
    return rtts


def stats(rtts):
    return {
        "mean": statistics.fmean(rtts),
        "median": statistics.median(rtts),
        "min": min(rtts),
        "max": max(rtts),
        "stdev": statistics.stdev(rtts) if len(rtts) > 1 else 0.0,
    }


def save_plots(results):
    xs = sorted(results)
    mean = [results[x]["mean"] for x in xs]
    med = [results[x]["median"] for x in xs]
    std = [results[x]["stdev"] for x in xs]

    def finish(title, name):
        plt.xlabel("Rozmiar datagramu [B]")
        plt.ylabel("RTT [µs]")
        plt.title(title)
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(name)
        plt.close()

    plt.figure(figsize=(9, 5))
    plt.plot(xs, mean, "o-")
    finish(f"Średni RTT (n={REPEATS})", "udp_rtt.png")

    plt.figure(figsize=(9, 5))
    plt.plot(xs, med, "s-", color="tab:orange")
    finish(f"Mediana RTT (n={REPEATS})", "udp_rtt_median.png")

    plt.figure(figsize=(9, 5))
    plt.errorbar(xs, mean, yerr=std, fmt="o-", capsize=3, alpha=0.7,
                 label="średnia ± odch. std.")
    plt.plot(xs, med, "s--", color="tab:orange", label="mediana")
    plt.legend()
    finish(f"RTT: średnia i mediana (n={REPEATS})", "udp_rtt_combined.png")


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRV_IP
    port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_PORT
    addr = (host, port)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(TIMEOUT_S)
    seq = [0]
    results = {}

    print(f"Klient UDP -> {host}:{port}")

    # rosnące rozmiary
    sizes = [100, 200, 500, 1000, 2000, 5000, 10000, 20000, 30000,
             40000, 50000, 60000, 60000, 70000, 80000, 90000, 100_000]
    last_ok, first_fail = MIN_SIZE, None
    for size in sizes:
        rtt = try_size(sock, addr, seq, size)
        if rtt is None:
            print(f"{size:6d} B: BŁĄD")
            first_fail = size
            break
        print(f"{size:6d} B: OK, RTT = {rtt}")
        results[size] = stats(rtt)
        last_ok = size

    # przeszukiwanie binarne
    if first_fail is not None:
        lo, hi = last_ok, first_fail
        while hi - lo > 1:
            mid = (lo + hi) // 2
            rtt = try_size(sock, addr, seq, mid, repeats=2)
            if rtt is None:
                print(f"  {mid} B: BŁĄD")
                hi = mid
            else:
                print(f"  {mid} B: OK, RTT = {rtt}")
                results[mid] = stats(rtt)
                lo = mid
        print(f"\nMaksymalny obsługiwany datagram: {lo} B (rozmiar {hi} B już nie działa)")
    else:
        print(f"\nWszystkie testowane rozmiary działają, maksimum >= {last_ok} B")

    sock.close()

    # Zapis i wykres
    with open("udp_rtt.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["size_B", "mean_us", "median_us", "min_us", "max_us", "stdev_us"])
        for x in sorted(results):
            r = results[x]
            w.writerow([x, f"{r['mean']:.3f}", f"{r['median']:.3f}",
                        f"{r['min']:.3f}", f"{r['max']:.3f}", f"{r['stdev']:.3f}"])

    save_plots(results)


if __name__ == "__main__":
    main()
