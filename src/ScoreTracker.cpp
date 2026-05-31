#include "ScoreTracker.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// ── Constructor ───────────────────────────────
ScoreTracker::ScoreTracker(const std::string& home, const std::string& away)
    : homeTeam(home), awayTeam(away),
      homeScore(0), awayScore(0),
      homeSets(0), awaySets(0) {}

// ── Private helpers ───────────────────────────
std::string ScoreTracker::determineWinner(const std::string& lastAction,
                                          int rallyNumber) const {
    // The team that performs the last attack/spike wins the point.
    // Odd rallies = home team served first = away team more likely attacked last
    // Even rallies = away team served first = home team more likely attacked last
    // This is a heuristic — real tracking would use player positions.

    std::string action = lastAction;
    std::transform(action.begin(), action.end(), action.begin(), ::tolower);

    if (action == "attack" || action == "spike") {
        // Attacker wins the point
        // Alternate based on rally number as a simple heuristic
        return (rallyNumber % 2 != 0) ? "away" : "home";
    }
    if (action == "dig" || action == "defense") {
        // Failed dig = attacker wins = opposite team
        return (rallyNumber % 2 != 0) ? "home" : "away";
    }
    if (action == "block") {
        // Successful block wins the point
        return (rallyNumber % 2 != 0) ? "home" : "away";
    }
    if (action == "serve") {
        // Ace serve = server wins
        return (rallyNumber % 2 != 0) ? "home" : "away";
    }

    // Default — alternate points to keep score realistic
    return (rallyNumber % 2 != 0) ? "home" : "away";
}

// ── Public interface ──────────────────────────
void ScoreTracker::processRally(int rallyNumber,
                                const std::string& lastAction,
                                double duration) {
    std::string winner = determineWinner(lastAction, rallyNumber);

    if (winner == "home") ++homeScore;
    else                  ++awayScore;

    // Check if a set is won (first to 25, must win by 2)
    bool homeWinsSet = (homeScore >= 25 && homeScore - awayScore >= 2);
    bool awayWinsSet = (awayScore >= 25 && awayScore - homeScore >= 2);

    if (homeWinsSet) {
        ++homeSets;
        std::cout << "  [SET] SET WON by " << homeTeam
                  << " (" << homeSets << "-" << awaySets << " sets)\n";
        homeScore = 0;
        awayScore = 0;
    } else if (awayWinsSet) {
        ++awaySets;
        std::cout << "  [SET] SET WON by " << awayTeam
                  << " (" << homeSets << "-" << awaySets << " sets)\n";
        homeScore = 0;
        awayScore = 0;
    }

    results.push_back({
        rallyNumber,
        winner,
        lastAction,
        duration,
        homeScore,
        awayScore
    });
}

int         ScoreTracker::getHomeScore() const { return homeScore; }
int         ScoreTracker::getAwayScore() const { return awayScore; }
int         ScoreTracker::getHomeSets()  const { return homeSets; }
int         ScoreTracker::getAwaySets()  const { return awaySets; }

std::string ScoreTracker::getWinner() const {
    if (homeSets > awaySets) return homeTeam;
    if (awaySets > homeSets) return awayTeam;
    return "Tied";
}

const std::vector<RallyResult>& ScoreTracker::getResults() const {
    return results;
}

void ScoreTracker::printScore() const {
    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  CURRENT SCORE\n";
    std::cout << "══════════════════════════════════════\n";
    std::cout << "  " << homeTeam << "  "
              << homeScore << " – " << awayScore
              << "  " << awayTeam << "\n";
    std::cout << "  Sets: " << homeSets << " – " << awaySets << "\n";
    std::cout << "══════════════════════════════════════\n";
}

void ScoreTracker::printSummary() const {
    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  MATCH SUMMARY\n";
    std::cout << "══════════════════════════════════════\n";
    std::cout << "  " << homeTeam << " vs " << awayTeam << "\n";
    std::cout << "  Final sets : "
              << homeSets << " – " << awaySets << "\n";
    std::cout << "  Final score: "
              << homeScore << " – " << awayScore << "\n";
    std::cout << "  Winner     : " << getWinner() << "\n";
    std::cout << "  Rallies    : " << results.size() << "\n";
    std::cout << "──────────────────────────────────────\n";
    std::cout << "  Rally breakdown:\n";
    for (const auto& r : results) {
        std::cout << "    Rally #" << std::setw(2) << r.rallyNumber
                  << " | " << std::setw(5) << std::fixed
                  << std::setprecision(1) << r.duration << "s"
                  << " | Last: " << std::setw(8) << r.lastAction
                  << " | Won by: " << r.winner
                  << " | Score: " << r.homeScore
                  << "-"          << r.awayScore << "\n";
    }
    std::cout << "══════════════════════════════════════\n";
}