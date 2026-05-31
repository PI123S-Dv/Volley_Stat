#include "JSONReader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

JSONReader::JSONReader(const std::string& fp) : filePath(fp) {}

std::string JSONReader::trim(const std::string& s) const {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    size_t end   = s.find_last_not_of(" \t\r\n\",");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

std::string JSONReader::extractString(const std::string& line,
                                      const std::string& key) const {
    std::string search = "\"" + key + "\"";
    size_t pos = line.find(search);
    if (pos == std::string::npos) return "";
    size_t colon = line.find(':', pos);
    if (colon == std::string::npos) return "";
    size_t q1 = line.find('"', colon + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = line.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";
    return line.substr(q1 + 1, q2 - q1 - 1);
}

double JSONReader::extractDouble(const std::string& line,
                                 const std::string& key) const {
    std::string search = "\"" + key + "\"";
    size_t pos = line.find(search);
    if (pos == std::string::npos) return 0.0;
    size_t colon = line.find(':', pos);
    if (colon == std::string::npos) return 0.0;
    size_t numStart = line.find_first_not_of(" \t", colon + 1);
    if (numStart == std::string::npos) return 0.0;
    try { return std::stod(line.substr(numStart)); } catch (...) { return 0.0; }
}

std::unique_ptr<MatchEvent> JSONReader::parseEvent(
    int eventId,
    const std::string& type,
    double timestamp,
    int playerId,
    bool successful) const
{
    // Serve
    if (type == "Serve" || type == "serve")
        return std::make_unique<Serve>(eventId, timestamp, playerId,
                                       successful, "detected");
    // Attack / Spike
    if (type == "Attack" || type == "attack" ||
        type == "Spike"  || type == "spike")
        return std::make_unique<Attack>(eventId, timestamp, playerId,
                                        successful, "detected", 0);
    // Block
    if (type == "Block" || type == "block")
        return std::make_unique<Block>(eventId, timestamp, playerId,
                                       successful, 1);
    // Dig / Defense / Receive / Set — all map to Defense
    if (type == "Dig"     || type == "dig"     ||
        type == "Defense" || type == "defense" ||
        type == "Receive" || type == "receive" ||
        type == "Set"     || type == "set")
        return std::make_unique<Defense>(eventId, timestamp, playerId,
                                         successful, type);

    // Unknown — still save it as Defense, no warning printed
    return std::make_unique<Defense>(eventId, timestamp, playerId,
                                     successful, type);
}

int JSONReader::countEvents() const {
    std::ifstream file(filePath);
    if (!file.is_open()) return -1;
    int count = 0;
    std::string line;
    while (std::getline(file, line))
        if (line.find("\"type\"") != std::string::npos) ++count;
    return count;
}

std::unique_ptr<MatchAnalysis> JSONReader::load(
    int matchId,
    const std::string& matchDate,
    const std::string& homeTeam,
    const std::string& awayTeam) const
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "  ERROR: Cannot open '" << filePath << "'\n";
        std::cout << "  Run analyze.py first.\n";
        return nullptr;
    }

    std::cout << "  Reading '" << filePath << "'...\n";

    auto analysis = std::make_unique<MatchAnalysis>(
        matchId, matchDate, homeTeam, awayTeam);

    std::string currentType;
    double      currentTimestamp  = 0.0;
    double      currentConfidence = 0.0;
    int         currentRally      = 0;
    int         eventId           = 1;
    int         eventsLoaded      = 0;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("\"type\"") != std::string::npos)
            currentType = extractString(line, "type");
        if (line.find("\"timestamp\"") != std::string::npos)
            currentTimestamp = extractDouble(line, "timestamp");
        if (line.find("\"confidence\"") != std::string::npos)
            currentConfidence = extractDouble(line, "confidence");
        if (line.find("\"rally_number\"") != std::string::npos)
            currentRally = (int)extractDouble(line, "rally_number");

        if (line.find('}') != std::string::npos && !currentType.empty()) {
            int  playerId   = (currentRally % 2 != 0) ? 1 : 7;
            bool successful = (currentConfidence >= 0.5);

            auto event = parseEvent(eventId++, currentType,
                                    currentTimestamp, playerId, successful);
            if (event) {
                analysis->addEvent(std::move(event));
                ++eventsLoaded;
            }
            currentType = ""; currentTimestamp = 0.0;
            currentConfidence = 0.0;
        }
    }

    if (eventsLoaded == 0) {
        std::cout << "  WARNING: No events found in '" << filePath << "'\n";
        return nullptr;
    }

    analysis->buildStatisticsFromEvents();
    std::cout << "  Loaded " << eventsLoaded << " events successfully\n";
    return analysis;
}