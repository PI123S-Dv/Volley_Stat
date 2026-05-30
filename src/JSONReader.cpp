#include "JSONReader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// ── Constructor ───────────────────────────────
JSONReader::JSONReader(const std::string& fp) : filePath(fp) {}

// ── Private helpers ───────────────────────────

std::string JSONReader::trim(const std::string& s) const {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    size_t end   = s.find_last_not_of(" \t\r\n\",");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

std::string JSONReader::extractString(const std::string& line,
                                      const std::string& key) const {
    // Finds  "key": "value"  and returns value
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
    // Finds  "key": 123.45  and returns the number
    std::string search = "\"" + key + "\"";
    size_t pos = line.find(search);
    if (pos == std::string::npos) return 0.0;

    size_t colon = line.find(':', pos);
    if (colon == std::string::npos) return 0.0;

    // Skip whitespace after colon
    size_t numStart = line.find_first_not_of(" \t", colon + 1);
    if (numStart == std::string::npos) return 0.0;

    try {
        return std::stod(line.substr(numStart));
    } catch (...) {
        return 0.0;
    }
}

std::unique_ptr<MatchEvent> JSONReader::parseEvent(
    int eventId,
    const std::string& type,
    double timestamp,
    int playerId,
    bool successful) const
{
    // Map JSON type strings to the right MatchEvent subclass
    if (type == "Serve" || type == "serve") {
        return std::make_unique<Serve>(eventId, timestamp, playerId,
                                      successful, "detected");
    }
    if (type == "Attack" || type == "attack" || type == "Spike" || type == "spike") {
        return std::make_unique<Attack>(eventId, timestamp, playerId,
                                       successful, "detected", 0);
    }
    if (type == "Block" || type == "block") {
        return std::make_unique<Block>(eventId, timestamp, playerId,
                                      successful, 1);
    }
    if (type == "Dig" || type == "dig" || type == "Defense" || type == "defense") {
        return std::make_unique<Defense>(eventId, timestamp, playerId,
                                        successful, "dig");
    }

    // Unknown type — default to Defense
    std::cout << "  ⚠  Unknown event type '" << type << "' — defaulting to Defense\n";
    return std::make_unique<Defense>(eventId, timestamp, playerId,
                                    successful, type);
}

// ── Public interface ──────────────────────────

int JSONReader::countEvents() const {
    std::ifstream file(filePath);
    if (!file.is_open()) return -1;

    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        // Each event object has a "type" field — count those
        if (line.find("\"type\"") != std::string::npos)
            ++count;
    }
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
        std::cout << "❌  JSONReader: cannot open '" << filePath << "'\n";
        std::cout << "    Make sure analyze.py has been run first.\n";
        return nullptr;
    }

    std::cout << "📂  Reading events from '" << filePath << "'...\n";

    auto analysis = std::make_unique<MatchAnalysis>(
        matchId, matchDate, homeTeam, awayTeam);

    // ── Simple line-by-line JSON parser ───────────────────
    // The JSON produced by analyze.py looks like:
    // [
    //   {
    //     "type": "Serve",
    //     "timestamp": 12.40,
    //     "confidence": 0.87,
    //     "position": {"x": 320, "y": 240},
    //     "rally_number": 1
    //   },
    //   ...
    // ]
    //
    // We read it line by line, collecting fields until we
    // have enough to build a MatchEvent, then reset.

    std::string currentType;
    double      currentTimestamp  = 0.0;
    double      currentConfidence = 0.0;
    int         currentRally      = 0;
    int         eventId           = 1;
    int         eventsLoaded      = 0;

    std::string line;
    while (std::getline(file, line)) {

        // Extract type
        if (line.find("\"type\"") != std::string::npos) {
            currentType = extractString(line, "type");
        }

        // Extract timestamp
        if (line.find("\"timestamp\"") != std::string::npos) {
            currentTimestamp = extractDouble(line, "timestamp");
        }

        // Extract confidence
        if (line.find("\"confidence\"") != std::string::npos) {
            currentConfidence = extractDouble(line, "confidence");
        }

        // Extract rally number
        if (line.find("\"rally_number\"") != std::string::npos) {
            currentRally = (int)extractDouble(line, "rally_number");
        }

        // Closing brace = end of one event object — build it now
        if (line.find('}') != std::string::npos && !currentType.empty()) {

            // Player ID: use rally number as a proxy
            // Odd rallies = home team serves, even = away team serves
            // This is approximate — real player tracking would be better
            int playerId = (currentRally % 2 != 0) ? 1 : 7;

            // Successful = confidence above 0.5
            bool successful = (currentConfidence >= 0.5);

            auto event = parseEvent(eventId++, currentType,
                                    currentTimestamp, playerId, successful);
            if (event) {
                analysis->addEvent(std::move(event));
                ++eventsLoaded;
            }

            // Reset for next event
            currentType      = "";
            currentTimestamp = 0.0;
            currentConfidence = 0.0;
        }
    }

    if (eventsLoaded == 0) {
        std::cout << "⚠  No events found in '" << filePath << "'\n";
        return nullptr;
    }

    // Build statistics from the loaded events
    analysis->buildStatisticsFromEvents();

    std::cout << "✅  Loaded " << eventsLoaded << " events from JSON\n";
    return analysis;
}