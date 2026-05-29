import java.io.*;
import java.util.*;
import java.util.stream.*;

/**
 * NTAprep Analytics Report Generator (Java)
 * ==========================================
 * Reads the exported CSV from score_engine.py or validator.cpp
 * and generates a detailed performance analytics report.
 *
 * Compile:  javac Analytics.java
 * Run:      java Analytics result_export.csv
 */
public class Analytics {

    static final String RESET = "\u001B[0m";
    static final String BOLD  = "\u001B[1m";
    static final String GREEN = "\u001B[92m";
    static final String RED   = "\u001B[91m";
    static final String YLW   = "\u001B[93m";
    static final String BLU   = "\u001B[94m";
    static final String CYN   = "\u001B[96m";
    static final String MAG   = "\u001B[95m";

    record QRecord(int num, String subject, String qtype,
                   String userAns, String correctAns, String result, int score) {}

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.out.println("Usage: java Analytics <result_export.csv>");
            return;
        }
        List<QRecord> records = loadCSV(args[0]);
        if (records.isEmpty()) { System.err.println("No data found."); return; }

        generateReport(records);
        generateWeaknessReport(records);
        exportMarkdown(records, "analytics_report.md");
        System.out.println("\n  📄 Markdown report → analytics_report.md");
    }

    static List<QRecord> loadCSV(String path) throws Exception {
        List<QRecord> list = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            String line;
            boolean first = true;
            while ((line = br.readLine()) != null) {
                if (first) { first = false; continue; } // skip header
                String[] p = line.split(",", 7);
                if (p.length < 7) continue;
                list.add(new QRecord(
                    Integer.parseInt(p[0].trim()),
                    p[1].trim(), p[2].trim(), p[3].trim(),
                    p[4].trim(), p[5].trim(), Integer.parseInt(p[6].trim())
                ));
            }
        }
        return list;
    }

    static void generateReport(List<QRecord> recs) {
        int total   = recs.stream().mapToInt(QRecord::score).sum();
        int maxScore= recs.size() * 4;
        long correct= recs.stream().filter(r -> r.result().equals("CORRECT")).count();
        long wrong  = recs.stream().filter(r -> r.result().equals("WRONG")).count();
        long partial= recs.stream().filter(r -> r.result().equals("PARTIAL")).count();
        long skipped= recs.stream().filter(r -> r.result().equals("SKIPPED")).count();
        double acc  = (correct + wrong) > 0 ? 100.0 * correct / (correct + wrong) : 0;

        String sep = "═".repeat(58);
        System.out.println("\n" + BOLD + CYN + sep + RESET);
        System.out.println("  " + BOLD + "NTAprep Java Analytics Report" + RESET);
        System.out.println(BOLD + CYN + sep + RESET);

        String sc = total > 0 ? GREEN + total : RED + total;
        System.out.printf("  Score    : %s%s / %d%n", sc, RESET, maxScore);
        System.out.printf("  Accuracy : %.1f%%%n", acc);
        System.out.printf("  Correct  : %s%d%s   Wrong: %s%d%s   Partial: %s%d%s   Skipped: %s%d%s%n",
            GREEN, correct, RESET, RED, wrong, RESET,
            MAG, partial, RESET, YLW, skipped, RESET);

        // Subject grouping
        Map<String, List<QRecord>> bySubj = recs.stream()
            .collect(Collectors.groupingBy(QRecord::subject));

        System.out.println("\n  " + BOLD + "Subject-wise Analysis:" + RESET);
        System.out.printf("  %-14s %8s %6s %6s %6s %7s%n",
            "Subject", "Score", "Corr", "Wrong", "Skip", "Acc");
        System.out.println("  " + "-".repeat(50));

        for (Map.Entry<String, List<QRecord>> e : new TreeMap<>(bySubj).entrySet()) {
            String subj = e.getKey();
            List<QRecord> sq = e.getValue();
            int sScore = sq.stream().mapToInt(QRecord::score).sum();
            int sMax   = sq.size() * 4;
            long sCorr = sq.stream().filter(r -> r.result().equals("CORRECT")).count();
            long sWrong= sq.stream().filter(r -> r.result().equals("WRONG")).count();
            long sSkip = sq.stream().filter(r -> r.result().equals("SKIPPED")).count();
            double sAcc= (sCorr + sWrong) > 0 ? 100.0 * sCorr / (sCorr + sWrong) : 0;
            String clr = subj.equals("Physics") ? BLU : subj.equals("Chemistry") ? GREEN : MAG;
            System.out.printf("  %s%-14s%s %4d/%-4d %6d %6d %6d %5.0f%%%n",
                clr, subj, RESET, sScore, sMax, sCorr, sWrong, sSkip, sAcc);
        }

        // MCQ vs MSQ
        Map<String, List<QRecord>> byType = recs.stream()
            .collect(Collectors.groupingBy(QRecord::qtype));
        System.out.println("\n  " + BOLD + "MCQ vs MSQ Breakdown:" + RESET);
        for (Map.Entry<String, List<QRecord>> e : byType.entrySet()) {
            List<QRecord> tq = e.getValue();
            long tc = tq.stream().filter(r -> r.result().equals("CORRECT")).count();
            double ta = tq.stream().filter(r->!r.result().equals("SKIPPED")).count() > 0
                ? 100.0*tc / tq.stream().filter(r->!r.result().equals("SKIPPED")).count() : 0;
            System.out.printf("  %-6s : %d questions, %d correct, %.0f%% accuracy%n",
                e.getKey().toUpperCase(), tq.size(), tc, ta);
        }
        System.out.println("\n" + BOLD + CYN + sep + RESET);
    }

    static void generateWeaknessReport(List<QRecord> recs) {
        System.out.println("\n  " + BOLD + YLW + "⚠ Weakness Analysis:" + RESET);
        Map<String, List<QRecord>> bySubj = recs.stream()
            .collect(Collectors.groupingBy(QRecord::subject));

        for (Map.Entry<String, List<QRecord>> e : new TreeMap<>(bySubj).entrySet()) {
            String subj = e.getKey();
            List<QRecord> sq = e.getValue();
            long wrong = sq.stream().filter(r -> r.result().equals("WRONG")).count();
            long total = sq.stream().filter(r -> !r.result().equals("SKIPPED")).count();
            if (total == 0) continue;
            double errRate = 100.0 * wrong / total;
            String indicator = errRate > 50 ? RED+"⚠ NEEDS WORK" : errRate > 30 ? YLW+"△ IMPROVE" : GREEN+"✓ GOOD";
            System.out.printf("  %-14s  Error rate: %.0f%%  %s%s%n", subj, errRate, indicator, RESET);
        }

        // Questions where score = -1 or -2 (penalty questions)
        List<QRecord> penalty = recs.stream()
            .filter(r -> r.score() < 0).collect(Collectors.toList());
        System.out.printf("%n  Penalty questions (negative score): %s%d%s%n",
            RED, penalty.size(), RESET);
        if (!penalty.isEmpty()) {
            System.out.println("  Q numbers: " +
                penalty.stream().map(r -> "Q" + r.num())
                    .collect(Collectors.joining(", ")));
        }
    }

    static void exportMarkdown(List<QRecord> recs, String path) throws Exception {
        int total   = recs.stream().mapToInt(QRecord::score).sum();
        int maxScore= recs.size() * 4;
        long correct= recs.stream().filter(r -> r.result().equals("CORRECT")).count();
        long wrong  = recs.stream().filter(r -> r.result().equals("WRONG")).count();
        long skipped= recs.stream().filter(r -> r.result().equals("SKIPPED")).count();
        double acc  = (correct + wrong) > 0 ? 100.0 * correct / (correct + wrong) : 0;

        try (PrintWriter pw = new PrintWriter(new FileWriter(path))) {
            pw.println("# NTAprep Analytics Report\n");
            pw.printf("**Score:** %d / %d  |  **Accuracy:** %.1f%%  |  "
                + "**Correct:** %d  |  **Wrong:** %d  |  **Skipped:** %d%n%n",
                total, maxScore, acc, correct, wrong, skipped);

            pw.println("## Subject-wise Performance\n");
            pw.println("| Subject | Score | Correct | Wrong | Skipped | Accuracy |");
            pw.println("|---------|-------|---------|-------|---------|----------|");

            Map<String, List<QRecord>> bySubj = recs.stream()
                .collect(Collectors.groupingBy(QRecord::subject));
            for (Map.Entry<String, List<QRecord>> e : new TreeMap<>(bySubj).entrySet()) {
                List<QRecord> sq = e.getValue();
                int ss = sq.stream().mapToInt(QRecord::score).sum();
                long sc = sq.stream().filter(r -> r.result().equals("CORRECT")).count();
                long sw = sq.stream().filter(r -> r.result().equals("WRONG")).count();
                long sk = sq.stream().filter(r -> r.result().equals("SKIPPED")).count();
                double sa = (sc + sw) > 0 ? 100.0 * sc / (sc + sw) : 0;
                pw.printf("| %s | %d/%d | %d | %d | %d | %.0f%% |%n",
                    e.getKey(), ss, sq.size()*4, sc, sw, sk, sa);
            }

            pw.println("\n## Question-by-Question\n");
            pw.println("| # | Subject | Type | Your Answer | Correct | Result | Score |");
            pw.println("|---|---------|------|-------------|---------|--------|-------|");
            for (QRecord r : recs) {
                pw.printf("| %d | %s | %s | %s | %s | %s | %+d |%n",
                    r.num(), r.subject(), r.qtype().toUpperCase(),
                    r.userAns().isEmpty()?"—":r.userAns(),
                    r.correctAns(), r.result(), r.score());
            }

            pw.println("\n---\n*Generated by NTAprep Analytics (Java)*");
        }
    }
}
