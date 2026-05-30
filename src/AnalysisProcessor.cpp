#include "AnalysisProcessor.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <cstdlib>   // rand

// ── MatchAnalysis ─────────────────────────────
MatchAnalysis::MatchAnalysis(int mid, const std::string& date,
                             const std::string& home, const std::string& away)
    : matchId(mid), matchDate(date), homeTeam(home), awayTeam(away),
      homeStats(home), awayStats(away) {}

void MatchAnalysis::addEvent(std::unique_ptr<MatchEvent> ev) {
    events.push_back(std::move(ev));
}

void MatchAnalysis::buildStatistics(
    const std::vector<PlayerStatistics>& homePlayers,
    const std::vector<PlayerStatistics>& awayPlayers)
{
    for (const auto& ps : homePlayers) homeStats.addPlayerStats(ps);
    for (const auto& ps : awayPlayers) awayStats.addPlayerStats(ps);
}

void MatchAnalysis::buildStatisticsFromEvents() {
    // Automatically build player stats from the events already loaded.
    // Since we don't know which team each player belongs to from JSON alone,
    // we split by player ID: odd IDs = home, even IDs = away.
    // This is a simple heuristic — can be improved with real player data.
    std::map<int, PlayerStatistics> homePlayers, awayPlayers;

    for (const auto& ev : events) {
        int pid = ev->getPlayerId();
        std::string pname = "Player#" + std::to_string(pid);

        if (pid % 2 != 0) {
            // Odd player ID = home team
            if (homePlayers.find(pid) == homePlayers.end())
                homePlayers.emplace(pid, PlayerStatistics(pid, pname));
            homePlayers.at(pid).processEvent(*ev);
        } else {
            // Even player ID = away team
            if (awayPlayers.find(pid) == awayPlayers.end())
                awayPlayers.emplace(pid, PlayerStatistics(pid, pname));
            awayPlayers.at(pid).processEvent(*ev);
        }
    }

    for (auto& [id, ps] : homePlayers) homeStats.addPlayerStats(ps);
    for (auto& [id, ps] : awayPlayers) awayStats.addPlayerStats(ps);
}

int         MatchAnalysis::getMatchId()     const { return matchId; }
std::string MatchAnalysis::getMatchDate()   const { return matchDate; }
std::string MatchAnalysis::getHomeTeam()    const { return homeTeam; }
std::string MatchAnalysis::getAwayTeam()    const { return awayTeam; }
int         MatchAnalysis::getTotalEvents() const { return (int)events.size(); }

const std::vector<std::unique_ptr<MatchEvent>>& MatchAnalysis::getEvents() const { return events; }
const TeamStatistics& MatchAnalysis::getHomeStats() const { return homeStats; }
const TeamStatistics& MatchAnalysis::getAwayStats() const { return awayStats; }

std::string MatchAnalysis::getSummary() const {
    return "Match #" + std::to_string(matchId) + "  " + matchDate + "\n" +
           homeTeam + " vs " + awayTeam + "\n" +
           "Home points : " + std::to_string(homeStats.getTotalTeamPoints()) + "\n" +
           "Away points : " + std::to_string(awayStats.getTotalTeamPoints()) + "\n" +
           "Total events: " + std::to_string(events.size());
}

std::string MatchAnalysis::getRecommendations() const {
    std::string rec;
    if (homeStats.getTeamEfficiency() < 0.5)
        rec += "• " + homeTeam + " should improve attack efficiency.\n";
    if (awayStats.getTeamEfficiency() < 0.5)
        rec += "• " + awayTeam + " should improve attack efficiency.\n";
    if (rec.empty())
        rec = "• Both teams performed well. Focus on consistency.\n";
    return rec;
}

void MatchAnalysis::printTimeline() const {
    std::cout << "\n── Event Timeline (" << events.size() << " events) ──\n";
    for (const auto& ev : events) ev->print();
}

void MatchAnalysis::printScoreboard() const {
    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  MATCH #" << matchId << "  |  " << matchDate << "\n";
    std::cout << "  " << homeTeam << " vs " << awayTeam << "\n";
    std::cout << "══════════════════════════════════════\n";
    homeStats.print();
    std::cout << "──────────────────────────────────────\n";
    awayStats.print();
    std::cout << "══════════════════════════════════════\n";
}

// ── AnalysisProcessor ─────────────────────────
AnalysisProcessor::AnalysisProcessor() : nextEventId(1) {}

std::unique_ptr<MatchEvent> AnalysisProcessor::generateServe(int pid, double ts) {
    static const char* types[] = {"jump", "float", "underhand"};
    bool ok = (rand() % 3) != 0;
    return std::make_unique<Serve>(nextEventId++, ts, pid, ok, types[rand() % 3]);
}
std::unique_ptr<MatchEvent> AnalysisProcessor::generateAttack(int pid, double ts) {
    static const char* zones[] = {"1","2","3","4","5","6"};
    int speed = 60 + rand() % 60;
    bool ok = (rand() % 2) == 0;
    return std::make_unique<Attack>(nextEventId++, ts, pid, ok, zones[rand() % 6], speed);
}
std::unique_ptr<MatchEvent> AnalysisProcessor::generateBlock(int pid, double ts) {
    int nb = 1 + rand() % 3;
    bool ok = (rand() % 2) == 0;
    return std::make_unique<Block>(nextEventId++, ts, pid, ok, nb);
}
std::unique_ptr<MatchEvent> AnalysisProcessor::generateDefense(int pid, double ts) {
    static const char* dtype[] = {"dig","receive","libero"};
    bool ok = (rand() % 3) != 0;
    return std::make_unique<Defense>(nextEventId++, ts, pid, ok, dtype[rand() % 3]);
}

std::unique_ptr<MatchAnalysis> AnalysisProcessor::processVideo(
    VideoFile& video,
    int matchId, const std::string& matchDate,
    const std::string& homeTeam, const std::string& awayTeam)
{
    if (!video.getIsValidated()) {
        std::cout << "⚠  Video not validated. Running validation...\n";
        auto res = video.validate();
        if (!res.valid) {
            std::cout << "❌  Cannot process: " << res.reason << "\n";
            return nullptr;
        }
    }

    std::cout << "🎥  AI processing '" << video.filename << "'...\n";

    auto analysis = std::make_unique<MatchAnalysis>(matchId, matchDate, homeTeam, awayTeam);

    srand(42);
    double duration = 3600.0;

    std::vector<PlayerStatistics> homePlayers, awayPlayers;
    for (int i = 1; i <= 6; ++i)
        homePlayers.emplace_back(i, "HomePlayer#" + std::to_string(i));
    for (int i = 7; i <= 12; ++i)
        awayPlayers.emplace_back(i, "AwayPlayer#" + std::to_string(i));

    for (int i = 0; i < 30; ++i) {
        double ts       = (duration / 30.0) * i + (rand() % 60);
        int teamPick    = rand() % 2;
        int playerIdx   = rand() % 6;
        int pid         = teamPick == 0 ? (playerIdx + 1) : (playerIdx + 7);
        int eventType   = rand() % 4;

        std::unique_ptr<MatchEvent> ev;
        switch (eventType) {
            case 0: ev = generateServe(pid, ts);    break;
            case 1: ev = generateAttack(pid, ts);   break;
            case 2: ev = generateBlock(pid, ts);    break;
            default: ev = generateDefense(pid, ts); break;
        }

        if (teamPick == 0) homePlayers[playerIdx].processEvent(*ev);
        else               awayPlayers[playerIdx].processEvent(*ev);

        analysis->addEvent(std::move(ev));
    }

    analysis->buildStatistics(homePlayers, awayPlayers);
    std::cout << "✅  Simulated analysis complete. 30 events generated.\n";
    return analysis;
}