#pragma once
#include <string>
#include <memory>
#include "AnalysisProcessor.h"

// ─────────────────────────────────────────────
//  JSONReader
//  Reads events.json produced by analyze.py
//  and builds a real MatchAnalysis object.
//
//  This is the bridge between the Python AI
//  pipeline and the C++ analysis system.
// ─────────────────────────────────────────────
class JSONReader {
private:
    std::string filePath;

    // Private helpers
    std::string trim(const std::string& s) const;
    std::string extractString(const std::string& line,
                              const std::string& key) const;
    double      extractDouble(const std::string& line,
                              const std::string& key) const;

    // Turn one JSON event object into the right MatchEvent subtype
    std::unique_ptr<MatchEvent> parseEvent(
        int eventId,
        const std::string& type,
        double timestamp,
        int playerId,
        bool successful
    ) const;

public:
    explicit JSONReader(const std::string& filePath);

    // Reads the JSON file and returns a complete MatchAnalysis
    // Returns nullptr if file cannot be opened or is empty
    std::unique_ptr<MatchAnalysis> load(
        int matchId,
        const std::string& matchDate,
        const std::string& homeTeam,
        const std::string& awayTeam
    ) const;

    // Quick check — returns number of events in the file
    // Returns -1 if file cannot be opened
    int countEvents() const;
};