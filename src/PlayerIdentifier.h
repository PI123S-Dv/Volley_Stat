#pragma once
#include <string>
#include <vector>
#include <map>

// ─────────────────────────────────────────────
//  PlayerAction
//  One action attributed to a specific player.
// ─────────────────────────────────────────────
struct PlayerAction {
    int         playerId;
    std::string playerName;
    std::string team;        // "home" or "away"
    std::string actionType;
    double      timestamp;
    int         rallyNumber;
    double      confidence;
};

// ─────────────────────────────────────────────
//  PlayerIdentifier
//  Assigns player numbers to detected actions.
//
//  Since the Python model detects actions but
//  not specific jersey numbers, we use position
//  on court and rally context to assign players.
//
//  Home team: players 1-6
//  Away team: players 7-12
// ─────────────────────────────────────────────
class PlayerIdentifier {
private:
    std::string homeTeam;
    std::string awayTeam;

    // Running action count per player (for stats)
    std::map<int, int> playerActionCount;

    // Determines which player likely performed an action
    // based on action type and rally context
    int assignPlayerId(const std::string& actionType,
                       int rallyNumber,
                       int actionIndexInRally) const;

    std::string getTeamForPlayer(int playerId) const;

public:
    PlayerIdentifier(const std::string& homeTeam,
                     const std::string& awayTeam);

    // Process a flat list of events from events.json
    // Returns a list of PlayerActions with player IDs assigned
    std::vector<PlayerAction> identifyPlayers(
        const std::vector<std::tuple<std::string,  // type
                                     double,        // timestamp
                                     int,           // rally_number
                                     double>>       // confidence
        & rawEvents
    ) const;

    // Print a summary of actions per player
    void printPlayerSummary(const std::vector<PlayerAction>& actions) const;
};