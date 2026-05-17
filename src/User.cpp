#include "User.h"
#include "AnalysisProcessor.h"
#include "VideoFile.h"
#include <iostream>

// ── User ──────────────────────────────────────
User::User(int id, const std::string& n, const std::string& e)
    : userId(id), name(n), email(e) {}

int         User::getUserId() const { return userId; }
std::string User::getName()   const { return name; }
std::string User::getEmail()  const { return email; }

std::string User::getRole() const { return "User"; }

void User::viewAnalysis(const MatchAnalysis& analysis) const {
    std::cout << "[" << getRole() << " " << name << "] Viewing analysis for match ID "
              << analysis.getMatchId() << " ("
              << analysis.getHomeTeam() << " vs " << analysis.getAwayTeam() << ")\n";
}

void User::print() const {
    std::cout << getRole() << " | ID: " << userId
              << " | Name: " << name
              << " | Email: " << email << "\n";
}

// ── Coach ─────────────────────────────────────
Coach::Coach(int id, const std::string& n, const std::string& e, const std::string& team)
    : User(id, n, e), teamName(team) {}

std::string Coach::getTeamName() const { return teamName; }
std::string Coach::getRole()     const { return "Coach"; }

std::string Coach::uploadVideo(VideoFile& video) {
    ValidationResult res = video.validate();
    if (res.valid) {
        managedMatchIds.push_back(video.fileId);
        return "✅  Coach " + name + " successfully uploaded '" + video.filename + "'";
    }
    return "❌  Upload failed: " + res.reason;
}

std::vector<int> Coach::getManagedMatchIds() const { return managedMatchIds; }

void Coach::print() const {
    User::print();
    std::cout << "   Team: " << teamName
              << " | Managed matches: " << managedMatchIds.size() << "\n";
}

// ── Analyst ───────────────────────────────────
Analyst::Analyst(int id, const std::string& n, const std::string& e, const std::string& spec)
    : User(id, n, e), specialization(spec), reportsGenerated(0) {}

std::string Analyst::getSpecialization()   const { return specialization; }
int         Analyst::getReportsGenerated() const { return reportsGenerated; }
std::string Analyst::getRole()             const { return "Analyst"; }

std::string Analyst::generateReport(const MatchAnalysis& analysis) {
    ++reportsGenerated;
    std::string report;
    report += "══════════════════════════════════════\n";
    report += "  MATCH REPORT #" + std::to_string(reportsGenerated) + "\n";
    report += "  Analyst : " + name + " (" + specialization + ")\n";
    report += "  Match ID: " + std::to_string(analysis.getMatchId()) + "\n";
    report += "══════════════════════════════════════\n";
    report += analysis.getSummary() + "\n";
    report += "── Recommendations ──\n";
    report += analysis.getRecommendations() + "\n";
    report += "══════════════════════════════════════\n";
    return report;
}

void Analyst::print() const {
    User::print();
    std::cout << "   Specialization: " << specialization
              << " | Reports generated: " << reportsGenerated << "\n";
}
