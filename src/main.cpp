#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <tuple>
#include <string>
#include <limits>
#include "User.h"
#include "VideoFile.h"
#include "MatchEvent.h"
#include "Statistics.h"
#include "AnalysisProcessor.h"
#include "JSONReader.h"
#include "ScoreTracker.h"
#include "PlayerIdentifier.h"
#include "Dashboard.h"

static void bar(char c = '=', int len = 50) {
    std::cout << std::string(len, c) << "\n";
}
static void banner(const std::string& title) {
    std::cout << "\n";
    bar();
    std::cout << "  " << title << "\n";
    bar();
}

// ── Simple credential store ───────────────────
struct Account {
    std::string username;
    std::string password;
    std::string fullName;
    std::string email;
    std::string teamName;
};

static const std::vector<Account> ACCOUNTS = {
    {"coach1", "pass123", "Ivan Petrov",     "ivan@club.bg",  "Team A"},
    {"coach2", "pass456", "Maria Georgieva", "maria@club.bg", "Team B"},
    {"admin",  "admin",   "Admin Coach",     "admin@club.bg", "Demo Team"},
};

// ── Login ─────────────────────────────────────
const Account* login() {
    bar();
    std::cout << "  VOLLEYBALL ANALYSIS PLATFORM  v3.0\n";
    bar();
    std::cout << "\n  Please log in:\n\n";

    for (int attempt = 1; attempt <= 3; ++attempt) {
        std::string username, password;
        std::cout << "  Username: ";
        std::cin >> username;
        std::cout << "  Password: ";
        std::cin >> password;

        for (const auto& acc : ACCOUNTS) {
            if (acc.username == username && acc.password == password) {
                std::cout << "\n  Welcome, " << acc.fullName << "!\n";
                bar('-');
                return &acc;
            }
        }
        std::cout << "  Incorrect credentials.";
        if (attempt < 3)
            std::cout << " Try again (" << (3 - attempt) << " left).";
        std::cout << "\n\n";
    }
    return nullptr;
}

// ── Read rallies.json ─────────────────────────
std::vector<std::tuple<int,std::string,double>>
loadRallies(const std::string& path) {
    std::vector<std::tuple<int,std::string,double>> rallies;
    std::ifstream file(path);
    if (!file.is_open()) return rallies;

    int         rallyNum   = 0;
    double      duration   = 0.0;
    std::string lastAction;
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("\"rally_number\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos)
                try { rallyNum = std::stoi(line.substr(c + 1)); } catch(...) {}
        }
        if (line.find("\"duration\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos)
                try { duration = std::stod(line.substr(c + 1)); } catch(...) {}
        }
        if (line.find("\"type\"") != std::string::npos) {
            size_t q1 = line.find('"', line.find(':') + 1);
            size_t q2 = (q1 != std::string::npos)
                        ? line.find('"', q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos)
                lastAction = line.substr(q1 + 1, q2 - q1 - 1);
        }
        if (line.find("\"event_count\"") != std::string::npos && rallyNum > 0) {
            rallies.emplace_back(rallyNum, lastAction, duration);
            rallyNum = 0; duration = 0.0; lastAction = "";
        }
    }
    return rallies;
}

// ── Read events.json ──────────────────────────
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
            if (c != std::string::npos)
                try { timestamp = std::stod(line.substr(c+1)); } catch(...) {}
        }
        if (line.find("\"rally_number\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos)
                try { rallyNum = std::stoi(line.substr(c+1)); } catch(...) {}
        }
        if (line.find("\"confidence\"") != std::string::npos) {
            size_t c = line.find(':');
            if (c != std::string::npos)
                try { confidence = std::stod(line.substr(c+1)); } catch(...) {}
        }
        if (line.find('}') != std::string::npos && !type.empty()) {
            events.emplace_back(type, timestamp, rallyNum, confidence);
            type = ""; timestamp = 0.0; rallyNum = 0; confidence = 0.0;
        }
    }
    return events;
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main() {
    std::cout << "\n";

    // ── LOGIN ─────────────────────────────────
    const Account* acc = login();
    if (!acc) {
        std::cout << "  Too many failed attempts. Exiting.\n\n";
        return 1;
    }

    Coach   coach(1, acc->fullName, acc->email, acc->teamName);
    Analyst analyst(2, acc->fullName, acc->email, "Match Analysis");

    // ── FEATURE 1: Logged in user ─────────────
    banner("FEATURE 1: Logged In User");
    coach.print();

    // ── FEATURE 2: Video validation ───────────
    banner("FEATURE 2: Video Validation");
    std::string videoName;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "  Enter video filename (press Enter for test_match.mp4): ";
    std::getline(std::cin, videoName);
    if (videoName.empty()) videoName = "test_match.mp4";

    VideoFile video(101, videoName, "./" + videoName, 250.0, "1080p");
    std::cout << "  " << coach.uploadVideo(video) << "\n";

    // ── FEATURE 3: Match info ─────────────────
    banner("FEATURE 3: Match Analysis");
    std::string homeTeam, awayTeam, matchDate;

    std::cout << "  Your team name (press Enter for '"
              << acc->teamName << "'): ";
    std::getline(std::cin, homeTeam);
    if (homeTeam.empty()) homeTeam = acc->teamName;

    std::cout << "  Opponent team name (press Enter for 'Opponent'): ";
    std::getline(std::cin, awayTeam);
    if (awayTeam.empty()) awayTeam = "Opponent";

    std::cout << "  Match date YYYY-MM-DD (press Enter for today): ";
    std::getline(std::cin, matchDate);
    if (matchDate.empty()) matchDate = "2025-01-01";

    Dashboard       dash;
    ReportGenerator repGen;

    JSONReader reader("events.json");
    int eventCount = reader.countEvents();
    std::unique_ptr<MatchAnalysis> analysis;

    if (eventCount > 0) {
        std::cout << "\n  Found events.json (" << eventCount
                  << " events) -- loading real match data\n";
        analysis = reader.load(1, matchDate, homeTeam, awayTeam);
    } else {
        std::cout << "\n  No events.json -- running simulation\n";
        std::cout << "  Run analyze.py on your video first for real data\n\n";
        AnalysisProcessor processor;
        analysis = processor.processVideo(video, 1, matchDate,
                                          homeTeam, awayTeam);
    }

    if (!analysis) {
        std::cerr << "  ERROR: Analysis failed.\n";
        return 1;
    }

    // ── FEATURE 4: Dashboard ──────────────────
    banner("FEATURE 4: Dashboard");
    dash.showScoreboard(*analysis);
    dash.showTimeline(*analysis);

    // ── FEATURE 5: Score tracking ─────────────
    banner("FEATURE 5: Score Tracker");
    ScoreTracker scoreTracker(homeTeam, awayTeam);
    auto rallies = loadRallies("rallies.json");
    if (!rallies.empty()) {
        std::cout << "  Loaded " << rallies.size() << " rallies\n\n";
        for (const auto& [num, lastAction, dur] : rallies)
            scoreTracker.processRally(num, lastAction, dur);
        scoreTracker.printSummary();
    } else {
        std::cout << "  No rallies.json -- run analyze.py first\n";
    }

    // ── FEATURE 6: Player identification ──────
    banner("FEATURE 6: Player Identification");
    PlayerIdentifier identifier(homeTeam, awayTeam);
    auto rawEvents = loadEvents("events.json");
    if (!rawEvents.empty()) {
        auto playerActions = identifier.identifyPlayers(rawEvents);
        identifier.printPlayerSummary(playerActions);
    } else {
        std::cout << "  No events.json -- run analyze.py first\n";
    }

    // ── FEATURE 7: Report generation ──────────
    banner("FEATURE 7: Report Generation");
    std::string report = analyst.generateReport(*analysis);
    std::cout << report;
    repGen.exportToFile(report, "match_report.txt");

    // ── FEATURE 8: Polymorphism demo ──────────
    banner("FEATURE 8: Event Type Hierarchy");
    std::vector<std::unique_ptr<MatchEvent>> demo;
    demo.push_back(std::make_unique<Serve>  (200, 10.5, 3, true,  "jump"));
    demo.push_back(std::make_unique<Attack> (201, 25.0, 5, false, "4", 95));
    demo.push_back(std::make_unique<Block>  (202, 40.2, 1, true,  2));
    demo.push_back(std::make_unique<Defense>(203, 55.8, 6, true,  "dig"));
    for (const auto& ev : demo)
        std::cout << "  " << ev->getSummary() << "\n";

    std::cout << "\n";
    bar();
    std::cout << "  Analysis complete. Goodbye, " << acc->fullName << "!\n";
    bar();
    std::cout << "\n";
    return 0;
}