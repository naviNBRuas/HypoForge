#!/usr/bin/env bash
# Runs the whole test-suite: assembler (valid/invalid), linker and end-to-end runs
# on the course simulator (skipped when the simulator binary cannot be executed).
#
#   make test            # or: bash scripts/run_tests.sh
#   SIM=/path/to/simulador bash scripts/run_tests.sh

set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MONT="$ROOT/montador"
LIG="$ROOT/ligador"
SIM="${SIM:-$ROOT/tools/simulador/simulador}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

pass=0; fail=0; skipped=0
ok()   { pass=$((pass+1)); printf '  \033[32mPASS\033[0m %s\n' "$1"; }
bad()  { fail=$((fail+1)); printf '  \033[31mFAIL\033[0m %s\n' "$1"; [ -n "${2:-}" ] && printf '%s\n' "$2" | sed 's/^/       /'; }

# The simulator prints "cannot read input file" when it starts but gets a bad path.
have_sim=0
if [ -x "$SIM" ]; then
  case "$(timeout 5 "$SIM" /nonexistent 2>&1 < /dev/null)" in *"cannot read input"*) have_sim=1 ;; esac
fi

echo "== montador: valid programs (object compared byte by byte)"
for src in "$ROOT"/tests/valid/*.asm; do
  name="$(basename "$src" .asm)"
  cp "$src" "$WORK/$name.asm"
  out="$("$MONT" "$WORK/$name.asm" 2>&1)"; rc=$?
  if [ $rc -ne 0 ]; then bad "$name (montador failed)" "$out"; continue; fi
  if cmp -s "$WORK/$name.OBJ" "$ROOT/tests/valid/$name.expected.OBJ"; then ok "$name"
  else bad "$name" "expected: $(cat "$ROOT/tests/valid/$name.expected.OBJ")"$'\n'"got:      $(cat "$WORK/$name.OBJ")"; fi
done

echo "== montador: invalid programs (line + error type)"
for src in "$ROOT"/tests/invalid/*.asm; do
  name="$(basename "$src" .asm)"
  cp "$src" "$WORK/$name.asm"; rm -f "$WORK/$name.OBJ"
  out="$("$MONT" "$WORK/$name.asm" 2>&1)"; rc=$?
  got="$(printf '%s\n' "$out" | sed -n -e 's/^\[ERRO \([A-Z]*\)\] linha \([0-9]*\):.*/\2 \1/p' -e 's/^\[ERRO \([A-Z]*\)\] arquivo:.*/0 \1/p')"
  want="$(cat "$ROOT/tests/invalid/$name.expected")"
  if [ $rc -eq 1 ] && [ "$got" = "$want" ] && [ ! -e "$WORK/$name.OBJ" ]; then ok "$name"
  else bad "$name (rc=$rc)" "expected:"$'\n'"$want"$'\n'"got:"$'\n'"$got"; fi
done

echo "== ligador: linking"
for dir in "$ROOT"/tests/link/*/; do
  name="$(basename "$dir")"
  rm -rf "$WORK/link_$name"; mkdir -p "$WORK/link_$name"; cp "$dir"/*.asm "$WORK/link_$name/"
  mods=(); for m in $(cat "$dir/modules"); do mods+=("$WORK/link_$name/$m"); done
  first="${mods[0]%.asm}"
  out="$("$LIG" "${mods[@]}" 2>&1)"; rc=$?
  if [ -f "$dir/expected.errors" ]; then
    good=1; [ $rc -eq 1 ] || good=0
    while IFS= read -r frag; do printf '%s' "$out" | grep -qF -- "$frag" || good=0; done < "$dir/expected.errors"
    [ ! -e "$first.LIG" ] || good=0
    if [ $good -eq 1 ]; then ok "$name (link error reported)"; else bad "$name" "$out"; fi
    continue
  fi
  if [ $rc -ne 0 ]; then bad "$name (ligador failed)" "$out"; continue; fi
  if cmp -s "$first.LIG" "$dir/expected.LIG"; then ok "$name (.LIG identical)"
  else bad "$name" "expected: $(cat "$dir/expected.LIG")"$'\n'"got:      $(cat "$first.LIG")"; continue; fi
  if [ -f "$dir/input" ]; then
    if [ $have_sim -eq 1 ]; then
      res="$(timeout 5 "$SIM" "$first.LIG" < "$dir/input" 2>&1)"
      if [ "$(echo $res)" = "$(echo $(cat "$dir/output"))" ]; then ok "$name (simulator output)"
      else bad "$name (simulator output)" "expected: $(echo $(cat "$dir/output"))"$'\n'"got:      $(echo $res)"; fi
    else skipped=$((skipped+1)); echo "  SKIP $name (simulator unavailable)"; fi
  fi
done

echo "== end-to-end: assemble + run on the course simulator"
for src in "$ROOT"/tests/sim/*.asm; do
  name="$(basename "$src" .asm)"
  if [ $have_sim -ne 1 ]; then skipped=$((skipped+1)); echo "  SKIP $name (simulator unavailable)"; continue; fi
  cp "$src" "$WORK/$name.asm"
  "$MONT" "$WORK/$name.asm" >/dev/null 2>&1 || { bad "$name (montador failed)"; continue; }
  res="$(timeout 5 "$SIM" "$WORK/$name.OBJ" < "$ROOT/tests/sim/$name.in" 2>&1)"
  if [ "$(echo $res)" = "$(echo $(cat "$ROOT/tests/sim/$name.out"))" ]; then ok "$name"
  else bad "$name" "expected: $(echo $(cat "$ROOT/tests/sim/$name.out"))"$'\n'"got:      $(echo $res)"; fi
done

echo "== command line: file names, arguments and stale outputs"
CLI="$WORK/cli"; mkdir -p "$CLI/dir.v1/sub dir"
prog=$'SECTION TEXT\nSTOP\n'
printf '%s' "$prog" > "$CLI/dir.v1/sub dir/my.prog.asm"
"$MONT" "$CLI/dir.v1/sub dir/my.prog.asm" >/dev/null 2>&1
if [ -f "$CLI/dir.v1/sub dir/my.prog.OBJ" ]; then ok "dots and blanks in the path -> my.prog.OBJ"; else bad "dots and blanks in the path"; fi
printf '%s' "$prog" > "$CLI/UPPER.ASM"; "$MONT" "$CLI/UPPER.ASM" >/dev/null 2>&1
if [ -f "$CLI/UPPER.OBJ" ]; then ok "upper-case .ASM extension -> UPPER.OBJ"; else bad "upper-case .ASM extension"; fi
printf '%s' "$prog" > "$CLI/semext.asm"; "$MONT" "$CLI/semext" >/dev/null 2>&1
if [ -f "$CLI/semext.OBJ" ]; then ok "name without extension finds semext.asm"; else bad "name without extension"; fi
printf 'SECTION TEXT\nSTOP X\n' > "$CLI/UPPER.ASM"; "$MONT" "$CLI/UPPER.ASM" >/dev/null 2>&1; rc=$?
if [ $rc -eq 1 ] && [ ! -e "$CLI/UPPER.OBJ" ]; then ok "failed assembly removes the stale .OBJ"; else bad "stale .OBJ (rc=$rc)"; fi
"$MONT" "$CLI/nao_existe.asm" >/dev/null 2>&1; rc=$?
if [ $rc -eq 2 ]; then ok "missing input file -> exit 2"; else bad "missing input file (rc=$rc)"; fi
"$MONT" >/dev/null 2>&1; rc1=$?; "$MONT" a.asm b.asm >/dev/null 2>&1; rc2=$?
if [ $rc1 -eq 2 ] && [ $rc2 -eq 2 ]; then ok "montador usage errors -> exit 2"; else bad "montador usage (rc=$rc1/$rc2)"; fi
"$LIG" >/dev/null 2>&1; rc1=$?; "$LIG" a.asm b.asm c.asm d.asm >/dev/null 2>&1; rc2=$?
if [ $rc1 -eq 2 ] && [ $rc2 -eq 2 ]; then ok "ligador with 0 or 4 modules -> exit 2"; else bad "ligador usage (rc=$rc1/$rc2)"; fi
cp "$ROOT"/tests/link/two_modules/*.asm "$CLI/"
(cd "$CLI" && "$LIG" modulo1.asm modulo2.asm >/dev/null 2>&1)
if cmp -s "$CLI/modulo1.LIG" "$ROOT/tests/link/two_modules/expected.LIG"; then ok "ligador run from another directory"; else bad "ligador from another directory"; fi
printf 'M: BEGIN\nSECTION TEXT\nSTOP X\nEND\n' > "$CLI/modulo2.asm"
(cd "$CLI" && "$LIG" modulo1.asm modulo2.asm >/dev/null 2>&1); rc=$?
if [ $rc -eq 1 ] && [ ! -e "$CLI/modulo1.LIG" ]; then ok "failed linking removes the stale .LIG"; else bad "stale .LIG (rc=$rc)"; fi

echo
echo "Result: $pass passed, $fail failed, $skipped skipped"
[ $fail -eq 0 ]
