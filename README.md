# NTAprep — PDF to CBT Platform

## 🌐 Web App (Open in Browser)

| File | Purpose |
|------|---------|
| `index.html` | Landing page |
| `maker.html` | **PDF Upload + Question Cropping Tool** |
| `test.html` | Fullscreen CBT Exam Interface |
| `answerkey.html` | Post-exam Answer Key Entry |
| `results.html` | Detailed Results + Question Review |

### How to Use
1. Extract this ZIP — open `index.html` in Chrome/Edge/Firefox
2. Click **Make a Test** → upload any JEE/NEET PDF
3. **Draw boxes** around each question to crop it
4. Set **Subject** (Physics/Chemistry/Maths) and **Question Type** (Single/Multiple correct)
5. Enter test name + duration → click **Launch CBT Test**
6. Fullscreen NTA interface opens — attempt all questions
7. Submit → enter **Answer Key** for each question (post-exam)
8. See full **Results** with question palette, review table, subject breakdown

### What Changed (v2)
- ✅ Removed "select correct answer before exam" from maker
- ✅ Added MCQ (Single Correct) / MSQ (Multiple Correct) question type toggle
- ✅ MSQ options use checkboxes; MCQ uses radio buttons
- ✅ Answer Key entered AFTER the exam on a dedicated page
- ✅ Results show full question palette (green/red/orange/purple)
- ✅ Click any question in palette to preview image + your answer vs correct
- ✅ Full question-by-question review table with scores
- ✅ MSQ partial marking (+1 per correct option if subset of answer)

---

## 🐍 Python Scoring Engine

```bash
python3 score_engine.py result.json [scheme]
# scheme: jee_main (default) | jee_advanced | neet
```

**To get result.json:** After submitting test, open browser DevTools
→ Application → Session Storage → copy `ntaprep_result` value → save as JSON

**Output:** Terminal report + CSV + summary JSON

Marking Schemes:
- `jee_main`: MCQ +4/−1, MSQ +4/−2/+1partial
- `jee_advanced`: MCQ +3/−1, MSQ +4/−2/+1partial
- `neet`: MCQ +4/−1 only

---

## ⚡ C++ Answer Validator

```bash
g++ -std=c++17 -O2 -o validator validator.cpp
./validator answers.csv [scheme]
```

**Input CSV format** (no header):
```
1, Physics, mcq, A, B
2, Chemistry, msq, "A C", "A B C"
3, Mathematics, mcq, , D
```
Columns: `question_num, subject, qtype, user_answer, correct_answer`
(empty user_answer = skipped)

**Output:** Coloured terminal report + `validator_result.csv`

---

## ☕ Java Analytics

```bash
javac Analytics.java
java Analytics result_export.csv
```

**Input:** CSV from Python `score_engine.py` (the `_report.csv` file)

**Output:**
- Terminal: Subject breakdown, MCQ vs MSQ stats, weakness analysis
- File: `analytics_report.md` — full Markdown report with tables

---

## 🔄 Full Workflow

```
PDF → maker.html → test.html → answerkey.html → results.html
                                    ↓
                              ntaprep_result (JSON in sessionStorage)
                                    ↓
                         python3 score_engine.py result.json
                                    ↓
                         result_report.csv → java Analytics
```

---

## Tech Stack
- **Frontend:** Vanilla HTML/CSS/JS, pdf.js for PDF rendering
- **Scoring:** Python 3 (terminal report, CSV, percentile estimate)
- **Validation:** C++17 (fast batch answer checking)
- **Analytics:** Java 17 (detailed breakdown + Markdown export)
- **Storage:** Browser sessionStorage (no server needed)

---

*Free · Open · No login · No ads · Made for JEE & NEET aspirants*
