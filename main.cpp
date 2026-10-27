#include "main.h"

#include <iostream>
#include <iomanip>
#include "evaluations/reuse/reuse_evaluator.h"

int main() {
    std::cout << "=================================================================\n";
    std::cout << "   GitSkills Database Parser & Near-Match Skill Reuse Analyzer    \n";
    std::cout << "=================================================================\n\n";

    // 1. Specify GitSkills SQLite database file path
    std::string dbPath = "agent_skills_sample.db";

    // Configure analysis options
    AnalysisOptions options;
    options.minSimilarityThreshold = 0.80;  // 80% similarity threshold for near matches
    options.maxSimilarityThreshold = 0.999; // Exclude 100% exact byte matches
    options.observationTimestamp = "2026-07-31T23:59:59Z"; // GitSkills snapshot date

    // 2. Instantiate child class SkillReuseAnalyzer (inherits from GitSkillsDatabaseParser)
    SkillReuseAnalyzer analyzer(dbPath, options);

    if (!analyzer.isOpen()) {
        std::cout << "[Note] SQLite database '" << dbPath << "' not found locally. Exiting program.\n";
        return 1;
    }

    std::cout << "[+] Database Connected Successfully: " << dbPath << "\n";
    std::cout << "[+] Total Artifacts in DB: " << analyzer.getArtifactCount() << "\n\n";

    // 3. Scan multi-skill repositories for near-match re-uses
    std::cout << "Scanning repositories for intra-repository near-match skill re-use...\n";
    std::vector<NearMatchReuseResult> reuseResults = analyzer.analyzeAllRepositories(50);

    if (reuseResults.empty()) {
        std::cout << "No near-match skill re-uses found under current thresholds in the sampled repositories.\n";
        return 0;
    }

    std::cout << "=================================================================\n";
    std::cout << " Found " << reuseResults.size() << " Near-Match Skill Re-use Cases\n";
    std::cout << "=================================================================\n\n";

    for (size_t i = 0; i < reuseResults.size(); ++i) {
        const auto& res = reuseResults[i];

        std::cout << "-----------------------------------------------------------------\n";
        std::cout << "Result #" << (i + 1) << "\n";
        std::cout << "  Repository Full Name : " << res.repoFullName << "\n";
        std::cout << "  Primary Skill Path   : " << res.primaryPath << "\n";
        std::cout << "  Re-use Count (Repo)  : " << res.reuseCount << " times\n";
        std::cout << "  Average Similarity   : " << std::fixed << std::setprecision(2) << (res.averageSimilarity * 100.0) << "%\n";
        std::cout << "  Skill Survival Rate  : " << (res.survivalRate * 100.0) << "%\n";
        std::cout << "  Lifespan             : " << res.lifespanDays << " days\n";
        std::cout << "  First Commit         : " << (res.firstCommitAt.empty() ? "N/A" : res.firstCommitAt) << "\n";
        std::cout << "  Last Commit          : " << (res.lastCommitAt.empty() ? "N/A" : res.lastCommitAt) << "\n";

        std::cout << "  Near-Matched Paths in Repo:\n";
        for (const auto& path : res.nearMatchPaths) {
            std::cout << "    - " << path << "\n";
        }

        std::cout << "\n  Skill String Content (Preview):\n";
        std::cout << "  \"" << res.skillContent.substr(0, 150) << "...\"\n";
        std::cout << "-----------------------------------------------------------------\n\n";
    }
    return 0;
}