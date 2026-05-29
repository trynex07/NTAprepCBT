/*
 * NTAprep Answer Validator (C++)
 * ================================
 * Ultra-fast answer key validation for large question sets.
 * Reads a CSV of user answers + correct answers and outputs
 * a full scored report with subject-wise analysis.
 *
 * Compile:  g++ -std=c++17 -O2 -o validator validator.cpp
 * Run:      ./validator answers.csv [scheme]
 *           scheme: jee_main (default) | jee_advanced | neet
 *
 * Input CSV format (no header):
 *   question_num, subject, qtype, user_answer, correct_answer
 *   e.g.:  1, Physics, mcq, A, B
 *          2, Chemistry, msq, "A,C", "A,B,C"
 *          3, Mathematics, mcq, , D        (empty = skipped)
 *
 * Output: coloured terminal report + result.txt
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>

// ── ANSI colours ─────────────────────────────────────────────────────────
#define RST  "\033[0m"
#define BOLD "\033[1m"
#define GRN  "\033[92m"
#define RED  "\033[91m"
#define YLW  "\033[93m"
#define CYN  "\033[96m"
#define BLU  "\033[94m"
#define MAG  "\033[95m"

// ── Marking Schemes ───────────────────────────────────────────────────────
struct Scheme {
    int mcq_correct, mcq_wrong;
    int msq_correct, msq_wrong, msq_partial;
};

std::map<std::string, Scheme> SCHEMES = {
    {"jee_main",     {4, -1,  4, -2, 1}},
    {"jee_advanced", {3, -1,  4, -2, 1}},
    {"neet",         {4, -1,  4, -1, 0}},
};

// ── Data structures ───────────────────────────────────────────────────────
enum class Result { CORRECT, WRONG, PARTIAL, SKIPPED };

struct Question {
    int num;
    std::string subject, qtype;
    std::set<char> userAns, correctAns;
    Result result;
    int score;
};

// ── Helpers ───────────────────────────────────────────────────────────────
std::string trim(const std::string& s) {
    size_t st = s.find_first_not_of(" \t\r\n\"");
    size_t en = s.find_last_not_of(" \t\r\n\"");
    return (st == std::string::npos) ? "" : s.substr(st, en - st + 1);
}

std::set<char> parseAns(const std::string& s) {
    std::set<char> res;
    for (char c : s) {
        char u = toupper(c);
        if (u >= 'A' && u <= 'D') res.insert(u);
    }
    return res;
}

std::string fmtAns(const std::set<char>& ans) {
    if (ans.empty()) return "—";
    std::string r;
    for (char c : ans) { if (!r.empty()) r += ","; r += c; }
    return r;
}

std::string resultStr(Result r) {
    switch(r) {
        case Result::CORRECT: return std::string(GRN) + "CORRECT " + RST;
        case Result::WRONG:   return std::string(RED) + "WRONG   " + RST;
        case Result::PARTIAL: return std::string(MAG) + "PARTIAL " + RST;
        default:              return std::string(YLW) + "SKIPPED " + RST;
    }
}

std::string scoreStr(int sc) {
    std::string s = (sc > 0 ? "+" : "") + std::to_string(sc);
    if (sc > 0) return std::string(GRN) + std::setw(4) + s + RST;
    if (sc < 0) return std::string(RED) + std::setw(4) + s + RST;
    return std::string(YLW) + std::setw(4) + s + RST;
}

// ── Score a question ──────────────────────────────────────────────────────
std::pair<Result, int> scoreQuestion(const Question& q, const Scheme& sc) {
    if (q.userAns.empty())    return {Result::SKIPPED, 0};
    if (q.correctAns.empty()) return {Result::SKIPPED, 0};

    if (q.qtype == "msq") {
        if (q.userAns == q.correctAns)
            return {Result::CORRECT, sc.msq_correct};
        // Check if user is subset of correct
        bool isSubset = true;
        for (char c : q.userAns)
            if (q.correctAns.find(c) == q.correctAns.end()) { isSubset = false; break; }
        if (isSubset && q.userAns.size() < q.correctAns.size())
            return {Result::PARTIAL, (int)q.userAns.size() * sc.msq_partial};
        return {Result::WRONG, sc.msq_wrong};
    } else {
        // MCQ: only one correct
        return (q.userAns == q.correctAns)
            ? std::make_pair(Result::CORRECT, sc.mcq_correct)
            : std::make_pair(Result::WRONG,   sc.mcq_wrong);
    }
}

// ── Parse CSV ─────────────────────────────────────────────────────────────
std::vector<Question> parseCSV(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { std::cerr << "Cannot open: " << path << "\n"; exit(1); }
    std::vector<Question> qs;
    std::string line;
    int lineNum = 0;
    while (std::getline(f, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') continue;  // skip comments/empty
        std::stringstream ss(line);
        std::string tok;
        std::vector<std::string> fields;
        while (std::getline(ss, tok, ',')) fields.push_back(trim(tok));
        // Handle quoted fields (e.g. "A,C")
        if (fields.size() < 5) {
            // Try re-parsing with quote handling
            fields.clear();
            bool inQ = false; std::string cur;
            for (char c : line + ",") {
                if (c == '"') { inQ = !inQ; }
                else if (c == ',' && !inQ) { fields.push_back(trim(cur)); cur = ""; }
                else cur += c;
            }
        }
        if (fields.size() < 5) { std::cerr << "Skip line " << lineNum << ": bad format\n"; continue; }
        Question q;
        try { q.num = std::stoi(fields[0]); } catch(...) { continue; }
        q.subject    = fields[1];
        q.qtype      = fields[2]; std::transform(q.qtype.begin(),q.qtype.end(),q.qtype.begin(),::tolower);
        q.userAns    = parseAns(fields[3]);
        q.correctAns = parseAns(fields[4]);
        qs.push_back(q);
    }
    return qs;
}

// ── Generate report ───────────────────────────────────────────────────────
void printReport(const std::vector<Question>& qs, const Scheme& sc, const std::string& schemeName, std::ofstream& out) {
    std::map<std::string, int> sScore, sCorr, sWrong, sSkip, sTotal;
    int total = 0, correct = 0, wrong = 0, skipped = 0;

    for (auto& q : qs) {
        sTotal[q.subject]++;
        auto [res, score] = scoreQuestion(q, sc);
        // We need mutable q for storing result
        total += score;
        if (res == Result::CORRECT || res == Result::PARTIAL) { sCorr[q.subject]++; correct++; }
        else if (res == Result::WRONG)  { sWrong[q.subject]++; wrong++; }
        else                            { sSkip[q.subject]++;  skipped++; }
        sScore[q.subject] += score;
    }

    double acc = (correct + wrong > 0) ? 100.0 * correct / (correct + wrong) : 0;

    auto sep = std::string(60, '=');
    std::cout << "\n" << BOLD << CYN << sep << RST << "\n";
    std::cout << BOLD << "  NTAprep C++ Validator  —  Scheme: " << schemeName << RST << "\n";
    std::cout << BOLD << CYN << sep << RST << "\n\n";

    std::cout << BOLD << "  Score    : " << RST;
    if (total > 0) std::cout << GRN; else std::cout << RED;
    std::cout << total << RST << " / " << (int)qs.size() * 4 << "\n";
    std::cout << "  Accuracy : " << std::fixed << std::setprecision(1) << acc << "%\n";
    std::cout << "  Correct  : " << GRN << correct << RST
              << "   Wrong: " << RED << wrong << RST
              << "   Skipped: " << YLW << skipped << RST << "\n";

    // Subject breakdown
    std::cout << "\n" << BOLD << "  Subject Breakdown:\n" << RST;
    std::cout << "  " << std::left << std::setw(14) << "Subject"
              << std::right << std::setw(8) << "Score"
              << std::setw(7) << "Corr"
              << std::setw(7) << "Wrong"
              << std::setw(7) << "Skip"
              << std::setw(8) << "Acc" << "\n";
    std::cout << "  " << std::string(51, '-') << "\n";
    for (auto& [subj, tot] : sTotal) {
        int att = sCorr[subj] + sWrong[subj];
        double accS = att > 0 ? 100.0 * sCorr[subj] / att : 0;
        int maxS = tot * 4;
        std::string clr = (subj == "Physics") ? BLU : (subj == "Chemistry") ? GRN : MAG;
        std::cout << "  " << clr << std::left << std::setw(14) << subj << RST
                  << std::right << std::setw(4) << sScore[subj] << "/" << std::setw(3) << maxS
                  << std::setw(7) << sCorr[subj]
                  << std::setw(7) << sWrong[subj]
                  << std::setw(7) << sSkip[subj]
                  << std::setw(6) << std::fixed << std::setprecision(0) << accS << "%\n";
    }

    // Question detail
    std::cout << "\n" << BOLD << "  Question-by-Question:\n" << RST;
    std::cout << "  " << std::setw(4) << "Q#"
              << std::left << std::setw(12) << "  Subject"
              << std::setw(6) << "Type"
              << std::setw(8) << "User"
              << std::setw(8) << "Key"
              << "Result    Score\n";
    std::cout << "  " << std::string(58, '-') << "\n";
    for (auto& q : qs) {
        auto [res, score] = scoreQuestion(q, sc);
        std::string clr = (q.subject == "Physics") ? BLU : (q.subject == "Chemistry") ? GRN : MAG;
        std::cout << "  " << std::right << std::setw(3) << q.num
                  << "  " << clr << std::left << std::setw(12) << q.subject.substr(0,11) << RST
                  << std::setw(6) << q.qtype.substr(0,3)
                  << std::setw(8) << fmtAns(q.userAns)
                  << std::setw(8) << fmtAns(q.correctAns)
                  << resultStr(res) << "  " << scoreStr(score) << "\n";
        // Write to file too
        std::string r_str = (res==Result::CORRECT?"CORRECT":(res==Result::WRONG?"WRONG":(res==Result::PARTIAL?"PARTIAL":"SKIPPED")));
        out << q.num << "," << q.subject << "," << q.qtype << ","
            << fmtAns(q.userAns) << "," << fmtAns(q.correctAns) << ","
            << r_str << "," << score << "\n";
    }

    std::cout << "\n" << BOLD << CYN << sep << RST << "\n";
    std::cout << "  Total: " << BOLD;
    if (total > 0) std::cout << GRN; else std::cout << RED;
    std::cout << total << RST << BOLD << " / " << (int)qs.size() * 4 << RST;
    std::cout << "  |  Accuracy: " << std::fixed << std::setprecision(1) << acc << "%\n";
    std::cout << BOLD << CYN << sep << RST << "\n\n";
}

// ── Main ─────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./validator <answers.csv> [scheme]\n";
        std::cout << "       scheme: jee_main (default) | jee_advanced | neet\n\n";
        std::cout << "CSV Format (no header):\n";
        std::cout << "  q_num, subject, qtype, user_answer, correct_answer\n";
        std::cout << "  e.g.: 1, Physics, mcq, A, B\n";
        std::cout << "        2, Chemistry, msq, \"A C\", \"A B C\"\n";
        return 1;
    }

    std::string csvPath  = argv[1];
    std::string schemeName = (argc > 2) ? argv[2] : "jee_main";

    if (SCHEMES.find(schemeName) == SCHEMES.end()) {
        std::cerr << "Unknown scheme: " << schemeName << ". Using jee_main.\n";
        schemeName = "jee_main";
    }

    auto qs = parseCSV(csvPath);
    if (qs.empty()) { std::cerr << "No questions parsed.\n"; return 1; }
    std::cout << "  Loaded " << qs.size() << " questions from " << csvPath << "\n";

    std::string outPath = "validator_result.csv";
    std::ofstream out(outPath);
    out << "num,subject,qtype,user_answer,correct_answer,result,score\n";

    printReport(qs, SCHEMES[schemeName], schemeName, out);

    std::cout << "  📄 Results written to " << outPath << "\n\n";
    return 0;
}
