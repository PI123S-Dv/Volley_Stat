#pragma once
#include <string>

// ─────────────────────────────────────────────
//  Base class: MatchEvent
// ─────────────────────────────────────────────
class MatchEvent {
protected:
    int         eventId;
    double      timestamp;      // seconds into the video
    int         playerId;
    bool        successful;

public:
    MatchEvent(int eventId, double timestamp, int playerId, bool successful);
    virtual ~MatchEvent() = default;

    int    getEventId()   const;
    double getTimestamp() const;
    int    getPlayerId()  const;
    bool   isSuccessful() const;

    virtual std::string getEventType() const = 0;   // pure virtual
    virtual std::string getSummary()   const;
    virtual void        print()        const;
};

// ─────────────────────────────────────────────
//  Derived: Serve
// ─────────────────────────────────────────────
class Serve : public MatchEvent {
private:
    std::string serveType;   // "jump", "float", "underhand"

public:
    Serve(int eventId, double timestamp, int playerId,
          bool successful, const std::string& serveType);

    std::string getServeType() const;
    std::string getEventType() const override;
    std::string getSummary()   const override;
    void        print()        const override;
};

// ─────────────────────────────────────────────
//  Derived: Attack
// ─────────────────────────────────────────────
class Attack : public MatchEvent {
private:
    std::string zone;    // court zone "1"–"6"
    int         speed;   // km/h

public:
    Attack(int eventId, double timestamp, int playerId,
           bool successful, const std::string& zone, int speed);

    std::string getZone()      const;
    int         getSpeed()     const;
    std::string getEventType() const override;
    std::string getSummary()   const override;
    void        print()        const override;
};

// ─────────────────────────────────────────────
//  Derived: Block
// ─────────────────────────────────────────────
class Block : public MatchEvent {
private:
    int numBlockers;   // solo=1, double=2, triple=3

public:
    Block(int eventId, double timestamp, int playerId,
          bool successful, int numBlockers);

    int         getNumBlockers() const;
    std::string getEventType()   const override;
    std::string getSummary()     const override;
    void        print()          const override;
};

// ─────────────────────────────────────────────
//  Derived: Defense
// ─────────────────────────────────────────────
class Defense : public MatchEvent {
private:
    std::string defenseType;   // "dig", "receive", "libero"

public:
    Defense(int eventId, double timestamp, int playerId,
            bool successful, const std::string& defenseType);

    std::string getDefenseType() const;
    std::string getEventType()   const override;
    std::string getSummary()     const override;
    void        print()          const override;
};
