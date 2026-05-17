#include "MatchEvent.h"
#include <iostream>
#include <iomanip>

// ── MatchEvent (base) ─────────────────────────
MatchEvent::MatchEvent(int id, double ts, int pid, bool ok)
    : eventId(id), timestamp(ts), playerId(pid), successful(ok) {}

int    MatchEvent::getEventId()   const { return eventId; }
double MatchEvent::getTimestamp() const { return timestamp; }
int    MatchEvent::getPlayerId()  const { return playerId; }
bool   MatchEvent::isSuccessful() const { return successful; }

std::string MatchEvent::getSummary() const {
    return "[" + getEventType() + "] t=" + std::to_string((int)timestamp) +
           "s  Player#" + std::to_string(playerId) +
           "  " + (successful ? "SUCCESS" : "FAIL");
}

void MatchEvent::print() const {
    std::cout << std::fixed << std::setprecision(1)
              << "  Event #" << eventId
              << " | " << getEventType()
              << " | @" << timestamp << "s"
              << " | Player #" << playerId
              << " | " << (successful ? "✅" : "❌") << "\n";
}

// ── Serve ─────────────────────────────────────
Serve::Serve(int id, double ts, int pid, bool ok, const std::string& type)
    : MatchEvent(id, ts, pid, ok), serveType(type) {}

std::string Serve::getServeType() const { return serveType; }
std::string Serve::getEventType() const { return "Serve"; }

std::string Serve::getSummary() const {
    return MatchEvent::getSummary() + "  Type:" + serveType;
}
void Serve::print() const {
    MatchEvent::print();
    std::cout << "    Serve type: " << serveType << "\n";
}

// ── Attack ────────────────────────────────────
Attack::Attack(int id, double ts, int pid, bool ok, const std::string& z, int spd)
    : MatchEvent(id, ts, pid, ok), zone(z), speed(spd) {}

std::string Attack::getZone()      const { return zone; }
int         Attack::getSpeed()     const { return speed; }
std::string Attack::getEventType() const { return "Attack"; }

std::string Attack::getSummary() const {
    return MatchEvent::getSummary() + "  Zone:" + zone + "  Speed:" + std::to_string(speed) + "km/h";
}
void Attack::print() const {
    MatchEvent::print();
    std::cout << "    Zone: " << zone << "  |  Speed: " << speed << " km/h\n";
}

// ── Block ─────────────────────────────────────
Block::Block(int id, double ts, int pid, bool ok, int nb)
    : MatchEvent(id, ts, pid, ok), numBlockers(nb) {}

int         Block::getNumBlockers() const { return numBlockers; }
std::string Block::getEventType()   const { return "Block"; }

std::string Block::getSummary() const {
    return MatchEvent::getSummary() + "  Blockers:" + std::to_string(numBlockers);
}
void Block::print() const {
    MatchEvent::print();
    std::cout << "    Blockers: " << numBlockers << "\n";
}

// ── Defense ───────────────────────────────────
Defense::Defense(int id, double ts, int pid, bool ok, const std::string& dtype)
    : MatchEvent(id, ts, pid, ok), defenseType(dtype) {}

std::string Defense::getDefenseType() const { return defenseType; }
std::string Defense::getEventType()   const { return "Defense"; }

std::string Defense::getSummary() const {
    return MatchEvent::getSummary() + "  DefenseType:" + defenseType;
}
void Defense::print() const {
    MatchEvent::print();
    std::cout << "    Defense type: " << defenseType << "\n";
}
