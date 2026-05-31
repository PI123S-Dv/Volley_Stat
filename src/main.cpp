#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <tuple>
#include "User.h"
#include "VideoFile.h"
#include "MatchEvent.h"
#include "Statistics.h"
#include "AnalysisProcessor.h"
#include "JSONReader.h"
#include "ScoreTracker.h"
#include "PlayerIdentifier.h"
#include "Dashboard.h"

// ── Helper: read rallies.json for ScoreTracker ────────────
// Returns list of (rallyNumber, lastAction, duration)
std::vector<std::tuple<int,std::string,double>>
loadRallies(const std::string& path) {
    std::vector<std::tuple<int,std::string,double>> rallies;
    std::ifstream file(path);
    if (!file.is_open()) return rallies;

    int    rallyNum   = 0;
    double duration   = 0.0;
    std::string lastAction;
    std::string line;

    while (std::getline(file, line)) {
        // rally_number
        if (line.find("\"rally_number\"") != std::string::npos) {
            size_t colon = line.find(':');
            if (colon != std::string::npos)
                rallyNum = std::stoi(line.substr(colon + 1));
        }
        // duration
        if (line.find("\"duration\"") != std::string::npos) {
            size_t colon = line.find(':');
            if (colon != std::string::npos)
                duration = std::stod(line.substr(colon + 1));
        }
        // last action — we track "type" and keep overwriting
        // so the last one we see is the final event of the rally
        if (line.find("\"type\"") != std::string::npos) {
            size_t q1 = line.find('"', line.find(':') + 1);
            size_t q2 = (q1 != std::string::npos)
                        ? line.find('"', q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos)
                lastAction = line.substr(q1 + 1, q2 - q1 - 1);
        }
        // End of rally object
        if (line.find("\"event_count\"") != std::string::npos && rallyNum > 0) {
            rallies.emplace_back(rallyNum, lastAction, duration);
            rallyNum = 0; duration = 0.0; lastAction = "";
        }
    }
    return rallies;
}

// ── Helper: read events.json for PlayerIdentifier ─────────
std::vector<std::tuple<std::string,double,int,double>>
loadEvents(const std::string& path) {
    std::vector<std::tuple<std::string,double,int,double>> events;
    std::ifstream file(path);
    if (!file.is_open()) return events;

    std::string type;
    double      timestamp  = 0.0;
    int         rallyNum   = 0;
    double      confidence = 0.0;
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("\"type\"") != std::string::npos) {
            size_t q1 = line.find('"', line.find(':') + 1);
            size_t q2 = (q1 != std::string::npos)
                        ? line.find('"', q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos)
                type = line.substr(q1 + 1, q2 - q1 - 1);
        }
        if (line.find("\"timestamp\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos) timestamp = std::stod(line.substr(c+1));
        }
        if (line.find("\"rally_number\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos) rallyNum = std::stoi(line.substr(c+1));
        }
        if (line.find("\"confidence\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos) confidence = std::stod(line.substr(c+1));
        }
        // End of event object — save it
        if (line.find('}') != std::string::npos && !type.empty()) {
            events.emplace_back(type, timestamp, rallyNum, confidence);
            type = ""; timestamp = 0.0; rallyNum = 0; confidence = 0.0;
        }
    }
    return events;
}

// ─────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────
int main() {
    std::cout << "\n╔══════════════════════════════════════╗\n";
    std::cout << "║  Volleyball Analysis Platform  v3.0  ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";

    const std::string HOME_TEAM  = "Levski";
    const std::string AWAY_TEAM  = "CSKA";
    const std::string MATCH_DATE = "2025-03-15";

    // ── FEATURE 1: User hierarchy ─────────────────────────
    std::cout << "=== FEATURE 1: Users ===\n";
    Coach   coach(1, "Ivan Petrov",     "ivan@club.bg",  "Levski Volleyball");
    Analyst analyst(2, "Maria Georgieva","maria@club.bg", "Defensive Play");
    coach.print();
    analyst.print();

    // ── FEATURE 2: Video validation ───────────────────────
    std::cout << "\n=== FEATURE 2: Video Validation ===\n";
    VideoFile video(101, "match_levski_cska.mp4",
                    "/videos/match101.mp4", 250.0, "1080p");
    std::cout << coach.uploadVideo(video) << "\n";

    // ── FEATURE 3: Load match analysis ────────────────────
    std::cout << "\n=== FEATURE 3: Match Analysis ===\n";
    Dashboard       dash;
    ReportGenerator repGen;

    JSONReader reader("events.json");
    int eventCount = reader.countEvents();
    std::unique_ptr<MatchAnalysis> analysis;

    if (eventCount > 0) {
        std::cout << "📂  Found events.json (" << eventCount
                  << " events) — loading real match data\n";
        analysis = reader.load(1, MATCH_DATE, HOME_TEAM, AWAY_TEAM);
    } else {
        std::cout << "ℹ   No events.json — running simulation\n";
        std::cout << "    (Run analyze.py first for real data)\n\n";
        AnalysisProcessor processor;
        analysis = processor.processVideo(video, 1, MATCH_DATE,
                                          HOME_TEAM, AWAY_TEAM);
    }

    if (!analysis) { std::cerr << "❌  Analysis failed.\n"; return 1; }

    // ── FEATURE 4: Dashboard ──────────────────────────────
    std::cout << "\n=== FEATURE 4: Dashboard ===\n";
    dash.showScoreboard(*analysis);
    dash.showTimeline(*analysis);

    // ── FEATURE 5: Score tracking ─────────────────────────
    std::cout << "\n=== FEATURE 5: Score Tracker ===\n";
    ScoreTracker scoreTracker(HOME_TEAM, AWAY_TEAM);

    auto rallies = loadRallies("rallies.json");
    if (!rallies.empty()) {
        std::cout << "📂  Loaded " << rallies.size()
                  << " rallies from rallies.json\n";
        for (const auto& [num, lastAction, dur] : rallies)
            scoreTracker.processRally(num, lastAction, dur);
        scoreTracker.printSummary();
    } else {
        std::cout << "ℹ   No rallies.json found — skipping score tracking\n";
        std::cout << "    (Run analyze.py first)\n";
    }

    // ── FEATURE 6: Player identification ──────────────────
    std::cout << "\n=== FEATURE 6: Player Identification ===\n";
    PlayerIdentifier identifier(HOME_TEAM, AWAY_TEAM);

    auto rawEvents = loadEvents("events.json");
    if (!rawEvents.empty()) {
        auto playerActions = identifier.identifyPlayers(rawEvents);
        identifier.printPlayerSummary(playerActions);
    } else {
        std::cout << "ℹ   No events.json found — skipping player ID\n";
    }

    // ── FEATURE 7: Report generation ──────────────────────
    std::cout << "\n=== FEATURE 7: Report Generation ===\n";
    std::string report = analyst.generateReport(*analysis);
    std::cout << report;
    repGen.exportToFile(report, "match_report.txt");

    // ── Polymorphism demo ─────────────────────────────────
    std::cout << "\n=== Polymorphism: All MatchEvent types ===\n";
    std::vector<std::unique_ptr<MatchEvent>> demo;
    demo.push_back(std::make_unique<Serve>  (200, 10.5, 3, true,  "jump"));
    demo.push_back(std::make_unique<Attack> (201, 25.0, 5, false, "4", 95));
    demo.push_back(std::make_unique<Block>  (202, 40.2, 1, true,  2));
    demo.push_back(std::make_unique<Defense>(203, 55.8, 6, true,  "dig"));
    for (const auto& ev : demo)
        std::cout << ev->getSummary() << "\n";

    std::cout << "\n✅  Done.\n\n";
    return 0;
}