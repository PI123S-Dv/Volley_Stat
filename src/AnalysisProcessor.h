#pragma once
#include <string>
#include <vector>
#include <memory>
#include "MatchEvent.h"
#include "Statistics.h"
#include "VideoFile.h"

// ─────────────────────────────────────────────
//  MatchAnalysis
//  Owns all events + statistics for one match.
//  (Composition: destroying MatchAnalysis destroys its events/stats)
// ─────────────────────────────────────────────
class MatchAnalysis {
private:
    int         matchId;
    std::string matchDate;
    std::string homeTeam;
    std::string awayTeam;

    // Composition – events belong to this analysis
    std::vector<std::unique_ptr<MatchEvent>> events;

    // Composition – stats belong to this analysis
    TeamStatistics homeStats;
    TeamStatistics awayStats;

public:
    MatchAnalysis(int matchId, const std::string& matchDate,
                  const std::string& homeTeam, const std::string& awayTeam);

    // Disable copy (composition semantics)
    MatchAnalysis(const MatchAnalysis&)            = delete;
    MatchAnalysis& operator=(const MatchAnalysis&) = delete;

    void addEvent(std::unique_ptr<MatchEvent> event);

    // Original method – pass player lists explicitly (used by AnalysisProcessor)
    void buildStatistics(const std::vector<PlayerStatistics>& homePlayerStats,
                         const std::vector<PlayerStatistics>& awayPlayerStats);

    // New method – build stats from events already added (used by JSONReader)
    // Automatically assigns players to home/away based on rally number
    void buildStatisticsFromEvents();

    int         getMatchId()     const;
    std::string getMatchDate()   const;
    std::string getHomeTeam()    const;
    std::string getAwayTeam()    const;
    int         getTotalEvents() const;

    const std::vector<std::unique_ptr<MatchEvent>>& getEvents() const;
    const TeamStatistics& getHomeStats() const;
    const TeamStatistics& getAwayStats() const;

    std::string getSummary()         const;
    std::string getRecommendations() const;

    void printTimeline()   const;
    void printScoreboard() const;
};

// ─────────────────────────────────────────────
//  AnalysisProcessor
//  Simulates AI analysis of a VideoFile.
//  Separated from VideoFile (Single Responsibility).
// ─────────────────────────────────────────────
class AnalysisProcessor {
private:
    int nextEventId;

    // Private helpers
    std::unique_ptr<MatchEvent> generateServe(int playerId, double timestamp);
    std::unique_ptr<MatchEvent> generateAttack(int playerId, double timestamp);
    std::unique_ptr<MatchEvent> generateBlock(int playerId, double timestamp);
    std::unique_ptr<MatchEvent> generateDefense(int playerId, double timestamp);

public:
    AnalysisProcessor();

    // Simulated analysis – used when no real JSON is available
    std::unique_ptr<MatchAnalysis> processVideo(
        VideoFile& video,
        int matchId,
        const std::string& matchDate,
        const std::string& homeTeam,
        const std::string& awayTeam
    );
};