#!/usr/bin/env python3
"""Jacobi 符号测试脚本：二分搜索最大 k 值，使得 C++ 程序在 1 秒内完成 100 对测试。

用法: python test_runner.py [--k-max MAX] [--dry-run K]
"""

import subprocess
import sys
import time
import random
import os

# 允许大整数字符串转换（k=100000 需要约 30000 位数字）
sys.set_int_max_str_digits(100000)


def jacobi_py(a, n):
    """纯 Python Jacobi 符号计算（用于验证 C++ 输出）。"""
    assert n > 0 and n & 1, "n must be positive odd"
    result = 1
    a %= n
    while a:
        t = (a & -a).bit_length() - 1  # trailing zeros
        a >>= t
        if t & 1:
            if n & 7 in (3, 5):
                result = -result
        if a == 1:
            return result
        if a & 3 == 3 and n & 3 == 3:
            result = -result
        a, n = n % a, a
    return 0 if n != 1 else result


def generate_pairs(k):
    """生成 100 对随机的 k-bit 整数 (m, n)，其中 n 为奇数。"""
    lo = 1 << (k - 1)
    hi = (1 << k) - 1
    pairs = []
    for _ in range(100):
        m = random.randint(lo, hi)
        n = random.randint(lo, hi) | 1
        pairs.append((m, n))
    return pairs


def run_test(k, exe_path, verbose=False):
    """运行 C++ 程序，返回 (time_ms, success, message)。"""
    pairs = generate_pairs(k)

    input_lines = [str(k)]
    for m, n in pairs:
        input_lines.append(str(m))
        input_lines.append(str(n))
    input_str = '\n'.join(input_lines) + '\n'

    proc = subprocess.Popen(
        [exe_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    t0 = time.perf_counter()
    try:
        stdout, stderr = proc.communicate(input=input_str, timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()
        return float('inf'), False, "Timeout"
    t1 = time.perf_counter()

    elapsed_ms = (t1 - t0) * 1000

    if proc.returncode != 0:
        return elapsed_ms, False, f"Exit {proc.returncode}: {stderr[:200]}"

    output_lines = stdout.strip().split('\n')
    if len(output_lines) != 100:
        return elapsed_ms, False, f"Expected 100 lines, got {len(output_lines)}"

    for i, line in enumerate(output_lines):
        expected = jacobi_py(pairs[i][0], pairs[i][1])
        try:
            actual = int(line.strip())
        except ValueError:
            return elapsed_ms, False, f"Non-integer output at line {i}: '{line}'"
        if actual != expected:
            return elapsed_ms, False, (
                f"Mismatch at pair {i}: ({pairs[i][0]}/{pairs[i][1]}): "
                f"got {actual}, expected {expected}"
            )

    return elapsed_ms, True, "OK"


def find_max_k(exe_path, start=1, max_search=1_000_000):
    """二分搜索最大 k，使得 100 对测试在 1000ms 内全部正确完成。"""
    print(f"Starting binary search for maximum k (limit: {max_search})...")

    # 指数搜索找到上界
    lo = start
    hi = start
    while hi <= max_search:
        t, ok, msg = run_test(hi, exe_path)
        print(f"  k={hi:>8}: {t:>8.1f}ms  {msg}")
        sys.stdout.flush()
        if not ok or t > 1000:
            break
        lo = hi
        hi *= 2

    if hi <= start:
        print("FAIL: even minimum k fails!")
        return None

    # 二分搜索
    hi = min(hi - 1, max_search)
    if hi < lo:
        return lo

    best = lo
    while lo <= hi:
        mid = (lo + hi) // 2
        t, ok, msg = run_test(mid, exe_path)
        print(f"  k={mid:>8}: {t:>8.1f}ms  {msg}")
        sys.stdout.flush()
        if ok and t <= 1000:
            best = mid
            lo = mid + 1
        else:
            hi = mid - 1

    return best


if __name__ == '__main__':
    import argparse

    ap = argparse.ArgumentParser()
    ap.add_argument('--max-k', type=int, default=1_000_000,
                    help='Maximum k to search (default: 1000000)')
    ap.add_argument('--dry-run', type=int, metavar='K',
                    help='Run a single test at the given k and exit')
    ap.add_argument('--exe', type=str, default='./jacobi_main.exe',
                    help='Path to the C++ executable')
    args = ap.parse_args()

    # 编译
    if not os.path.exists(args.exe):
        print(f"Building {args.exe}...")
        subprocess.run(['bash', 'build.sh', 'main'], check=True, cwd=os.path.dirname(os.path.abspath(args.exe)) or '.')

    if args.dry_run is not None:
        t, ok, msg = run_test(args.dry_run, args.exe)
        print(f"k={args.dry_run}: {t:.1f}ms  {msg}")
        sys.exit(0 if ok else 1)

    max_k = find_max_k(args.exe, start=1, max_search=args.max_k)
    if max_k is not None:
        print(f"\nMaximum k: {max_k}")
    else:
        print("\nFailed to find maximum k")
        sys.exit(1)
