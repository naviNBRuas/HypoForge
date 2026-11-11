#!/usr/bin/env python3
"""Robustness fuzzing for montador and ligador.

Feeds thousands of random byte strings, token soups and mutated test files to the
assembler, plus random (and mutated) module combinations to the linker, and fails if
any run crashes, times out or prints a sanitizer report. Exit codes 0 (assembled) and
1 (errors reported) are both acceptable outcomes.

    python3 scripts/fuzz.py [N]          # default N = 6000 programs (N/10 links)
    make stress                          # fuzz + differential test

For memory checks, build first with:
    make clean && make CXXFLAGS="-std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all"
"""
import glob, os, random, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = ".exe" if os.name == "nt" else ""
M = os.path.join(ROOT, "montador" + EXE)
L = os.path.join(ROOT, "ligador" + EXE)
N = int(sys.argv[1]) if len(sys.argv) > 1 else 6000
D = tempfile.mkdtemp(prefix="hypoforge-fuzz-")
os.chdir(ROOT)
seeds = [open(f, "rb").read() for f in glob.glob("tests/**/*.asm", recursive=True)]
words = [b"ADD",b"SUB",b"MULT",b"DIV",b"JMP",b"JMPN",b"JMPP",b"JMPZ",b"COPY",b"LOAD",b"STORE",b"INPUT",b"OUTPUT",b"STOP",
         b"SECTION",b"SECAO",b"TEXT",b"DATA",b"SPACE",b"CONST",b"EQU",b"IF",b"BEGIN",b"END",b"PUBLIC",b"EXTERN",
         b"X",b"Y",b"L1",b"A",b":",b"+",b"-",b",",b";",b"0",b"1",b"-5",b"65536",b"99999999999",b"\t",b"\r",b"\n",b"\xc3\xa7",b"\x00",b"@"]
def gen(r):
    k = r.random()
    if k < 0.4:  # mutate a real test file
        b = bytearray(r.choice(seeds))
        for _ in range(r.randint(1, 8)):
            op = r.randint(0, 2); pos = r.randint(0, max(0, len(b)))
            if op == 0 and b: del b[pos % len(b)]
            elif op == 1: b[pos:pos] = r.choice(words) + b" "
            else: b[pos:pos] = bytes([r.randint(0, 255)])
        return bytes(b)
    if k < 0.9:  # token soup that looks like a program
        lines = []
        for _ in range(r.randint(0, 40)):
            lines.append(b" ".join(r.choice(words) for _ in range(r.randint(0, 6))))
        return b"\n".join(lines)
    return bytes(r.randint(0, 255) for _ in range(r.randint(0, 400)))
r = random.Random(1234); bad = 0; okc = 0
for i in range(N):
    p = os.path.join(D, f"f{i%50}.asm"); open(p, "wb").write(gen(r))
    res = subprocess.run([M, p], capture_output=True, timeout=10)
    if res.returncode not in (0, 1) or b"Sanitizer" in res.stderr or b"runtime error" in res.stderr:
        bad += 1; open(os.path.join(D, f"CRASH{bad}.asm"), "wb").write(open(p, "rb").read()); print("CRASH", res.returncode, res.stderr[-300:])
    okc += res.returncode == 0
print(f"assembler fuzz: {N} programs, {okc} assembled, {N-okc} rejected, {bad} crashes")
# linker: random combinations of real module files plus mutated ones
mods = [f for f in glob.glob("tests/link/**/*.asm", recursive=True)] + glob.glob("examples/modulo*.asm")
lbad = 0
for i in range(N // 10):
    picks = []
    for j in range(r.randint(1, 3)):
        p = os.path.join(D, f"m{j}.asm"); src = bytearray(open(r.choice(mods), "rb").read())
        if r.random() < 0.3 and src: src[r.randrange(len(src))] = r.choice(b"XYZ:+ \n0")
        open(p, "wb").write(src); picks.append(p)
    res = subprocess.run([L] + picks, capture_output=True, timeout=20)
    if res.returncode not in (0, 1) or b"Sanitizer" in res.stderr + res.stdout or b"runtime error" in res.stderr + res.stdout:
        lbad += 1; print("LINK CRASH", res.returncode, (res.stdout + res.stderr)[-400:])
print(f"linker fuzz: {N//10} links, {lbad} crashes")
if bad or lbad:
    print('failing inputs kept in', D)
    sys.exit(1)
shutil.rmtree(D, ignore_errors=True)
