#pragma once
#include <string>
#include <vector>
#include "AnalysisProcessor.h"

// ─────────────────────────────────────────────
//  Dashboard
//  Displays analysis data to the user.
// ─────────────────────────────────────────────
class Dashboard {
public:
    void showScoreboard(const MatchAnalysis& analysis) const;
    void showTimeline(const MatchAnalysis& analysis)   const;
    void showPlayerStats(const TeamStatistics& stats)  const;
    void showComparison(const MatchAnalysis& a,
                        const MatchAnalysis& b)        const;
};

// ─────────────────────────────────────────────
//  ReportGenerator
//  Generates a text-based report (simulates PDF export).
// ─────────────────────────────────────────────
class ReportGenerator {
public:
    // Returns the report as a formatted string (would be saved to PDF in production)
    std::string generateReport(const MatchAnalysis& analysis,
                               const std::string& analystName) const;

    // Saves report to a .txt file (simulating PDF export)
    void exportToFile(const std::string& report,
                      const std::string& filepath) const;
};
