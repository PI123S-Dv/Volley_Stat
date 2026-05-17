#include "Statistics.h"
#include <iostream>
#include <iomanip>

// ── PlayerStatistics ──────────────────────────
PlayerStatistics::PlayerStatistics(int pid, const std::string& pname)
    : playerId(pid), playerName(pname),
      totalServes(0), successfulServes(0),
      totalAttacks(0), successfulAttacks(0),
      totalBlocks(0), successfulBlocks(0),
      totalDefenses(0), successfulDefenses(0) {}

void PlayerStatistics::processEvent(const MatchEvent& e) {
    const std::string type = e.getEventType();
    bool ok = e.isSuccessful();

    if      (type == "Serve")   { totalServes++;   if (ok) successfulServes++;   }
    else if (type == "Attack")  { totalAttacks++;  if (ok) successfulAttacks++;  }
    else if (type == "Block")   { totalBlocks++;   if (ok) successfulBlocks++;   }
    else if (type == "Defense") { totalDefenses++; if (ok) successfulDefenses++; }
}

int         PlayerStatistics::getPlayerId()   const { return playerId; }
std::string PlayerStatistics::getPlayerName() const { return playerName; }
int         PlayerStatistics::getTotalPoints() const { return successfulAttacks + successfulBlocks; }

double PlayerStatistics::getServeAccuracy() const {
    return totalServes ? (double)successfulServes / totalServes : 0.0;
}
double PlayerStatistics::getAttackEfficiency() const {
    return totalAttacks ? (double)successfulAttacks / totalAttacks : 0.0;
}
double PlayerStatistics::getBlockSuccess() const {
    return totalBlocks ? (double)successfulBlocks / totalBlocks : 0.0;
}
double PlayerStatistics::getDefenseSuccess() const {
    return totalDefenses ? (double)successfulDefenses / totalDefenses : 0.0;
}

void PlayerStatistics::print() const {
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "  Player #" << playerId << " – " << playerName << "\n"
              << "    Points (atk+blk): " << getTotalPoints() << "\n"
              << "    Serve accuracy  : " << std::setprecision(1) << getServeAccuracy()*100 << "%"
              << "  (" << successfulServes << "/" << totalServes << ")\n"
              << "    Attack eff.     : " << getAttackEfficiency()*100 << "%"
              << "  (" << successfulAttacks << "/" << totalAttacks << ")\n"
              << "    Block success   : " << getBlockSuccess()*100 << "%"
              << "  (" << successfulBlocks << "/" << totalBlocks << ")\n"
              << "    Defense success : " << getDefenseSuccess()*100 << "%"
              << "  (" << successfulDefenses << "/" << totalDefenses << ")\n";
}

// ── TeamStatistics ────────────────────────────
TeamStatistics::TeamStatistics(const std::string& tn) : teamName(tn) {}

void TeamStatistics::addPlayerStats(const PlayerStatistics& ps) {
    playerStats.push_back(ps);
}

int TeamStatistics::getTotalTeamPoints() const {
    int total = 0;
    for (const auto& ps : playerStats) total += ps.getTotalPoints();
    return total;
}

double TeamStatistics::getTeamEfficiency() const {
    if (playerStats.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& ps : playerStats) sum += ps.getAttackEfficiency();
    return sum / playerStats.size();
}

std::string TeamStatistics::getTeamName() const { return teamName; }

const std::vector<PlayerStatistics>& TeamStatistics::getAllPlayerStats() const {
    return playerStats;
}

void TeamStatistics::print() const {
    std::cout << "Team: " << teamName << "\n"
              << "  Total Points   : " << getTotalTeamPoints() << "\n"
              << std::fixed << std::setprecision(1)
              << "  Team Efficiency: " << getTeamEfficiency()*100 << "%\n"
              << "  Players:\n";
    for (const auto& ps : playerStats) ps.print();
}
