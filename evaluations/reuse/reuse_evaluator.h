#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "dependencies/sqlite3.h"

// One row from the GitSkills artifacts table. Duplicate rows are intentionally
// retained because identical copies are part of the reuse analysis.
struct SkillOccurrence {
    std::string repoFullName;
    std::string path;
    std::string filename;
    std::string fileSha;
    std::string content;
    std::string firstCommitAt;
    std::string lastCommitAt;
    int64_t commitCount = 0;
    bool dedupPrimary = false;
};

struct AnalysisOptions {
    double minSimilarityThreshold = 0.80;
    double maxSimilarityThreshold = 0.999;
    std::size_t minContentLength = 1;
    bool ignoreFrontmatter = true;
    std::string observationTimestamp;

    // Half-life for exponential recency decay. At this many days after the
    // last commit, the recency component is 0.5.
    double recencyHalfLifeDays = 90.0;
};

struct NearMatchReuseResult {
    std::string repoFullName;
    std::string primaryPath;
    std::string skillContent;

    // Every other member of this connected similarity group is listed in
    // exactly one of these vectors. Exact copies are deliberately included.
    std::vector<std::string> identicalPaths;
    std::vector<std::string> nearMatchPaths;
    std::vector<std::string> identicalShas;
    std::vector<std::string> nearMatchShas;

    std::size_t identicalCopyCount = 0;
    std::size_t nearMatchCount = 0;
    std::size_t reuseCount = 0;
    double averageSimilarity = 0.0;
    double lifespanDays = 0.0;

    // Persistence metrics are in [0, 1]. survivalRate is retained for API
    // compatibility and equals persistenceScore when a cutoff is supplied.
    // These are heuristic commit-history proxies, not proof of actual usage.
    double survivalRate = 0.0;
    double persistenceScore = 0.0;
    double temporalCoverage = 0.0;
    double recencyScore = 0.0;
    double commitActivityScore = 0.0;

    int64_t totalCommits = 0;
    std::string firstCommitAt;
    std::string lastCommitAt;
};

class GitSkillsDatabaseParser {
public:
    explicit GitSkillsDatabaseParser(const std::string& dbPath);
    virtual ~GitSkillsDatabaseParser();

    GitSkillsDatabaseParser(const GitSkillsDatabaseParser&) = delete;
    GitSkillsDatabaseParser& operator=(const GitSkillsDatabaseParser&) = delete;
    GitSkillsDatabaseParser(GitSkillsDatabaseParser&& other) noexcept;
    GitSkillsDatabaseParser& operator=(GitSkillsDatabaseParser&& other) noexcept;

    bool isOpen() const;
    int64_t getArtifactCount() const;
    std::vector<SkillOccurrence> getSkillsByRepository(const std::string& repoFullName) const;
    std::vector<std::string> getMultiSkillRepositories(std::size_t limit) const;

protected:
    void checkDbConnection() const;
    static double parseIsoTimestampToEpoch(const std::string& isoStr);

    sqlite3* db_ = nullptr;
    std::string dbPath_;
};

class SkillReuseAnalyzer : public GitSkillsDatabaseParser {
public:
    SkillReuseAnalyzer(const std::string& dbPath, AnalysisOptions options = {});

    std::vector<NearMatchReuseResult> analyzeRepositoryReuse(const std::string& repoFullName) const;
    std::vector<NearMatchReuseResult> analyzeAllRepositories(std::size_t maxRepos = 0) const;

    static double calculateSimilarity(const std::string& a,
                                      const std::string& b,
                                      bool stripFrontmatterFlag = true);
    static double calculateSurvivalRate(const std::string& firstCommit,
                                        const std::string& lastCommit,
                                        const std::string& observationCutoff);

    // Full persistence score when the artifact's aggregate commit count is known.
    static double calculatePersistenceScore(const std::string& firstCommit,
                                            const std::string& lastCommit,
                                            const std::string& observationCutoff,
                                            int64_t commitCount,
                                            double recencyHalfLifeDays = 90.0);

private:
    AnalysisOptions options_;

    static std::string stripFrontmatter(const std::string& text);
    static std::string normalizeForComparison(const std::string& text, bool stripFm);
    static std::vector<std::string> tokenize(const std::string& text);
    static double jaccardSimilarity(const std::vector<std::string>& a,
                                    const std::vector<std::string>& b);
};
