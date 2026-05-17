#include <iostream>
#include <memory>
#include "User.h"
#include "VideoFile.h"
#include "MatchEvent.h"
#include "Statistics.h"
#include "AnalysisProcessor.h"
#include "Dashboard.h"

int main() {
    std::cout << "\n╔══════════════════════════════════════╗\n";
    std::cout << "║  Volleyball Analysis Platform  v1.0  ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";

    // ─────────────────────────────────────────
    // FEATURE 1: User hierarchy (Coach + Analyst)
    // ─────────────────────────────────────────
    std::cout << "=== FEATURE 1: User Hierarchy ===\n";

    Coach   coach(1, "Ivan Petrov", "ivan@club.bg", "Levski Volleyball");
    Analyst analyst(2, "Maria Georgieva", "maria@club.bg", "Defensive Play");

    coach.print();
    analyst.print();

    // ─────────────────────────────────────────
    // FEATURE 2: Video upload with validation
    // ─────────────────────────────────────────
    std::cout << "\n=== FEATURE 2: Video Upload & Validation ===\n";

    // Valid video
    VideoFile video1(101, "match_levski_cska.mp4", "/videos/match101.mp4", 250.0, "1080p");
    std::cout << coach.uploadVideo(video1) << "\n";
    video1.print();

    // Invalid video (too small, bad resolution)
    VideoFile video2(102, "bad_quality.avi", "/videos/bad.avi", 3.0, "480p");
    std::cout << coach.uploadVideo(video2) << "\n";

    // ─────────────────────────────────────────
    // FEATURE 3: AI Analysis (AnalysisProcessor)
    // ─────────────────────────────────────────
    std::cout << "\n=== FEATURE 3: AI Video Analysis ===\n";

    AnalysisProcessor processor;
    auto analysis1 = processor.processVideo(
        video1, 1, "2025-03-15", "Levski", "CSKA"
    );

    if (!analysis1) { std::cerr << "Analysis failed.\n"; return 1; }

    // ─────────────────────────────────────────
    // FEATURE 4: Dashboard – scoreboard + timeline
    // ─────────────────────────────────────────
    std::cout << "\n=== FEATURE 4: Dashboard ===\n";

    Dashboard dash;
    dash.showScoreboard(*analysis1);
    dash.showTimeline(*analysis1);

    // ─────────────────────────────────────────
    // FEATURE 5: Report generation (Analyst)
    // ─────────────────────────────────────────
    std::cout << "\n=== FEATURE 5: Report Generation ===\n";

    std::string report = analyst.generateReport(*analysis1);
    std::cout << report;

    ReportGenerator repGen;
    repGen.exportToFile(report, "match_report.txt");

    // ─────────────────────────────────────────
    // FEATURE 6: Match comparison
    // ─────────────────────────────────────────
    std::cout << "\n=== FEATURE 6: Match Comparison ===\n";

    VideoFile video3(103, "match_levski_lokomotiv.mp4", "/videos/match103.mp4", 200.0, "720p");
    std::cout << coach.uploadVideo(video3) << "\n";

    auto analysis2 = processor.processVideo(
        video3, 2, "2025-03-22", "Levski", "Lokomotiv"
    );

    if (analysis2) {
        dash.showComparison(*analysis1, *analysis2);
    }

    // ─────────────────────────────────────────
    // Polymorphism demo: MatchEvent types
    // ─────────────────────────────────────────
    std::cout << "\n=== Polymorphism: All MatchEvent types ===\n";
    std::vector<std::unique_ptr<MatchEvent>> demoEvents;
    demoEvents.push_back(std::make_unique<Serve>   (200, 10.5, 3, true,  "jump"));
    demoEvents.push_back(std::make_unique<Attack>  (201, 25.0, 5, false, "4", 95));
    demoEvents.push_back(std::make_unique<Block>   (202, 40.2, 1, true,  2));
    demoEvents.push_back(std::make_unique<Defense> (203, 55.8, 6, true,  "dig"));

    for (const auto& ev : demoEvents)
        std::cout << ev->getSummary() << "\n";

    std::cout << "\n✅  All features demonstrated successfully.\n\n";
    return 0;
}
