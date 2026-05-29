#!/usr/bin/env python3
"""
NTAprep Scoring Engine (Python)
================================
Export your test result JSON from the browser (open DevTools → Application →
Session Storage → copy ntaprep_result) and save it as result.json.
Then run:  python3 score_engine.py result.json

Features:
  - Full JEE Main / JEE Advanced / NEET marking scheme support
  - MCQ (+4/−1) and MSQ (+4/partial/−2) scoring
  - Subject-wise and section-wise breakdown
  - Percentile estimation against a sample population
  - Detailed per-question report
  - Exports to CSV for further analysis
"""

import json, sys, csv, math, os
from datetime import datetime

# ─── Marking Schemes ────────────────────────────────────────────────────────
SCHEMES = {
    "jee_main":    {"mcq": (4, -1),  "msq": (4, -2, 1)},   # +4/-1 MCQ, +4/-2/+1partial MSQ
    "jee_advanced":{"mcq": (3, -1),  "msq": (4, -2, 1)},   # JEE Adv MCQ +3/-1
    "neet":        {"mcq": (4, -1),  "msq": (4, -1, 0)},   # NEET only MCQ
}

LABELS = ["A", "B", "C", "D"]
SUBJ_CLR = {"Physics": "\033[94m", "Chemistry": "\033[92m", "Mathematics": "\033[95m"}
RESET = "\033[0m"
BOLD  = "\033[1m"
GREEN = "\033[92m"
RED   = "\033[91m"
YELLOW= "\033[93m"
CYAN  = "\033[96m"

def color(text, code):
    return f"{code}{text}{RESET}"

def fmt_ans(a, qtype):
    if a is None: return "—"
    if qtype == "msq":
        arr = a if isinstance(a, list) else [a]
        return ", ".join(LABELS[i] for i in arr if 0 <= i < 4)
    return LABELS[a] if isinstance(a, int) and 0 <= a < 4 else "—"

def question_result(q, scheme="jee_main"):
    ua = q.get("userAnswer")
    ck = q.get("correctAnswer")
    qtype = q.get("qtype", "mcq")
    if ua is None or ck is None:
        return "skipped", 0
    s = SCHEMES.get(scheme, SCHEMES["jee_main"])
    if qtype == "msq":
        pos, neg, partial = s["msq"]
        u_set = set(ua if isinstance(ua, list) else [ua])
        c_set = set(ck if isinstance(ck, list) else [ck])
        if u_set == c_set:
            return "correct", pos
        if u_set.issubset(c_set) and len(u_set) < len(c_set):
            return "partial", len(u_set) * partial
        return "wrong", neg
    else:
        pos, neg = s["mcq"]
        return ("correct", pos) if ua == ck else ("wrong", neg)

def calculate(data, scheme="jee_main"):
    questions = data.get("questions", [])
    SUBJS = data.get("SUBJS", [])
    sScore = {s: 0 for s in SUBJS}
    sCorr  = {s: 0 for s in SUBJS}
    sWrong = {s: 0 for s in SUBJS}
    sSkip  = {s: 0 for s in SUBJS}
    sTotal = {s: 0 for s in SUBJS}
    details = []
    total = 0

    for gi, q in enumerate(questions):
        subj = q.get("subject", "Unknown")
        sTotal[subj] = sTotal.get(subj, 0) + 1
        res, sc = question_result(q, scheme)
        total += sc
        sScore[subj] = sScore.get(subj, 0) + sc
        details.append({
            "num": gi + 1, "subject": subj, "qtype": q.get("qtype", "mcq"),
            "user": fmt_ans(q.get("userAnswer"), q.get("qtype", "mcq")),
            "correct": fmt_ans(q.get("correctAnswer"), q.get("qtype", "mcq")),
            "result": res, "score": sc
        })
        if res == "correct":   sCorr[subj]  = sCorr.get(subj,0)  + 1
        elif res == "wrong":   sWrong[subj] = sWrong.get(subj,0) + 1
        else:                  sSkip[subj]  = sSkip.get(subj,0)  + 1

    total_correct = sum(sCorr.values())
    total_wrong   = sum(sWrong.values())
    total_skip    = sum(sSkip.values())
    attempted     = total_correct + total_wrong
    accuracy      = round(total_correct / attempted * 100, 1) if attempted > 0 else 0
    max_score     = len(questions) * 4

    return {
        "total": total, "maxScore": max_score, "accuracy": accuracy,
        "correct": total_correct, "wrong": total_wrong, "skipped": total_skip,
        "sScore": sScore, "sCorr": sCorr, "sWrong": sWrong, "sSkip": sSkip,
        "sTotal": sTotal, "SUBJS": SUBJS, "details": details,
        "timeUsed": data.get("timeUsed", 0), "name": data.get("name", "Mock Test")
    }

def fmt_time(s):
    h, rem = divmod(int(s), 3600)
    m, sec = divmod(rem, 60)
    return f"{h}h {m}m {sec}s" if h else f"{m}m {sec}s"

def percentile_estimate(score, max_score):
    """Rough JEE Main percentile estimation (based on historical data patterns)"""
    pct = score / max_score * 100
    # Sigmoid-like mapping calibrated to JEE Main distributions
    if score <= 0:   return 1.0
    if score >= 250: return 99.9
    p = 1 / (1 + math.exp(-0.05 * (score - 110)))
    return round(min(99.9, max(1.0, p * 100)), 2)

def print_report(r):
    name = r["name"]
    bar = "═" * 60
    print(f"\n{BOLD}{CYAN}{bar}{RESET}")
    print(f"  {BOLD}NTAprep Score Report  —  {name}{RESET}")
    print(f"{BOLD}{CYAN}{bar}{RESET}")

    # Score
    pct_est = percentile_estimate(r["total"], r["maxScore"])
    sc_clr  = GREEN if r["total"] > r["maxScore"]*0.5 else (YELLOW if r["total"] > 0 else RED)
    print(f"\n  {BOLD}Total Score  : {color(str(r['total']), sc_clr)} / {r['maxScore']}{RESET}")
    print(f"  Accuracy     : {r['accuracy']}%")
    print(f"  Percentile   : ~{pct_est} (estimated, JEE Main pattern)")
    print(f"  Time Used    : {fmt_time(r['timeUsed'])}")
    print(f"  Correct      : {color(str(r['correct']), GREEN)}  |  "
          f"Wrong: {color(str(r['wrong']), RED)}  |  "
          f"Skipped: {color(str(r['skipped']), YELLOW)}")

    # Subject breakdown
    print(f"\n  {BOLD}Subject-wise Breakdown:{RESET}")
    print(f"  {'Subject':<14} {'Score':>7} {'Corr':>6} {'Wrong':>6} {'Skip':>6} {'Acc':>7}")
    print(f"  {'─'*14} {'─'*7} {'─'*6} {'─'*6} {'─'*6} {'─'*7}")
    for s in r["SUBJS"]:
        max_s = r["sTotal"].get(s, 0) * 4
        att   = r["sCorr"].get(s, 0) + r["sWrong"].get(s, 0)
        acc_s = round(r["sCorr"].get(s,0) / att * 100, 1) if att > 0 else 0
        sc_s  = r["sScore"].get(s, 0)
        clr   = SUBJ_CLR.get(s, "")
        print(f"  {color(f'{s:<14}', clr)} {sc_s:>6}/{max_s:<4} "
              f"{r['sCorr'].get(s,0):>6} {r['sWrong'].get(s,0):>6} "
              f"{r['sSkip'].get(s,0):>6} {acc_s:>6}%")

    # Per-question table
    print(f"\n  {BOLD}Question-by-Question Review:{RESET}")
    print(f"  {'Q':>3}  {'Subj':<8} {'Type':<5} {'Your':<8} {'Key':<8} {'Result':<10} {'Score':>6}")
    print(f"  {'─'*3}  {'─'*8} {'─'*5} {'─'*8} {'─'*8} {'─'*10} {'─'*6}")
    for d in r["details"]:
        res_clr = GREEN if d["result"]=="correct" else (RED if d["result"]=="wrong" else YELLOW)
        sc_str = f"+{d['score']}" if d["score"] > 0 else str(d["score"])
        print(f"  {d['num']:>3}  {d['subject'][:8]:<8} {d['qtype']:<5} "
              f"{d['user']:<8} {d['correct']:<8} "
              f"{color(f\"{d['result']:<10}\", res_clr)} "
              f"{color(f'{sc_str:>6}', res_clr)}")

    print(f"\n{BOLD}{CYAN}{bar}{RESET}\n")

def export_csv(r, path="result_export.csv"):
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["num","subject","qtype","user_answer","correct_answer","result","score"])
        w.writeheader()
        for d in r["details"]:
            w.writerow({"num":d["num"],"subject":d["subject"],"qtype":d["qtype"],
                        "user_answer":d["user"],"correct_answer":d["correct"],
                        "result":d["result"],"score":d["score"]})
    print(f"  📄 CSV exported → {path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 score_engine.py <result.json> [scheme]")
        print("       scheme: jee_main (default) | jee_advanced | neet")
        sys.exit(1)

    path = sys.argv[1]
    scheme = sys.argv[2] if len(sys.argv) > 2 else "jee_main"

    if not os.path.exists(path):
        print(f"File not found: {path}")
        sys.exit(1)

    with open(path) as f:
        data = json.load(f)

    r = calculate(data, scheme)
    print_report(r)

    # Export CSV
    csv_path = path.replace(".json", "_report.csv")
    export_csv(r, csv_path)

    # Also write a summary JSON
    summary = {
        "test": r["name"], "scheme": scheme,
        "score": r["total"], "maxScore": r["maxScore"],
        "accuracy": r["accuracy"], "correct": r["correct"],
        "wrong": r["wrong"], "skipped": r["skipped"],
        "timeUsed": fmt_time(r["timeUsed"]),
        "estimatedPercentile": percentile_estimate(r["total"], r["maxScore"]),
        "subjectWise": {s: {"score":r["sScore"].get(s,0),"correct":r["sCorr"].get(s,0),
                            "wrong":r["sWrong"].get(s,0),"skipped":r["sSkip"].get(s,0)}
                        for s in r["SUBJS"]},
        "generatedAt": datetime.now().isoformat()
    }
    summary_path = path.replace(".json", "_summary.json")
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2)
    print(f"  📊 Summary JSON → {summary_path}\n")

if __name__ == "__main__":
    main()
