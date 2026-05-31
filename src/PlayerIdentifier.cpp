#include "PlayerIdentifier.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

PlayerIdentifier::PlayerIdentifier(const std::string& home,
                                   const std::string& away)
    : homeTeam(home), awayTeam(away) {}

int PlayerIdentifier::assignPlayerId(const std::string& actionType,
                                     int rallyNumber,
                                     int actionIndexInRally) const {
    bool homeServes = (rallyNumber % 2 != 0);
    int  serverOffset = (rallyNumber / 2) % 6;

    std::string action = actionType;
    std::transform(action.begin(), action.end(), action.begin(), ::tolower);

    if (action == "serve")
        return homeServes ? (1 + serverOffset) : (7 + serverOffset);
    if (action == "attack" || action == "spike") {
        int offset = actionIndexInRally % 3;
        return homeServes ? (9 + offset) : (3 + offset);
    }
    if (action == "block") {
        int offset = actionIndexInRally % 3;
        return homeServes ? (3 + offset) : (9 + offset);
    }
    if (action == "dig" || action == "defense" || action == "receive")
        return homeServes ? 6 : 12;
    if (action == "set")
        return homeServes ? 2 : 8;

    return homeServes ? (1 + actionIndexInRally % 6)
                      : (7 + actionIndexInRally % 6);
}

std::string PlayerIdentifier::getTeamForPlayer(int playerId) const {
    return (playerId <= 6) ? "home" : "away";
}

std::vector<PlayerAction> PlayerIdentifier::identifyPlayers(
    const std::vector<std::tuple<std::string,
                                 double,
                                 int,
                                 double>>& rawEvents) const
{
    std::vector<PlayerAction> identified;
    std::map<int, int> rallyActionIndex;

    for (const auto& [type, timestamp, rallyNum, confidence] : rawEvents) {
        int idx  = rallyActionIndex[rallyNum]++;
        int pid  = assignPlayerId(type, rallyNum, idx);

        std::string team     = getTeamForPlayer(pid);
        std::string teamName = (team == "home") ? homeTeam : awayTeam;
        std::string pname    = teamName + " P#" + std::to_string(pid);

        identified.push_back({pid, pname, team, type,
                               timestamp, rallyNum, confidence});
    }
    return identified;
}

void PlayerIdentifier::printPlayerSummary(
    const std::vector<PlayerAction>& actions) const
{
    std::map<int, std::map<std::string, int>> stats;
    std::map<int, std::string> playerNames;

    for (const auto& a : actions) {
        stats[a.playerId][a.actionType]++;
        playerNames[a.playerId] = a.playerName;
    }

    std::cout << std::string(50, '=') << "\n";
    std::cout << "  PLAYER ACTION SUMMARY\n";
    std::cout << std::string(50, '=') << "\n";

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

    std::cout << std::string(50, '=') << "\n";
}