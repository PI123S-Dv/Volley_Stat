#include "ScoreTracker.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

ScoreTracker::ScoreTracker(const std::string& home, const std::string& away)
    : homeTeam(home), awayTeam(away),
      homeScore(0), awayScore(0),
      homeSets(0), awaySets(0) {}

std::string ScoreTracker::determineWinner(const std::string& lastAction,
                                          int rallyNumber) const {
    std::string action = lastAction;
    std::transform(action.begin(), action.end(), action.begin(), ::tolower);

    if (action == "attack" || action == "spike")
        return (rallyNumber % 2 != 0) ? "away" : "home";
    if (action == "dig" || action == "defense" || action == "receive")
        return (rallyNumber % 2 != 0) ? "home" : "away";
    if (action == "block")
        return (rallyNumber % 2 != 0) ? "home" : "away";
    if (action == "serve")
        return (rallyNumber % 2 != 0) ? "home" : "away";

    return (rallyNumber % 2 != 0) ? "home" : "away";
}

void ScoreTracker::processRally(int rallyNumber,
                                const std::string& lastAction,
                                double duration) {
    std::string winner = determineWinner(lastAction, rallyNumber);

    if (winner == "home") ++homeScore;
    else                  ++awayScore;

    bool homeWinsSet = (homeScore >= 25 && homeScore - awayScore >= 2);
    bool awayWinsSet = (awayScore >= 25 && awayScore - homeScore >= 2);

    if (homeWinsSet) {
        ++homeSets;
        std::cout << "  [SET WON] " << homeTeam
                  << " (" << homeSets << "-" << awaySets << " sets)\n";
        homeScore = 0; awayScore = 0;
    } else if (awayWinsSet) {
        ++awaySets;
        std::cout << "  [SET WON] " << awayTeam
                  << " (" << homeSets << "-" << awaySets << " sets)\n";
        homeScore = 0; awayScore = 0;
    }

    results.push_back({rallyNumber, winner, lastAction,
                       duration, homeScore, awayScore});
}

int         ScoreTracker::getHomeScore() const { return homeScore; }
int         ScoreTracker::getAwayScore() const { return awayScore; }
int         ScoreTracker::getHomeSets()  const { return homeSets; }
int         ScoreTracker::getAwaySets()  const { return awaySets; }

std::string ScoreTracker::getWinner() const {
    if (homeSets > awaySets) return homeTeam;
    if (awaySets > homeSets) return awayTeam;
    if (homeScore > awayScore) return homeTeam;
    if (awayScore > homeScore) return awayTeam;
    return "Tied";
}

const std::vector<RallyResult>& ScoreTracker::getResults() const {
    return results;
}

void ScoreTracker::printScore() const {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "  CURRENT SCORE\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "  " << homeTeam << "  "
              << homeScore << " - " << awayScore
              << "  " << awayTeam << "\n";
    std::cout << "  Sets: " << homeSets << " - " << awaySets << "\n";
    std::cout << std::string(50, '=') << "\n";
}

void ScoreTracker::printSummary() const {
    std::cout << std::string(50, '=') << "\n";
    std::cout << "  MATCH SUMMARY\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "  " << homeTeam << " vs " << awayTeam << "\n";
    std::cout << "  Final sets : " << homeSets << " - " << awaySets << "\n";
    std::cout << "  Final score: " << homeScore << " - " << awayScore << "\n";
    std::cout << "  Winner     : " << getWinner() << "\n";
    std::cout << "  Rallies    : " << results.size() << "\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << "  Rally breakdown:\n";
    for (const auto& r : results) {
        std::cout << "    Rally #" << std::setw(2) << r.rallyNumber
                  << " | " << std::setw(5) << std::fixed
                  << std::setprecision(1) << r.duration << "s"
                  << " | Last: " << std::setw(8) << r.lastAction
                  << " | Won by: " << std::setw(5) << r.winner
                  << " | Score: " << r.homeScore
                  << " - "        << r.awayScore << "\n";
    }
    std::cout << std::string(50, '=') << "\n";
}