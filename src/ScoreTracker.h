#pragma once
#include <string>
#include <vector>

// ─────────────────────────────────────────────
//  RallyResult
//  Stores the outcome of one rally.
// ─────────────────────────────────────────────
struct RallyResult {
    int         rallyNumber;
    std::string winner;      // "home" or "away"
    std::string lastAction;  // what ended the rally
    double      duration;    // seconds
    int         homeScore;   // running score after this rally
    int         awayScore;
};

// ─────────────────────────────────────────────
//  ScoreTracker
//  Reads the rally list and determines:
//   - Who won each rally
//   - Running score after each rally
//   - Final match winner
//   - Set scores (volleyball plays to 25 per set)
// ─────────────────────────────────────────────
class ScoreTracker {
private:
    std::string homeTeam;
    std::string awayTeam;

    int homeScore;
    int awayScore;
    int homeSets;
    int awaySets;

    std::vector<RallyResult> results;

    // Determines winner of a rally from its last action
    // Attack/Spike by home = home wins the point
    // Attack/Spike by away = away wins the point
    // If unclear = home gets the point (safe default)
    std::string determineWinner(const std::string& lastAction,
                                int rallyNumber) const;

public:
    ScoreTracker(const std::string& homeTeam,
                 const std::string& awayTeam);

    // Process one rally — determines winner and updates score
    void processRally(int rallyNumber,
                      const std::string& lastAction,
                      double duration);

    // Getters
    int         getHomeScore() const;
    int         getAwayScore() const;
    int         getHomeSets()  const;
    int         getAwaySets()  const;
    std::string getWinner()    const;   // overall match winner

    const std::vector<RallyResult>& getResults() const;

    void printScore()   const;
    void printSummary() const;
};