#!/usr/bin/env python3
"""Differential test for montador.

Generates random *valid* programs (random case, blanks, tabs, comments, CRLF, SECAO/SECTION,
DATA before or after TEXT, labels alone on a line, LABEL + N, SPACE n, signed CONST, EQU/IF)
and computes the expected object independently, in this script. The assembler output must
match byte for byte.

    python3 scripts/differential.py [N]    # default N = 3000 programs
"""
import os, random, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
M = os.path.join(ROOT, "montador" + (".exe" if os.name == "nt" else ""))
N = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
D = tempfile.mkdtemp(prefix="hypoforge-diff-")
OPS = {"ADD":1,"SUB":2,"MULT":3,"DIV":4,"JMP":5,"JMPN":6,"JMPP":7,"JMPZ":8,"COPY":9,"LOAD":10,"STORE":11,"INPUT":12,"OUTPUT":13,"STOP":14}
def case(r, s): return "".join(c.upper() if r.random() < .5 else c.lower() for c in s)
def ws(r): return r.choice([" ", "  ", "\t", " \t "])
def gen(r):
    nd = r.randint(1, 6)
    data = []  # (name, kind, size/value)
    for i in range(nd):
        if r.random() < .5: data.append((f"D{i}", "SPACE", r.randint(1, 4)))
        else: data.append((f"D{i}", "CONST", r.randint(-50, 50) or 1))  # avoid const 0 (DIV check)
    equs = [(f"E{i}", r.randint(0, 1)) for i in range(r.randint(0, 3))]
    # text statements: (label?, op, operands[(name, off)]), plus IF markers
    ni = r.randint(1, 25); text = []; codelabels = [f"L{i}" for i in range(ni)]
    for i in range(ni):
        op = r.choice(list(OPS))
        args = []
        if op in ("JMP","JMPN","JMPP","JMPZ"): args = [(r.choice(codelabels), 0)]
        elif op != "STOP":
            n = 2 if op == "COPY" else 1
            for k in range(n):
                write = op in ("STORE","INPUT") or (op == "COPY" and k == 1)
                cands = [d for d in data if d[1] == "SPACE"] if write else data
                if not cands: op, args = "STOP", []; break
                d = r.choice(cands); off = r.randrange(d[2]) if d[1] == "SPACE" else 0
                args.append((d[0], off))
        guard = r.choice(equs) if equs and r.random() < .2 else None
        text.append((f"L{i}", op, args, guard))
    # addresses
    kept = [t for t in text if t[3] is None or t[3][1] != 0]
    addr = {}; loc = 0
    for lab, op, args, g in kept:
        addr[lab] = loc; loc += 1 + len(args)
    dloc = loc
    for name, kind, v in data:
        addr[name] = dloc; dloc += v if kind == "SPACE" else 1
    # labels of dropped statements might be jump targets: retarget jumps to kept labels
    keptl = [t[0] for t in kept]
    if not keptl: return None
    fixed = []
    for lab, op, args, g in text:
        if op.startswith("JMP"): args = [(a if a in keptl else r.choice(keptl), 0) for a, _ in args]
        fixed.append((lab, op, args, g))
    words = []
    for lab, op, args, g in fixed:
        if g is not None and g[1] == 0: continue
        words.append(OPS[op]); words += [addr[a] + o for a, o in args]
    for name, kind, v in data: words += [0] * v if kind == "SPACE" else [v]
    # render
    out = []
    for name, v in equs: out.append(f"{case(r,name)}:{ws(r)}{case(r,'EQU')} {v}")
    def rtext():
        t = [case(r, r.choice(["SECTION","SECAO"])) + ws(r) + case(r, "TEXT")]
        for lab, op, args, g in fixed:
            if g is not None: t.append(case(r, "IF") + ws(r) + case(r, g[0]))
            a = ws(r).join(case(r, n) + (r.choice([" + ", "+", " +", "+ "]) + str(o) if o or r.random() < .1 else "") for n, o in args)
            line = case(r, op) + (ws(r) + a if a else "")
            if r.random() < .3: t += [case(r, lab) + r.choice([":", " :", ": "]), r.choice(["", "; c", "\t"]) , line]
            else: t.append(case(r, lab) + r.choice([":", ": ", " : "]) + ws(r) + line)
            if r.random() < .2: t[-1] += ws(r) + "; comentario COPY A, B"
        return t
    def rdata():
        t = [case(r, r.choice(["SECTION","SECAO"])) + ws(r) + case(r, "DATA")]
        for name, kind, v in data:
            if kind == "SPACE": arg = "" if v == 1 and r.random() < .5 else ws(r) + str(v)
            else: arg = ws(r) + (str(v) if v < 0 or r.random() < .5 else "+" + str(v))
            t.append(case(r, name) + ":" + ws(r) + case(r, kind) + arg)
        return t
    out += rtext() + rdata() if r.random() < .5 else rdata() + rtext()
    if r.random() < .3: out = [ws(r) + l for l in out]
    nl = r.choice(["\n", "\r\n"])
    return nl.join(out) + (nl if r.random() < .5 else ""), " ".join(map(str, words)) + (" " if words else "")
r = random.Random(42); bad = 0; n = 0
while n < N:
    g = gen(r)
    if g is None: continue
    src, exp = g; n += 1
    p = os.path.join(D, "p.asm"); open(p, "w", newline="").write(src)
    res = subprocess.run([M, p], capture_output=True, text=True)
    got = open(os.path.join(D, "p.OBJ")).read() if res.returncode == 0 else None
    if got != exp:
        bad += 1
        if bad <= 3: print("MISMATCH\n", src, "\nexpected:", exp, "\ngot:", got, res.stdout[-500:])
print(f"differential: {N} random valid programs, {bad} mismatches")
if bad:
    print('failing inputs kept in', D)
    sys.exit(1)
shutil.rmtree(D, ignore_errors=True)
