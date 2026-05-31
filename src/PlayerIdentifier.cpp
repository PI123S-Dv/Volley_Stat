#include "PlayerIdentifier.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

PlayerIdentifier::PlayerIdentifier(const std::string& home,
                                   const std::string& away)
    : homeTeam(home), awayTeam(away) {}

// ── Private helpers ───────────────────────────
int PlayerIdentifier::assignPlayerId(const std::string& actionType,
                                     int rallyNumber,
                                     int actionIndexInRally) const {
    // Volleyball rotation logic:
    // - Server rotates through positions 1-6
    // - Odd rallies: home team serves (player IDs 1-6)
    // - Even rallies: away team serves (player IDs 7-12)

    bool homeServes = (rallyNumber % 2 != 0);

    // Server ID cycles through the team based on rally number
    int serverOffset = (rallyNumber / 2) % 6;

    std::string action = actionType;
    std::transform(action.begin(), action.end(),
                   action.begin(), ::tolower);

    if (action == "serve") {
        // Server is always the rotating player
        return homeServes ? (1 + serverOffset) : (7 + serverOffset);
    }

    if (action == "attack" || action == "spike") {
        // Attacker is usually the opposite team's front row
        // Player IDs 3,4,5 = home front row / 9,10,11 = away front row
        int frontRowOffset = actionIndexInRally % 3;
        return homeServes ? (9 + frontRowOffset) : (3 + frontRowOffset);
    }

    if (action == "block") {
        // Blocker is the receiving team's front row
        int frontRowOffset = actionIndexInRally % 3;
        return homeServes ? (3 + frontRowOffset) : (9 + frontRowOffset);
    }

    if (action == "dig" || action == "defense") {
        // Libero or back row player
        // Player 6 = home libero, player 12 = away libero
        return homeServes ? 6 : 12;
    }

    if (action == "set") {
        // Setter is usually player 2 (home) or 8 (away)
        return homeServes ? 2 : 8;
    }

    // Default — use action index to vary the player
    return homeServes ? (1 + actionIndexInRally % 6)
                      : (7 + actionIndexInRally % 6);
}

std::string PlayerIdentifier::getTeamForPlayer(int playerId) const {
    return (playerId <= 6) ? "home" : "away";
}

// ── Public interface ──────────────────────────
std::vector<PlayerAction> PlayerIdentifier::identifyPlayers(
    const std::vector<std::tuple<std::string,
                                 double,
                                 int,
                                 double>>& rawEvents) const
{
    std::vector<PlayerAction> identified;

    // Track action index per rally for rotation logic
    std::map<int, int> rallyActionIndex;

    for (const auto& [type, timestamp, rallyNum, confidence] : rawEvents) {
        int idx = rallyActionIndex[rallyNum]++;
        int pid = assignPlayerId(type, rallyNum, idx);

        std::string team     = getTeamForPlayer(pid);
        std::string teamName = (team == "home") ? homeTeam : awayTeam;
        std::string pname    = teamName + " P#" + std::to_string(pid);

        identified.push_back({
            pid,
            pname,
            team,
            type,
            timestamp,
            rallyNum,
            confidence
        });
    }

    return identified;
}

void PlayerIdentifier::printPlayerSummary(
    const std::vector<PlayerAction>& actions) const
{
    // Count actions per player
    std::map<int, std::map<std::string, int>> stats;
    std::map<int, std::string> playerNames;

    for (const auto& a : actions) {
        stats[a.playerId][a.actionType]++;
        playerNames[a.playerId] = a.playerName;
    }

    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  PLAYER ACTION SUMMARY\n";
    std::cout << "══════════════════════════════════════\n";

    // Home team
    std::cout << "  " << homeTeam << "\n";
    for (const auto& [pid, actionMap] : stats) {
        if (pid > 6) continue;
        std::cout << "    " << playerNames[pid] << ":\n";
        for (const auto& [action, count] : actionMap)
            std::cout << "      " << std::setw(10) << action
                      << ": " << count << "\n";
    }

    std::cout << "  " << awayTeam << "\n";
    for (const auto& [pid, actionMap] : stats) {
        if (pid <= 6) continue;
        std::cout << "    " << playerNames[pid] << ":\n";
        for (const auto& [action, count] : actionMap)
            std::cout << "      " << std::setw(10) << action
                      << ": " << count << "\n";
    }

    std::cout << "══════════════════════════════════════\n";
}