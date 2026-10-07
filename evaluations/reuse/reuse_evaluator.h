#ifndef REUSE_EVALUATOR_HPP
#define REUSE_EVALUATOR_HPP

#include <string>
#include <vector>
#include <memory>

#include "dependencies/sqlite3.h"

/**
 * @struct SkillOccurrence
 * @brief Represents an individual skill file instance mined from the GitSkills database artifacts table.
 */
struct SkillOccurrence {
    std::string repoFullName;   ///< Repository full name (e.g., "owner/repo")
    std::string path;           ///< File path of the SKILL.md file inside the repo
    std::string filename;       ///< Exact file basename
    std::string fileSha;        ///< Git blob hash (file_sha)
    std::string content;        ///< Raw Markdown text content
    std::string firstCommitAt;  ///< ISO-8601 UTC timestamp of first commit
    std::string lastCommitAt;   ///< ISO-8601 UTC timestamp of last commit
    int64_t commitCount = 0;    ///< Number of commits for this file
    bool dedupPrimary = false;  ///< Primary representative flag
};

/**
 * @struct NearMatchReuseResult
 * @brief Results for a skill re-used as a near match (not exact) within a single repository.
 */
struct NearMatchReuseResult {
    std::string repoFullName;                ///< Target repository name
    std::string primaryPath;                 ///< File path of the base/representative skill
    std::string skillContent;                ///< The skill string content itself
    size_t reuseCount = 0;                   ///< Number of near-match re-uses in this single repo
    double averageSimilarity = 0.0;           ///< Average similarity score [0.0, 1.0]
    std::vector<std::string> nearMatchPaths; ///< Paths of all near-matched copies in the repo
    std::vector<std::string> nearMatchShas;  ///< Git SHAs of near-matched copies

    // Temporal analytics and survival rate
    double survivalRate = 0.0;               ///< Skill survival rate [0.0, 1.0]
    double lifespanDays = 0.0;               ///< Active lifespan in days (last_commit - first_commit)
    std::string firstCommitAt;               ///< Earliest commit timestamp in cluster
    std::string lastCommitAt;                ///< Latest commit timestamp in cluster
    int64_t totalCommits = 0;                ///< Combined commit count across instances
};

/**
 * @struct AnalysisOptions
 * @brief Configuration thresholds for near-match detection and survival rate calculation.
 */
struct AnalysisOptions {
    double minSimilarityThreshold = 0.80;  ///< Minimum similarity ratio for near match (80%)
    double maxSimilarityThreshold = 0.999; ///< Upper threshold (<100% to exclude exact byte duplicates)
    size_t minContentLength = 40;          ///< Minimum text length to filter out trivial files
    std::string observationTimestamp = "2026-07-31T23:59:59Z"; ///< July 2026 GitSkills cutoff
    bool ignoreFrontmatter = true;         ///< Strip YAML frontmatter before comparison
};

/**
 * @class GitSkillsDatabaseParser
 * @brief Base class for opening and parsing GitSkills SQLite database releases.
 *
 * Manages SQLite connection lifecycle, statement execution, and table querying
 * adhering to modern C++17 RAII principles.
 */
class GitSkillsDatabaseParser {
public:
    /**
     * @brief Constructs parser and connects to SQLite database.
     * @param dbPath Path to agent_skills_release.db or agent_skills_sample.db
     */
    explicit GitSkillsDatabaseParser(const std::string& dbPath);

    /**
     * @brief Virtual destructor for polymorphic cleanup.
     */
    virtual ~GitSkillsDatabaseParser();

    // Prevent copying for RAII database handle safety
    GitSkillsDatabaseParser(const GitSkillsDatabaseParser&) = delete;
    GitSkillsDatabaseParser& operator=(const GitSkillsDatabaseParser&) = delete;

    // Support move operations
    GitSkillsDatabaseParser(GitSkillsDatabaseParser&&) noexcept;
    GitSkillsDatabaseParser& operator=(GitSkillsDatabaseParser&&) noexcept;

    /**
     * @brief Checks if the database file is connected and open.
     */
    bool isOpen() const;

    /**
     * @brief Returns total row count of the artifacts table.
     */
    int64_t getArtifactCount() const;

    /**
     * @brief Retrieves all skill file occurrences for a specific repository.
     * @param repoFullName Full repository identifier (e.g. "owner/repo")
     * @return Vector of SkillOccurrence structs
     */
    std::vector<SkillOccurrence> getSkillsByRepository(const std::string& repoFullName) const;

    /**
     * @brief Gets repository names that contain multiple (>=2) skill files.
     * @param limit Maximum repositories to return (0 = no limit)
     */
    std::vector<std::string> getMultiSkillRepositories(size_t limit = 0) const;

    /**
     * @brief Helper utility parsing ISO-8601 UTC timestamp string to Unix epoch seconds.
     * @param isoStr Timestamp string (e.g. "2026-07-22T17:07:58Z")
     * @return Seconds since Unix epoch (0.0 if invalid)
     */
    static double parseIsoTimestampToEpoch(const std::string& isoStr);

protected:
    sqlite3* db_ = nullptr;
    std::string dbPath_;

    void checkDbConnection() const;
};

/**
 * @class SkillReuseAnalyzer
 * @brief Child class extending GitSkillsDatabaseParser to analyze intra-repository near-match skill re-use.
 *
 * Inherits database access capabilities from GitSkillsDatabaseParser, implements Jaccard token and
 * Levenshtein string similarity algorithms, detects near-match skill re-use within single repositories,
 * calculates re-use counts, provides the skill text string, and evaluates skill survival rates.
 */
class SkillReuseAnalyzer : public GitSkillsDatabaseParser {
public:
    /**
     * @brief Constructor initializing DB connection and analysis options.
     * @param dbPath Path to SQLite database
     * @param options Analysis thresholds and parameters
     */
    explicit SkillReuseAnalyzer(const std::string& dbPath, AnalysisOptions options = AnalysisOptions());

    /**
     * @brief Destructor.
     */
    ~SkillReuseAnalyzer() override = default;

    /**
     * @brief Analyzes near-match skill re-use within a single repository.
     * @param repoFullName Target repository name (e.g., "owner/repo")
     * @return Vector of NearMatchReuseResult structures for near-matched skills
     */
    std::vector<NearMatchReuseResult> analyzeRepositoryReuse(const std::string& repoFullName) const;

    /**
     * @brief Scans multiple repositories to find near-match skill re-uses.
     * @param maxRepos Limit on number of multi-skill repos to inspect (0 = all)
     * @return Vector of NearMatchReuseResult structures across inspected repos
     */
    std::vector<NearMatchReuseResult> analyzeAllRepositories(size_t maxRepos = 0) const;

    /**
     * @brief Static method to calculate text similarity ratio between two skill strings.
     * @param s1 First skill text
     * @param s2 Second skill text
     * @param stripFrontmatter Whether to remove YAML frontmatter prior to comparison
     * @return Similarity score between 0.0 (different) and 1.0 (identical)
     */
    static double calculateSimilarity(const std::string& s1, const std::string& s2, bool stripFrontmatter = true);

    /**
     * @brief Calculates the skill survival rate based on first commit, last commit, and observation horizon.
     * @param firstCommit Earliest commit ISO-8601 timestamp
     * @param lastCommit Latest commit ISO-8601 timestamp
     * @param observationCutoff Snapshot observation cutoff timestamp
     * @return Bounded survival rate in range [0.0, 1.0]
     */
    static double calculateSurvivalRate(const std::string& firstCommit,
                                         const std::string& lastCommit,
                                         const std::string& observationCutoff);

    void setOptions(const AnalysisOptions& options) { options_ = options; }
    const AnalysisOptions& getOptions() const { return options_; }

private:
    AnalysisOptions options_;

    static std::string stripFrontmatter(const std::string& text);
    static std::vector<std::string> tokenize(const std::string& text);
    static double jaccardSimilarity(const std::vector<std::string>& tokens1, const std::vector<std::string>& tokens2);
    static size_t levenshteinDistance(const std::string& s1, const std::string& s2);
};

#endif // REUSE_EVALUATOR_HPP