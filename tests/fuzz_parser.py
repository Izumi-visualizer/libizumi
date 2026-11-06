#!/usr/bin/env python3
"""Seeded, process-isolated mutation and model-based Kanata parser fuzzing."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time


def hex_string(text):
    return "-" if text is None else text.encode().hex()


def valid_trace(rng):
    """Generate valid commands and independently model their observable state."""
    cycle = rng.choice([0, 100, 2**32, 2**63])
    lines = ["Kanata\t0004", f"C=\t{cycle}"]
    instructions = {}
    for ident in rng.sample([0, 1, 255, 256, 257, 1025, 4097], rng.randint(1, 7)):
        lines.append(f"I\t{ident}\t{rng.getrandbits(64)}\t{rng.randrange(8)}")
        label = rng.choice(["nop", "  add\ta0, a1, a2", "FADD.D", "", "x" * 512])
        pc = rng.choice([None, 0, 0x80000000, 2**64 - 1])
        lines.append(f"L\t{ident}\t0\t" + (f"0x{pc:x}: " if pc is not None else "") + label)
        # PC extraction consumes whitespace immediately after the colon.
        if pc is not None:
            label = label.lstrip()
        suffix = rng.choice(["", " ; comment", "\toperand"])
        lines += [f"L\t{ident}\t0\t{suffix}", f"L\t{ident}\t1\tignored"]
        instructions[ident] = {"label": label + suffix, "pc": pc, "stages": [],
                               "finished": False, "flushed": False}
    for _ in range(rng.randint(1, 100)):
        delta = rng.randrange(8)
        cycle += delta
        lines.append(f"C\t{delta}")
        ident = rng.choice(list(instructions))
        inst = instructions[ident]
        if inst["finished"]:
            continue
        lane = rng.choice([0, 1, 7, 2**64 - 1])
        name = rng.choice(["F", "Execute", "Long stage name", "stl"])
        action = rng.randrange(5)
        if action < 3:
            for stage in inst["stages"]:
                if stage["open"] and stage["lane"] == lane:
                    stage.update(duration=cycle - stage["cycle"], open=False)
            inst["stages"].append(dict(cycle=cycle, duration=0, lane=lane, open=True, name=name))
            lines.append(f"S\t{ident}\t{lane}\t{name}")
        elif action == 3:
            for stage in reversed(inst["stages"]):
                if stage["open"] and stage["lane"] == lane and stage["name"] == name:
                    stage.update(duration=cycle - stage["cycle"], open=False)
                    break
            lines.append(f"E\t{ident}\t{lane}\t{name}")
        else:
            flushed = rng.choice([False, True])
            lines.append(f"R\t{ident}\t0\t{int(flushed)}")
            inst.update(finished=True, flushed=flushed)
            for stage in inst["stages"]:
                if stage["open"]:
                    stage.update(duration=cycle - stage["cycle"], open=False)
    output = []
    for ident, inst in sorted(instructions.items()):
        pc = None if inst["pc"] is None else f'0x{inst["pc"]:016x}'
        output.append(f'I {ident} {int(inst["finished"])} {int(inst["flushed"])} '
                      f'{hex_string(pc)} {hex_string(inst["label"])}\n')
        for s in inst["stages"]:
            output.append(f'S {s["cycle"]} {s["duration"]} {s["lane"]} '
                          f'{int(s["open"])} {hex_string(s["name"])}\n')
    ending = rng.choice(["\n", "\r\n"])
    return (ending.join(lines) + ending).encode(), "".join(output).encode()


def invalid_seeds():
    return [b"C\t-1\n", b"C=\t18446744073709551616\n", b"C\t1junk\n",
            b"I\t-1\t0\t0\n", b"I\t18446744073709551615\t0\t0\n",
            b"C=\t18446744073709551615\nC\t1\n",
            b"C=\t10\nI\t0\t0\t0\nS\t0\t0\tF\nC=\t1\nR\t0\t0\t0\n",
            b"I\t0\t0\t0\nL\t0\t4294967296\twrapped\n",
            b"I\t0\t0\t0\nS\t0\t-1\tF\n",
            b"I\t0\t0\t0\nR\t0\t0\t-1\n", b"I\t0\t0\t0\x00hidden\n",
            b"I\t0\t0\t0\nR\t0\t0\t0\nS\t0\t0\tF\n"]


def mutate(rng, data):
    data = bytearray(data)
    for _ in range(rng.randint(1, 5)):
        pos = rng.randrange(len(data) + 1)
        action = rng.randrange(4)
        if action == 0 and pos < len(data):
            data[pos] = rng.randrange(256)
        elif action == 1:
            data[pos:pos] = rng.choice([b"\x00", b"-1", b"18446744073709551616", b"\t", b"\n"])
        elif action == 2:
            del data[pos:pos + rng.randint(1, 40)]
        else:
            data[pos:pos] = data[max(0, pos - 40):pos]
    return bytes(data[:16384])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--seconds", type=float, default=600)
    parser.add_argument("--seed", type=int, default=20261106)
    parser.add_argument("--output", type=Path, default=Path("fuzz-results"))
    parser.add_argument("--timeout", type=float, default=2)
    args = parser.parse_args()
    if args.seconds <= 0 or args.timeout <= 0:
        parser.error("seconds and timeout must be positive")
    driver = args.driver.resolve()
    if not driver.is_file():
        parser.error(f"driver does not exist: {driver}")
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(args.seed)
    seeds = invalid_seeds()
    stats = dict(seed=args.seed, cases=0, valid=0, rejected=0, failures=0)
    started = time.monotonic()
    with tempfile.TemporaryDirectory(prefix="izumi-fuzz-") as temp:
        path = Path(temp) / "input.kanata"
        while time.monotonic() - started < args.seconds:
            index = stats["cases"]
            expected = None
            must_reject = index < len(seeds)
            if must_reject:
                data = seeds[index]
            else:
                data, expected = valid_trace(rng)
                if rng.randrange(2):
                    data, expected = mutate(rng, data), None
            path.write_bytes(data)
            env = os.environ.copy()
            # Rejected inputs exit(1), so their process-lifetime allocations
            # cannot be cleaned up by the caller. Leak-check valid model cases.
            env["ASAN_OPTIONS"] = ("detect_leaks=" + str(int(expected is not None)) +
                                   ":allocator_may_return_null=1:max_allocation_size_mb=64")
            env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
            reason = None
            stderr = b""
            actual = b""
            try:
                result = subprocess.run([str(driver), str(path)], capture_output=True,
                                        timeout=args.timeout, env=env)
                stderr, actual = result.stderr, result.stdout
                if result.returncode not in (0, 1):
                    reason = f"exit {result.returncode}"
                elif any(marker in stderr for marker in
                         [b"ERROR: AddressSanitizer", b"AddressSanitizer:DEADLYSIGNAL",
                          b"ERROR: LeakSanitizer", b"runtime error:"]):
                    reason = "sanitizer finding"
                elif result.returncode == 1 and b"Error:" not in stderr:
                    reason = "driver invariant failure"
                elif must_reject and result.returncode == 0:
                    reason = "accepted invalid seed"
                elif expected is not None and (result.returncode != 0 or actual != expected):
                    reason = "reference model mismatch"
                elif result.returncode == 1:
                    stats["rejected"] += 1
                else:
                    stats["valid"] += 1
            except subprocess.TimeoutExpired as exc:
                reason, stderr = "timeout", exc.stderr or b""
            stats["cases"] += 1
            if reason:
                stats["failures"] += 1
                digest = hashlib.sha256(data).hexdigest()[:16]
                base = args.output / digest
                base.with_suffix(".kanata").write_bytes(data)
                base.with_suffix(".stderr").write_bytes(stderr)
                base.with_suffix(".actual").write_bytes(actual)
                if expected is not None:
                    base.with_suffix(".expected").write_bytes(expected)
                stats.update(reason=reason, artifact=str(base.with_suffix(".kanata")))
                print(f"FAIL case {index}: {reason}; {base}.kanata", flush=True)
                break
    stats["elapsed_seconds"] = round(time.monotonic() - started, 3)
    (args.output / "summary.json").write_text(json.dumps(stats, indent=2) + "\n")
    print(json.dumps(stats, indent=2))
    return int(stats["failures"] != 0)


if __name__ == "__main__":
    raise SystemExit(main())
