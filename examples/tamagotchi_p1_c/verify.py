# verify.py
# created: Jan 30, 2025
# last modified: Feb 12, 2025
# James Ezra Seitenschlag
import sys, os

orig_path = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "..", "tamagotcha", "tama_extracted", "tama.bin"))
gen_path = os.path.normpath(os.path.join(os.path.dirname(__file__), "tamagotchi_p1.bin"))

if not os.path.exists(orig_path):
    print(f"[ERROR] Original ROM not found: {orig_path}")
    sys.exit(1)

if not os.path.exists(gen_path):
    print(f"[ERROR] Generated binary not found: {gen_path}")
    sys.exit(1)

with open(orig_path, "rb") as f:
    orig = f.read()
with open(gen_path, "rb") as f:
    gen = f.read()

if orig == gen:
    print("=====================================================================")
    print("  [SUCCESS] 100% BYTE-IDENTICAL TO ORIGINAL BANDAI ROM (tama.bin)!")
    print(f"  Size: {len(gen):,} Bytes ({len(gen)//2:,} 12-Bit words)")
    print("  Differences: 0 bytes")
    print("=====================================================================")
    sys.exit(0)
else:
    diffs = [i for i in range(min(len(orig), len(gen))) if orig[i] != gen[i]]
    print(f"[ERROR] {len(diffs)} bytes differ! (First difference at byte {diffs[0] if diffs else 'N/A'})")
    sys.exit(1)
