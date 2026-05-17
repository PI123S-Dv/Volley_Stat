#include "Dashboard.h"
#include <iostream>
#include <fstream>

void Dashboard::showScoreboard(const MatchAnalysis& analysis) const {
    analysis.printScoreboard();
}

void Dashboard::showTimeline(const MatchAnalysis& analysis) const {
    analysis.printTimeline();
}

void Dashboard::showPlayerStats(const TeamStatistics& stats) const {
    std::cout << "\n── Player Statistics: " << stats.getTeamName() << " ──\n";
    stats.print();
}

void Dashboard::showComparison(const MatchAnalysis& a, const MatchAnalysis& b) const {
    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  MATCH COMPARISON\n";
    std::cout << "══════════════════════════════════════\n";
    std::cout << "Match #" << a.getMatchId() << " (" << a.getMatchDate() << ")\n";
    std::cout << "  " << a.getHomeTeam() << " pts: " << a.getHomeStats().getTotalTeamPoints()
              << "  vs  "
              << a.getAwayTeam() << " pts: " << a.getAwayStats().getTotalTeamPoints() << "\n";
    std::cout << "Match #" << b.getMatchId() << " (" << b.getMatchDate() << ")\n";
    std::cout << "  " << b.getHomeTeam() << " pts: " << b.getHomeStats().getTotalTeamPoints()
              << "  vs  "
              << b.getAwayTeam() << " pts: " << b.getAwayStats().getTotalTeamPoints() << "\n";
    std::cout << "══════════════════════════════════════\n";
}

// ── ReportGenerator ───────────────────────────
std::string ReportGenerator::generateReport(const MatchAnalysis& analysis,
                                            const std::string& analystName) const {
    std::string r;
    r += "══════════════════════════════════════\n";
    r += "  VOLLEYBALL MATCH REPORT\n";
    r += "  Analyst: " + analystName + "\n";
    r += "══════════════════════════════════════\n";
    r += analysis.getSummary() + "\n\n";
    r += "── Recommendations ──\n";
    r += analysis.getRecommendations() + "\n";
    r += "══════════════════════════════════════\n";
    return r;
}

void ReportGenerator::exportToFile(const std::string& report,
                                   const std::string& filepath) const {
    std::ofstream out(filepath);
    if (out.is_open()) {
        out << report;
        std::cout << "📄  Report saved to: " << filepath << "\n";
    } else {
        std::cout << "❌  Could not write report to: " << filepath << "\n";
    }
}
