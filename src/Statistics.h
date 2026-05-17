#pragma once
#include <string>
#include <vector>
#include <memory>
#include "MatchEvent.h"

// ─────────────────────────────────────────────
//  PlayerStatistics
//  Aggregates event data for one player.
// ─────────────────────────────────────────────
class PlayerStatistics {
private:
    int    playerId;
    std::string playerName;

    int    totalServes;
    int    successfulServes;
    int    totalAttacks;
    int    successfulAttacks;
    int    totalBlocks;
    int    successfulBlocks;
    int    totalDefenses;
    int    successfulDefenses;

public:
    PlayerStatistics(int playerId, const std::string& playerName);

    void processEvent(const MatchEvent& event);

    int         getPlayerId()         const;
    std::string getPlayerName()       const;
    int         getTotalPoints()      const;  // successful attacks + blocks
    double      getServeAccuracy()    const;  // 0.0–1.0
    double      getAttackEfficiency() const;
    double      getBlockSuccess()     const;
    double      getDefenseSuccess()   const;

    void print() const;
};

// ─────────────────────────────────────────────
//  TeamStatistics
//  Aggregates PlayerStatistics for the team.
// ─────────────────────────────────────────────
class TeamStatistics {
private:
    std::string teamName;
    std::vector<PlayerStatistics> playerStats;

public:
    explicit TeamStatistics(const std::string& teamName);

    void addPlayerStats(const PlayerStatistics& ps);
    int  getTotalTeamPoints()  const;
    double getTeamEfficiency() const;    // avg attack efficiency

    std::string getTeamName() const;
    const std::vector<PlayerStatistics>& getAllPlayerStats() const;

    void print() const;
};
