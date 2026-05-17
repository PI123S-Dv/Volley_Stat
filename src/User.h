#pragma once
#include <string>
#include <vector>

// Forward declaration
class MatchAnalysis;

// ─────────────────────────────────────────────
//  Base class: User
// ─────────────────────────────────────────────
class User {
protected:
    int         userId;
    std::string name;
    std::string email;

public:
    User(int id, const std::string& name, const std::string& email);
    virtual ~User() = default;

    int         getUserId()  const;
    std::string getName()    const;
    std::string getEmail()   const;

    virtual std::string getRole() const;
    virtual void        viewAnalysis(const MatchAnalysis& analysis) const;

    virtual void print() const;
};

// ─────────────────────────────────────────────
//  Derived class: Coach
// ─────────────────────────────────────────────
class Coach : public User {
private:
    std::string          teamName;
    std::vector<int>     managedMatchIds;   // private – only Coach tracks this

public:
    Coach(int id, const std::string& name,
          const std::string& email, const std::string& teamName);

    std::string getTeamName() const;
    std::string getRole()     const override;

    // Returns success/failure message
    std::string uploadVideo(class VideoFile& video);

    std::vector<int> getManagedMatchIds() const;
    void print() const override;
};

// ─────────────────────────────────────────────
//  Derived class: Analyst
// ─────────────────────────────────────────────
class Analyst : public User {
private:
    std::string specialization;
    int         reportsGenerated;

public:
    Analyst(int id, const std::string& name,
            const std::string& email,
            const std::string& specialization = "General");

    std::string getSpecialization()  const;
    int         getReportsGenerated() const;
    std::string getRole()            const override;

    // Generates a report string from a MatchAnalysis
    std::string generateReport(const MatchAnalysis& analysis);

    void print() const override;
};
