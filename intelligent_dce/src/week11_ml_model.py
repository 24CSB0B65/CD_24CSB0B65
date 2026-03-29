# =============================================================================
# week11_ml_model.py  —  Week 11: ML Model Training & Evaluation
#
# WHAT THIS WEEK DOES:
#   Reads the labeled dataset (dataset.csv) produced by Week 10 (C++ pipeline),
#   trains a Decision Tree classifier to predict whether a variable is
#   LIVE or DEAD, evaluates it, and saves the trained model + report.
#
# WHY DECISION TREE?
#   - Handles small datasets well (we have only 4 rows from 1 snippet,
#     but the code is designed to scale as more snippets are added).
#   - Produces human-readable rules — you can literally print the tree
#     and explain WHY a variable was predicted LIVE or DEAD.
#   - No feature scaling needed (unlike SVM or KNN).
#   - Directly matches compiler intuition: "if var_use_count > 0 → LIVE"
#
# PIPELINE POSITION:
#   C++ (Weeks 1-10) → dataset.csv → [THIS FILE] → model.pkl + report
# =============================================================================

import pandas as pd
import numpy as np
import pickle
import os
from sklearn.tree import DecisionTreeClassifier, export_text
from sklearn.naive_bayes import GaussianNB
from sklearn.model_selection import LeaveOneOut, cross_val_score
from sklearn.metrics import (accuracy_score, precision_score,
                             recall_score, f1_score,
                             classification_report, confusion_matrix)

# ─────────────────────────────────────────────────────────────────────────────
# STEP 1 — Load and clean dataset.csv
# ─────────────────────────────────────────────────────────────────────────────
print("=" * 60)
print("WEEK 11: ML MODEL TRAINING & EVALUATION")
print("=" * 60)

CSV_PATH = "dataset.csv"

# Read CSV — skip duplicate header rows (a known issue when saveDatasetCSV
# is called multiple times with writeHeader=true)
raw = pd.read_csv(CSV_PATH)

# Drop any row where 'snippet_id' is literally the string "snippet_id"
# (duplicate header rows become data rows when pandas reads the file)
df = raw[raw["snippet_id"] != "snippet_id"].copy()
df.reset_index(drop=True, inplace=True)

# Convert all feature columns to numeric
numeric_cols = ["var_decl_count", "var_use_count", "total_statements",
                "total_variables", "cfg_depth", "cfg_nodes",
                "side_effect_count", "usage_ratio"]
for col in numeric_cols:
    df[col] = pd.to_numeric(df[col])

print(f"\n[1] Dataset loaded: {len(df)} rows, {df['label'].value_counts().to_dict()}")
print("\nFull Dataset:")
print(df.to_string(index=False))

# ─────────────────────────────────────────────────────────────────────────────
# STEP 2 — Prepare features (X) and labels (y)
# ─────────────────────────────────────────────────────────────────────────────
FEATURE_COLS = numeric_cols   # all 8 numeric columns are features
X = df[FEATURE_COLS].values
y = df["label"].values        # "LIVE" or "DEAD"

print(f"\n[2] Features used: {FEATURE_COLS}")
print(f"    X shape: {X.shape},  Classes: {np.unique(y)}")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 3 — Train Decision Tree classifier
#
# max_depth=3: keeps the tree small and readable for a small dataset.
# random_state=42: reproducibility.
# class_weight='balanced': compensates if LIVE/DEAD counts are unequal.
# ─────────────────────────────────────────────────────────────────────────────
print("\n[3] Training Decision Tree Classifier...")

dt_model = DecisionTreeClassifier(
    max_depth=3,
    random_state=42,
    class_weight="balanced"
)
dt_model.fit(X, y)

# Also train Gaussian Naive Bayes for comparison
gnb_model = GaussianNB()
gnb_model.fit(X, y)

print("    Decision Tree: TRAINED")
print("    Gaussian Naive Bayes: TRAINED (for comparison)")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 4 — Evaluate on training set
#
# NOTE: With only 4 samples we evaluate on the training set AND use
# Leave-One-Out Cross Validation (LOO-CV) which is the correct strategy
# for tiny datasets. LOO-CV trains on N-1 samples, tests on 1, repeating
# N times — gives an unbiased estimate of generalization.
# ─────────────────────────────────────────────────────────────────────────────
print("\n[4] Evaluating models...")

# --- Training set predictions ---
y_pred_dt  = dt_model.predict(X)
y_pred_gnb = gnb_model.predict(X)

def print_metrics(name, y_true, y_pred):
    acc  = accuracy_score(y_true, y_pred)
    # zero_division=0 avoids warnings when a class has no predictions
    prec = precision_score(y_true, y_pred, pos_label="LIVE", zero_division=0)
    rec  = recall_score(y_true, y_pred, pos_label="LIVE", zero_division=0)
    f1   = f1_score(y_true, y_pred, pos_label="LIVE", zero_division=0)
    print(f"\n  [{name}]")
    print(f"    Accuracy  : {acc:.4f}  ({int(acc*len(y_true))}/{len(y_true)} correct)")
    print(f"    Precision : {prec:.4f}  (of predicted LIVE, how many are truly LIVE)")
    print(f"    Recall    : {rec:.4f}  (of all true LIVE, how many were found)")
    print(f"    F1 Score  : {f1:.4f}  (harmonic mean of precision & recall)")
    return acc, prec, rec, f1

acc_dt,  prec_dt,  rec_dt,  f1_dt  = print_metrics("Decision Tree (train set)", y, y_pred_dt)
acc_gnb, prec_gnb, rec_gnb, f1_gnb = print_metrics("Naive Bayes  (train set)", y, y_pred_gnb)

# --- Leave-One-Out Cross Validation ---
loo = LeaveOneOut()
loo_scores_dt  = cross_val_score(dt_model,  X, y, cv=loo, scoring="accuracy")
loo_scores_gnb = cross_val_score(gnb_model, X, y, cv=loo, scoring="accuracy")

print(f"\n  [LOO-CV Accuracy]")
print(f"    Decision Tree : {loo_scores_dt.mean():.4f}  (avg over {len(loo_scores_dt)} folds)")
print(f"    Naive Bayes   : {loo_scores_gnb.mean():.4f}  (avg over {len(loo_scores_gnb)} folds)")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 5 — Print human-readable decision tree rules
# ─────────────────────────────────────────────────────────────────────────────
print("\n[5] Decision Tree Rules (how the model makes decisions):")
print("-" * 50)
tree_rules = export_text(dt_model, feature_names=FEATURE_COLS)
print(tree_rules)

# ─────────────────────────────────────────────────────────────────────────────
# STEP 6 — Per-variable prediction report
# ─────────────────────────────────────────────────────────────────────────────
print("\n[6] Per-Variable Prediction Report:")
print("-" * 60)
print(f"{'Variable':<12} {'True Label':<12} {'DT Pred':<12} {'GNB Pred':<12} {'Match?'}")
print("-" * 60)
for i, row in df.iterrows():
    true_lbl  = row["label"]
    dt_pred   = y_pred_dt[i]
    gnb_pred  = y_pred_gnb[i]
    match     = "YES" if dt_pred == true_lbl else "NO"
    print(f"{row['variable_name']:<12} {true_lbl:<12} {dt_pred:<12} {gnb_pred:<12} {match}")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 7 — Save the trained Decision Tree model (pickle)
# ─────────────────────────────────────────────────────────────────────────────
MODEL_FILE = "model.pkl"
with open(MODEL_FILE, "wb") as f:
    pickle.dump(dt_model, f)
print(f"\n[7] Trained model saved to '{MODEL_FILE}'")

# ─────────────────────────────────────────────────────────────────────────────
# STEP 8 — Save full evaluation report to ml_report.txt
# ─────────────────────────────────────────────────────────────────────────────
REPORT_FILE = "ml_report.txt"
with open(REPORT_FILE, "w") as rpt:
    rpt.write("=" * 60 + "\n")
    rpt.write("WEEK 11: ML MODEL EVALUATION REPORT\n")
    rpt.write("=" * 60 + "\n\n")

    rpt.write("DATASET SUMMARY\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"Total samples : {len(df)}\n")
    rpt.write(f"LIVE samples  : {(y == 'LIVE').sum()}\n")
    rpt.write(f"DEAD samples  : {(y == 'DEAD').sum()}\n\n")

    rpt.write("DECISION TREE METRICS (training set)\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"Accuracy  : {acc_dt:.4f}\n")
    rpt.write(f"Precision : {prec_dt:.4f}\n")
    rpt.write(f"Recall    : {rec_dt:.4f}\n")
    rpt.write(f"F1 Score  : {f1_dt:.4f}\n\n")

    rpt.write("NAIVE BAYES METRICS (training set)\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"Accuracy  : {acc_gnb:.4f}\n")
    rpt.write(f"Precision : {prec_gnb:.4f}\n")
    rpt.write(f"Recall    : {rec_gnb:.4f}\n")
    rpt.write(f"F1 Score  : {f1_gnb:.4f}\n\n")

    rpt.write("LOO-CV ACCURACY\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"Decision Tree : {loo_scores_dt.mean():.4f}\n")
    rpt.write(f"Naive Bayes   : {loo_scores_gnb.mean():.4f}\n\n")

    rpt.write("DECISION TREE RULES\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(tree_rules + "\n")

    rpt.write("CLASSIFICATION REPORT (Decision Tree)\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(classification_report(y, y_pred_dt, zero_division=0))

    rpt.write("\nPER-VARIABLE PREDICTIONS\n")
    rpt.write("-" * 40 + "\n")
    rpt.write(f"{'Variable':<12} {'True':<8} {'DT Pred':<10} {'GNB Pred':<10} {'Correct?'}\n")
    for i, row in df.iterrows():
        match = "YES" if y_pred_dt[i] == row["label"] else "NO"
        rpt.write(f"{row['variable_name']:<12} {row['label']:<8} {y_pred_dt[i]:<10} {y_pred_gnb[i]:<10} {match}\n")

print(f"[8] Evaluation report saved to '{REPORT_FILE}'")
print("\n" + "=" * 60)
print("Week 11 COMPLETE. Outputs: model.pkl, ml_report.txt")
print("=" * 60)