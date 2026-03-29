# =============================================================================
# week12_safety_validation.py  —  Week 12: Safety Validation Layer
#
# WHAT THIS WEEK DOES:
#   Before actually removing any variable, this module applies THREE
#   safety checks to every ML prediction of "DEAD":
#
#   CHECK 1 — Side-Effect Guard
#     If a variable has side_effect_count > 0, it is NEVER removed,
#     regardless of what the ML model predicted. Removing a statement
#     with a side effect (pointer write, I/O) changes program behaviour.
#
#   CHECK 2 — Use-Count Guard
#     If var_use_count > 0, the variable is actually referenced somewhere
#     downstream. Classical liveness would mark it LIVE; the ML model
#     might occasionally mislabel it. This guard catches that.
#
#   CHECK 3 — Classical Liveness Cross-Check
#     The classical liveness label (from dataset.csv, column "label")
#     is compared against the ML prediction. If they disagree for a
#     variable predicted DEAD by ML, the classical result wins and the
#     variable is kept (SAFE_KEEP).
#
# WHY A SAFETY LAYER?
#   ML models are probabilistic — they can be wrong. In a compiler,
#   a wrong dead-code removal BREAKS the program. The safety layer
#   ensures we only remove code that BOTH the ML model AND classical
#   analysis agree is dead, AND that has no side effects, AND that
#   has zero usage count.
#
# OUTPUT:
#   safe_elimination_report.txt — per-variable decisions with reasons
#   safe_optimized_code.txt     — the final optimized program
# =============================================================================

import pandas as pd
import numpy as np
import pickle
import os

print("=" * 60)
print("WEEK 12: SAFETY VALIDATION LAYER")
print("=" * 60)

# ─────────────────────────────────────────────────────────────────────────────
# STEP 1 — Load dataset and trained model
# ─────────────────────────────────────────────────────────────────────────────
CSV_PATH   = "dataset.csv"
MODEL_FILE = "model.pkl"

raw = pd.read_csv(CSV_PATH)
df  = raw[raw["snippet_id"] != "snippet_id"].copy()
df.reset_index(drop=True, inplace=True)

numeric_cols = ["var_decl_count", "var_use_count", "total_statements",
                "total_variables", "cfg_depth", "cfg_nodes",
                "side_effect_count", "usage_ratio"]
for col in numeric_cols:
    df[col] = pd.to_numeric(df[col])

with open(MODEL_FILE, "rb") as f:
    model = pickle.load(f)

print(f"\n[1] Dataset loaded: {len(df)} rows")
print(f"    Model loaded from: '{MODEL_FILE}'")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 2 — Get ML predictions for every variable
# ─────────────────────────────────────────────────────────────────────────────
FEATURE_COLS = numeric_cols
X = df[FEATURE_COLS].values
ml_predictions = model.predict(X)

print(f"\n[2] ML predictions obtained for {len(ml_predictions)} variables")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 3 — Apply three-layer safety validation
#
# For each variable, the final decision is one of:
#   REMOVE      — ML says DEAD AND all safety checks pass
#   SAFE_KEEP   — ML says DEAD but a safety check overrides it → keep
#   KEEP        — ML says LIVE → keep (no safety check needed)
# ─────────────────────────────────────────────────────────────────────────────
print("\n[3] Applying safety checks...")
print("-" * 70)

results = []

for i, row in df.iterrows():
    var_name        = row["variable_name"]
    ml_pred         = ml_predictions[i]
    classical_label = row["label"]          # ground truth from C++ pipeline
    use_count       = int(row["var_use_count"])
    side_effects    = int(row["side_effect_count"])

    # Default: follow ML prediction
    final_decision = ml_pred
    reason         = "ML prediction accepted"
    safe           = True

    if ml_pred == "DEAD":
        # ── CHECK 1: Side-Effect Guard ──────────────────────────────────
        if side_effects > 0:
            final_decision = "SAFE_KEEP"
            reason         = f"OVERRIDE: side_effect_count={side_effects} > 0 (unsafe to remove)"
            safe           = False

        # ── CHECK 2: Use-Count Guard ─────────────────────────────────────
        elif use_count > 0:
            final_decision = "SAFE_KEEP"
            reason         = f"OVERRIDE: var_use_count={use_count} > 0 (variable is referenced)"
            safe           = False

        # ── CHECK 3: Classical Liveness Cross-Check ──────────────────────
        elif classical_label == "LIVE":
            final_decision = "SAFE_KEEP"
            reason         = "OVERRIDE: classical liveness says LIVE (ML disagrees — classical wins)"
            safe           = False

        else:
            # All three checks passed — safe to remove
            final_decision = "REMOVE"
            reason         = "All safety checks passed: use_count=0, no side effects, classical=DEAD"

    else:
        # ML says LIVE — no need to check, just keep
        final_decision = "KEEP"
        reason         = "ML predicts LIVE"

    results.append({
        "variable":        var_name,
        "ml_prediction":   ml_pred,
        "classical_label": classical_label,
        "use_count":       use_count,
        "side_effects":    side_effects,
        "final_decision":  final_decision,
        "safe":            safe,
        "reason":          reason
    })

    # Print to console
    status_icon = "  REMOVE" if final_decision == "REMOVE" else \
                  "  SAFE_KEEP" if final_decision == "SAFE_KEEP" else \
                  "  KEEP"
    print(f"  Variable '{var_name}':")
    print(f"    ML={ml_pred:<6}  Classical={classical_label:<6}  "
          f"use_count={use_count}  side_fx={side_effects}")
    print(f"     {status_icon}")
    print(f"    Reason: {reason}")
    print()

results_df = pd.DataFrame(results)

# ─────────────────────────────────────────────────────────────────────────────
# STEP 4 — Summary statistics
# ─────────────────────────────────────────────────────────────────────────────
n_remove    = (results_df["final_decision"] == "REMOVE").sum()
n_safe_keep = (results_df["final_decision"] == "SAFE_KEEP").sum()
n_keep      = (results_df["final_decision"] == "KEEP").sum()

print("=" * 60)
print("SAFETY VALIDATION SUMMARY")
print("=" * 60)
print(f"  Total variables checked : {len(results_df)}")
print(f"  Safely REMOVED          : {n_remove}")
print(f"  Kept (ML=LIVE)          : {n_keep}")
print(f"  Safety OVERRIDE (kept)  : {n_safe_keep}")
if n_remove + n_safe_keep > 0:
    ml_dead_count = (results_df["ml_prediction"] == "DEAD").sum()
    safety_rate = n_safe_keep / ml_dead_count * 100 if ml_dead_count > 0 else 0
    print(f"  Safety override rate    : {safety_rate:.1f}% of ML-DEAD predictions overridden")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 5 — Generate safe optimized code
#
# Reconstruct the full program WITH int main() wrapper.
# We read lines from optimized_code.txt (C++ DCE output) which contains
# the already-live statements, then further filter out any variables
# the ML+Safety layer decided to REMOVE.
#
# IMPORTANT rules:
#   - "int main()" is ALWAYS kept — it is never a dataset variable.
#   - "return ..." lines are ALWAYS kept.
#   - Only lines of the form "int <exact_var_name> = ...;" are candidates
#     for removal, and only if that exact variable name is in vars_to_remove.
#   - We match the variable name as a whole word so "int b = 5;" is matched
#     by var "b" but "int main()" is NEVER matched by any dataset variable.
# ─────────────────────────────────────────────────────────────────────────────
import re

print("\n[5] Generating safe optimized code...")

# Build the set of variables the ML+Safety layer approved for removal
vars_to_remove = set(
    row["variable"] for _, row in results_df.iterrows()
    if row["final_decision"] == "REMOVE"
)

# Read the C++ optimized_code.txt (contains live statements without main wrapper)
raw_lines = []
if os.path.exists("optimized_code.txt"):
    with open("optimized_code.txt") as f:
        raw_lines = [l.rstrip() for l in f.readlines() if l.strip()]
else:
    # Fallback: reconstruct from dataset rows that are not removed
    for _, row in df.iterrows():
        raw_lines.append(f"int {row['variable_name']} = 0;")
    raw_lines.append("return 0;")

# Filter raw_lines: remove only lines that declare a dead variable.
# A declaration line looks like:  int <varname> = <value>;
# We use a regex to extract the declared variable name and check it
# against vars_to_remove — this avoids any accidental substring match.
body_lines = []
for line in raw_lines:
    # Match exactly:  int <identifier> = <anything> ;
    m = re.match(r'^\s*int\s+([A-Za-z_]\w*)\s*=', line)
    if m:
        declared_var = m.group(1)   # the variable name only (e.g. "b")
        if declared_var in vars_to_remove:
            print(f"  [REMOVE] {line.strip()}  (variable '{declared_var}' is DEAD)")
            continue   # skip this line — it's dead
    body_lines.append(line)

# Now wrap the surviving body inside a proper int main() function
safe_program_lines = []
safe_program_lines.append("int main()")
safe_program_lines.append("{")
for line in body_lines:
    safe_program_lines.append("    " + line.strip())
# Only add return 0 if there is no return statement already in the body
has_return = any(re.match(r'^\s*return\b', ln) for ln in body_lines)
if not has_return:
    safe_program_lines.append("    return 0;")
safe_program_lines.append("}")

print("\n--- SAFE OPTIMIZED PROGRAM ---")
for line in safe_program_lines:
    print(f"  {line}")

with open("safe_optimized_code.txt", "w") as f:
    f.write("--- SAFE OPTIMIZED PROGRAM (Week 12 Safety Validation) ---\n\n")
    for line in safe_program_lines:
        f.write(line + "\n")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 6 — Save detailed safety report
# ─────────────────────────────────────────────────────────────────────────────
REPORT_FILE = "safe_elimination_report.txt"
with open(REPORT_FILE, "w") as rpt:
    rpt.write("=" * 60 + "\n")
    rpt.write("WEEK 12: SAFETY VALIDATION REPORT\n")
    rpt.write("=" * 60 + "\n\n")

    rpt.write("THREE SAFETY CHECKS APPLIED\n")
    rpt.write("-" * 40 + "\n")
    rpt.write("CHECK 1: Side-Effect Guard   — if side_effect_count > 0: KEEP\n")
    rpt.write("CHECK 2: Use-Count Guard     — if var_use_count > 0: KEEP\n")
    rpt.write("CHECK 3: Classical Cross-Check — if classical=LIVE but ML=DEAD: KEEP\n\n")

    rpt.write("PER-VARIABLE DECISIONS\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"{'Variable':<12} {'ML':<8} {'Classical':<12} {'UseCount':<10} {'SideFx':<8} {'Decision':<12} Reason\n")
    rpt.write("-" * 100 + "\n")
    for _, r in results_df.iterrows():
        rpt.write(f"{r['variable']:<12} {r['ml_prediction']:<8} {r['classical_label']:<12} "
                  f"{r['use_count']:<10} {r['side_effects']:<8} {r['final_decision']:<12} {r['reason']}\n")

    rpt.write(f"\nSUMMARY\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"Total variables : {len(results_df)}\n")
    rpt.write(f"REMOVED         : {n_remove}\n")
    rpt.write(f"KEPT (ML=LIVE)  : {n_keep}\n")
    rpt.write(f"SAFE OVERRIDE   : {n_safe_keep}\n\n")

    rpt.write("SAFE OPTIMIZED PROGRAM\n")
    rpt.write("-" * 40 + "\n")
    for line in safe_program_lines:
        rpt.write(line + "\n")

print(f"\n[6] Safety report saved  '{REPORT_FILE}'")
print(f"    Safe program saved   'safe_optimized_code.txt'")
print("\n" + "=" * 60)
print("Week 12 COMPLETE. Outputs: safe_elimination_report.txt, safe_optimized_code.txt")
print("=" * 60)