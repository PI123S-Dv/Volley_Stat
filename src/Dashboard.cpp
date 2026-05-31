#include "Dashboard.h"
#include <iostream>
#include <iomanip>
#include <fstream>

// ── Helpers ───────────────────────────────────
static void line(char c = '-', int len = 50) {
    std::cout << std::string(len, c) << "\n";
}
static void header(const std::string& title) {
    line('=');
    std::cout << "  " << title << "\n";
    line('=');
}
static void section(const std::string& title) {
    line('-');
    std::cout << "  " << title << "\n";
    line('-');
}

void Dashboard::showScoreboard(const MatchAnalysis& analysis) const {
    std::cout << "\n";
    header("MATCH #" + std::to_string(analysis.getMatchId())
           + "  |  " + analysis.getMatchDate());
    std::cout << "  " << analysis.getHomeTeam()
              << " vs " << analysis.getAwayTeam() << "\n";
    line('=');

    // Home team
    const auto& home = analysis.getHomeStats();
    std::cout << "\n  TEAM: " << home.getTeamName() << "\n";
    std::cout << "  Total Points   : " << home.getTotalTeamPoints() << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Team Efficiency: "
              << home.getTeamEfficiency() * 100 << "%\n\n";

    for (const auto& ps : home.getAllPlayerStats()) {
        std::cout << "    Player #" << ps.getPlayerId()
                  << " - " << ps.getPlayerName() << "\n";
        std::cout << "      Points        : " << ps.getTotalPoints() << "\n";
        std::cout << "      Serve acc.    : "
                  << ps.getServeAccuracy() * 100 << "%\n";
        std::cout << "      Attack eff.   : "
                  << ps.getAttackEfficiency() * 100 << "%\n";
        std::cout << "      Block success : "
                  << ps.getBlockSuccess() * 100 << "%\n";
        std::cout << "      Defense success: "
                  << ps.getDefenseSuccess() * 100 << "%\n";
    }

    line('-');

    // Away team
    const auto& away = analysis.getAwayStats();
    std::cout << "\n  TEAM: " << away.getTeamName() << "\n";
    std::cout << "  Total Points   : " << away.getTotalTeamPoints() << "\n";
    std::cout << "  Team Efficiency: "
              << away.getTeamEfficiency() * 100 << "%\n\n";

    for (const auto& ps : away.getAllPlayerStats()) {
        std::cout << "    Player #" << ps.getPlayerId()
                  << " - " << ps.getPlayerName() << "\n";
        std::cout << "      Points        : " << ps.getTotalPoints() << "\n";
        std::cout << "      Serve acc.    : "
                  << ps.getServeAccuracy() * 100 << "%\n";
        std::cout << "      Attack eff.   : "
                  << ps.getAttackEfficiency() * 100 << "%\n";
        std::cout << "      Block success : "
                  << ps.getBlockSuccess() * 100 << "%\n";
        std::cout << "      Defense success: "
                  << ps.getDefenseSuccess() * 100 << "%\n";
    }

    line('=');
}

void Dashboard::showTimeline(const MatchAnalysis& analysis) const {
    const auto& events = analysis.getEvents();
    std::cout << "\n";
    section("EVENT TIMELINE  (" + std::to_string(events.size()) + " events)");
    for (const auto& ev : events) {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  [" << std::setw(6) << ev->getTimestamp() << "s]"
                  << "  " << std::setw(8) << std::left << ev->getEventType()
                  << std::right
                  << "  Player #" << ev->getPlayerId()
                  << "  " << (ev->isSuccessful() ? "OK" : "FAIL")
                  << "\n";
    }
    line('-');
}

void Dashboard::showPlayerStats(const TeamStatistics& stats) const {
    std::cout << "\n";
    section("PLAYER STATS: " + stats.getTeamName());
    stats.print();
}

void Dashboard::showComparison(const MatchAnalysis& a,
                               const MatchAnalysis& b) const {
    std::cout << "\n";
    header("MATCH COMPARISON");
    std::cout << "  Match #" << a.getMatchId()
              << " (" << a.getMatchDate() << ")\n";
    std::cout << "    " << a.getHomeTeam() << " pts: "
              << a.getHomeStats().getTotalTeamPoints()
              << "  vs  "
              << a.getAwayTeam() << " pts: "
              << a.getAwayStats().getTotalTeamPoints() << "\n\n";
    std::cout << "  Match #" << b.getMatchId()
              << " (" << b.getMatchDate() << ")\n";
    std::cout << "    " << b.getHomeTeam() << " pts: "
              << b.getHomeStats().getTotalTeamPoints()
              << "  vs  "
              << b.getAwayTeam() << " pts: "
              << b.getAwayStats().getTotalTeamPoints() << "\n";
    line('=');
}

// ── ReportGenerator ───────────────────────────
std::string ReportGenerator::generateReport(const MatchAnalysis& analysis,
                                            const std::string& analystName) const {
    std::string r;
    r += "\n";
    r += std::string(50, '=') + "\n";
    r += "  VOLLEYBALL MATCH REPORT\n";
    r += "  Analyst : " + analystName + "\n";
    r += std::string(50, '=') + "\n";
    r += analysis.getSummary() + "\n\n";
    r += std::string(50, '-') + "\n";
    r += "  Recommendations\n";
    r += std::string(50, '-') + "\n";
    r += analysis.getRecommendations() + "\n";
    r += std::string(50, '=') + "\n";
    return r;
}

void ReportGenerator::exportToFile(const std::string& report,
                                   const std::string& filepath) const {
    std::ofstream out(filepath);
    if (out.is_open()) {
        out << report;
        std::cout << "  Report saved to: " << filepath << "\n";
    } else {
        std::cout << "  ERROR: Could not write to: " << filepath << "\n";
    }
}