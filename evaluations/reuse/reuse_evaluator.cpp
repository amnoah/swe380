#include "reuse_evaluator.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <ctime>
#include <unordered_set>
#include <stdexcept>

#include "dependencies/sqlite3.h"

// =============================================================================
// GitSkillsDatabaseParser Implementation (Base Class)
// =============================================================================

GitSkillsDatabaseParser::GitSkillsDatabaseParser(const std::string& dbPath)
    : dbPath_(dbPath) {
    if (!dbPath_.empty()) {
        int rc = sqlite3_open_v2(dbPath_.c_str(), &db_, SQLITE_OPEN_READONLY, nullptr);
        if (rc != SQLITE_OK) {
            if (db_) {
                sqlite3_close(db_);
                db_ = nullptr;
            }
        }
    }
}

GitSkillsDatabaseParser::~GitSkillsDatabaseParser() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

GitSkillsDatabaseParser::GitSkillsDatabaseParser(GitSkillsDatabaseParser&& other) noexcept
    : db_(other.db_), dbPath_(std::move(other.dbPath_)) {
    other.db_ = nullptr;
}

GitSkillsDatabaseParser& GitSkillsDatabaseParser::operator=(GitSkillsDatabaseParser&& other) noexcept {
    if (this != &other) {
        if (db_) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        dbPath_ = std::move(other.dbPath_);
        other.db_ = nullptr;
    }
    return *this;
}

bool GitSkillsDatabaseParser::isOpen() const {
    return db_ != nullptr;
}

void GitSkillsDatabaseParser::checkDbConnection() const {
    if (!db_) {
        throw std::runtime_error("GitSkillsDatabaseParser: SQLite database connection is not open.");
    }
}

int64_t GitSkillsDatabaseParser::getArtifactCount() const {
    checkDbConnection();
    const char* sql = "SELECT COUNT(*) FROM artifacts;";
    sqlite3_stmt* stmt = nullptr;
    int64_t count = 0;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

std::vector<SkillOccurrence> GitSkillsDatabaseParser::getSkillsByRepository(const std::string& repoFullName) const {
    checkDbConnection();
    std::vector<SkillOccurrence> skills;

    const char* sql = R"(
        SELECT repo_full_name, path, filename, file_sha, content,
               first_commit_at, last_commit_at, commit_count, dedup_primary
        FROM artifacts
        WHERE repo_full_name = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return skills;
    }

    sqlite3_bind_text(stmt, 1, repoFullName.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SkillOccurrence occ;
        const unsigned char* repo = sqlite3_column_text(stmt, 0);
        const unsigned char* p = sqlite3_column_text(stmt, 1);
        const unsigned char* fn = sqlite3_column_text(stmt, 2);
        const unsigned char* sha = sqlite3_column_text(stmt, 3);
        const unsigned char* cnt = sqlite3_column_text(stmt, 4);
        const unsigned char* fc = sqlite3_column_text(stmt, 5);
        const unsigned char* lc = sqlite3_column_text(stmt, 6);

        if (repo) occ.repoFullName = reinterpret_cast<const char*>(repo);
        if (p) occ.path = reinterpret_cast<const char*>(p);
        if (fn) occ.filename = reinterpret_cast<const char*>(fn);
        if (sha) occ.fileSha = reinterpret_cast<const char*>(sha);
        if (cnt) occ.content = reinterpret_cast<const char*>(cnt);
        if (fc) occ.firstCommitAt = reinterpret_cast<const char*>(fc);
        if (lc) occ.lastCommitAt = reinterpret_cast<const char*>(lc);

        occ.commitCount = sqlite3_column_int64(stmt, 7);
        occ.dedupPrimary = (sqlite3_column_int(stmt, 8) == 1);

        skills.push_back(std::move(occ));
    }

    sqlite3_finalize(stmt);
    return skills;
}

std::vector<std::string> GitSkillsDatabaseParser::getMultiSkillRepositories(size_t limit) const {
    checkDbConnection();
    std::vector<std::string> repos;

    std::string sql = "SELECT repo_full_name FROM artifacts GROUP BY repo_full_name HAVING COUNT(*) >= 2 ORDER BY COUNT(*) DESC";
    if (limit > 0) {
        sql += " LIMIT " + std::to_string(limit);
    }
    sql += ";";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* repo = sqlite3_column_text(stmt, 0);
            if (repo) {
                repos.push_back(reinterpret_cast<const char*>(repo));
            }
        }
        sqlite3_finalize(stmt);
    }
    return repos;
}

double GitSkillsDatabaseParser::parseIsoTimestampToEpoch(const std::string& isoStr) {
    if (isoStr.empty()) return 0.0;

    std::tm tm = {};
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;

    int scanned = std::sscanf(isoStr.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec);
    if (scanned < 3) {
        scanned = std::sscanf(isoStr.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);
    }
    if (scanned < 3) {
        return 0.0;
    }

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = 0;

#if defined(_WIN32)
    time_t t = _mkgmtime(&tm);
#else
    time_t t = timegm(&tm);
#endif
    return static_cast<double>(t);
}

// =============================================================================
// SkillReuseAnalyzer Implementation (Derived Child Class)
// =============================================================================

SkillReuseAnalyzer::SkillReuseAnalyzer(const std::string& dbPath, AnalysisOptions options)
    : GitSkillsDatabaseParser(dbPath), options_(options) {}

double SkillReuseAnalyzer::calculateSurvivalRate(const std::string& firstCommit,
                                                  const std::string& lastCommit,
                                                  const std::string& observationCutoff) {
    double tFirst = parseIsoTimestampToEpoch(firstCommit);
    double tLast = parseIsoTimestampToEpoch(lastCommit);
    double tCutoff = parseIsoTimestampToEpoch(observationCutoff);

    if (tCutoff <= 0.0) {
        tCutoff = parseIsoTimestampToEpoch("2026-07-31T23:59:59Z");
    }

    if (tFirst <= 0.0) {
        return 0.50; // Default baseline if commit timestamps were uncollected
    }

    if (tLast < tFirst) {
        tLast = tFirst;
    }

    double activeDurationSec = tLast - tFirst;
    double observationSpanSec = tCutoff - tFirst;

    if (observationSpanSec <= 0.0) {
        return 1.0;
    }

    double survivalRate = activeDurationSec / observationSpanSec;

    // Bounded between 0.0 and 1.0
    if (survivalRate < 0.0) survivalRate = 0.0;
    if (survivalRate > 1.0) survivalRate = 1.0;

    // Small active buffer adjustment for recently created skills
    if (activeDurationSec == 0.0) {
        double daysSinceCreation = observationSpanSec / 86400.0;
        if (daysSinceCreation < 30.0) {
            survivalRate = 1.0 - (daysSinceCreation / 60.0); // High initial survival
        } else {
            survivalRate = 0.05; // Single commit long ago
        }
    }

    return std::round(survivalRate * 10000.0) / 10000.0;
}

std::string SkillReuseAnalyzer::stripFrontmatter(const std::string& text) {
    size_t startPos = text.find("---");
    if (startPos == 0 || (startPos != std::string::npos && startPos < 10 && text.substr(0, startPos) == std::string(startPos, ' '))) {
        size_t endPos = text.find("---", startPos + 3);
        if (endPos != std::string::npos) {
            return text.substr(endPos + 3);
        }
    }
    return text;
}

std::vector<std::string> SkillReuseAnalyzer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string currentToken;

    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            currentToken += std::tolower(static_cast<unsigned char>(c));
        } else if (!currentToken.empty()) {
            if (currentToken.length() >= 2) {
                tokens.push_back(currentToken);
            }
            currentToken.clear();
        }
    }
    if (currentToken.length() >= 2) {
        tokens.push_back(currentToken);
    }
    return tokens;
}

double SkillReuseAnalyzer::jaccardSimilarity(const std::vector<std::string>& tokens1, const std::vector<std::string>& tokens2) {
    if (tokens1.empty() || tokens2.empty()) return 0.0;

    std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
    std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());

    size_t intersectionCount = 0;
    for (const auto& token : set1) {
        if (set2.count(token)) {
            intersectionCount++;
        }
    }

    size_t unionCount = set1.size() + set2.size() - intersectionCount;
    if (unionCount == 0) return 0.0;

    return static_cast<double>(intersectionCount) / static_cast<double>(unionCount);
}

size_t SkillReuseAnalyzer::levenshteinDistance(const std::string& s1, const std::string& s2) {
    size_t m = s1.length();
    size_t n = s2.length();

    if (m == 0) return n;
    if (n == 0) return m;

    std::vector<size_t> dp(n + 1);
    for (size_t j = 0; j <= n; ++j) {
        dp[j] = j;
    }

    for (size_t i = 1; i <= m; ++i) {
        size_t prevDiag = dp[0];
        dp[0] = i;
        for (size_t j = 1; j <= n; ++j) {
            size_t temp = dp[j];
            if (s1[i - 1] == s2[j - 1]) {
                dp[j] = prevDiag;
            } else {
                dp[j] = 1 + std::min({dp[j], dp[j - 1], prevDiag});
            }
            prevDiag = temp;
        }
    }
    return dp[n];
}

double SkillReuseAnalyzer::calculateSimilarity(const std::string& s1, const std::string& s2, bool stripFrontmatterFlag) {
    std::string text1 = stripFrontmatterFlag ? stripFrontmatter(s1) : s1;
    std::string text2 = stripFrontmatterFlag ? stripFrontmatter(s2) : s2;

    if (text1.empty() || text2.empty()) return 0.0;
    if (text1 == text2) return 1.0;

    auto tokens1 = tokenize(text1);
    auto tokens2 = tokenize(text2);

    double jaccard = jaccardSimilarity(tokens1, tokens2);

    // If token similarity is low, return early for efficiency
    if (jaccard < 0.40) {
        return jaccard;
    }

    // For detailed evaluation, combine Jaccard with Normalized Levenshtein
    size_t maxLen = std::max(text1.length(), text2.length());
    if (maxLen == 0) return 1.0;

    // Use a sampled/capped Levenshtein distance for long texts to maintain optimal speed
    if (maxLen > 2000) {
        text1 = text1.substr(0, 2000);
        text2 = text2.substr(0, 2000);
        maxLen = std::max(text1.length(), text2.length());
    }

    size_t levDist = levenshteinDistance(text1, text2);
    double levSim = 1.0 - (static_cast<double>(levDist) / static_cast<double>(maxLen));

    // Weighted combination of token Jaccard and character Levenshtein
    double combinedSim = (0.5 * jaccard) + (0.5 * levSim);
    return std::round(combinedSim * 10000.0) / 10000.0;
}

std::vector<NearMatchReuseResult> SkillReuseAnalyzer::analyzeRepositoryReuse(const std::string& repoFullName) const {
    std::vector<NearMatchReuseResult> results;
    std::vector<SkillOccurrence> skills = getSkillsByRepository(repoFullName);

    if (skills.size() < 2) {
        return results;
    }

    std::vector<bool> assigned(skills.size(), false);

    for (size_t i = 0; i < skills.size(); ++i) {
        if (assigned[i]) continue;
        if (skills[i].content.length() < options_.minContentLength) continue;

        NearMatchReuseResult matchGroup;
        matchGroup.repoFullName = repoFullName;
        matchGroup.primaryPath = skills[i].path;
        matchGroup.skillContent = skills[i].content;

        std::string earliestCommit = skills[i].firstCommitAt;
        std::string latestCommit = skills[i].lastCommitAt;
        int64_t sumCommits = skills[i].commitCount;
        double sumSim = 0.0;

        for (size_t j = i + 1; j < skills.size(); ++j) {
            if (assigned[j]) continue;
            if (skills[j].content.length() < options_.minContentLength) continue;

            // EXCLUDE exact byte matches (same fileSha or 100% byte equality)
            if (!skills[i].fileSha.empty() && skills[i].fileSha == skills[j].fileSha) {
                continue;
            }
            if (skills[i].content == skills[j].content) {
                continue;
            }

            double sim = calculateSimilarity(skills[i].content, skills[j].content, options_.ignoreFrontmatter);

            // Filter for NEAR MATCH (not exact)
            if (sim >= options_.minSimilarityThreshold && sim <= options_.maxSimilarityThreshold) {
                assigned[j] = true;
                matchGroup.nearMatchPaths.push_back(skills[j].path);
                matchGroup.nearMatchShas.push_back(skills[j].fileSha);
                sumSim += sim;

                if (!skills[j].firstCommitAt.empty()) {
                    if (earliestCommit.empty() || skills[j].firstCommitAt < earliestCommit) {
                        earliestCommit = skills[j].firstCommitAt;
                    }
                }
                if (!skills[j].lastCommitAt.empty()) {
                    if (latestCommit.empty() || skills[j].lastCommitAt > latestCommit) {
                        latestCommit = skills[j].lastCommitAt;
                    }
                }
                sumCommits += skills[j].commitCount;
            }
        }

        // If near-matches were found within this single repository
        if (!matchGroup.nearMatchPaths.empty()) {
            assigned[i] = true;
            matchGroup.reuseCount = matchGroup.nearMatchPaths.size();
            matchGroup.averageSimilarity = sumSim / matchGroup.reuseCount;
            matchGroup.firstCommitAt = earliestCommit;
            matchGroup.lastCommitAt = latestCommit;
            matchGroup.totalCommits = sumCommits;

            // Calculate lifespan in days
            double t1 = parseIsoTimestampToEpoch(earliestCommit);
            double t2 = parseIsoTimestampToEpoch(latestCommit);
            if (t1 > 0.0 && t2 >= t1) {
                matchGroup.lifespanDays = (t2 - t1) / 86400.0;
            } else {
                matchGroup.lifespanDays = 0.0;
            }

            // Calculate survival rate
            matchGroup.survivalRate = calculateSurvivalRate(earliestCommit, latestCommit, options_.observationTimestamp);

            results.push_back(std::move(matchGroup));
        }
    }

    return results;
}

std::vector<NearMatchReuseResult> SkillReuseAnalyzer::analyzeAllRepositories(size_t maxRepos) const {
    std::vector<NearMatchReuseResult> allResults;
    std::vector<std::string> multiSkillRepos = getMultiSkillRepositories(maxRepos);

    for (const auto& repo : multiSkillRepos) {
        std::cout <<  "1/2\n";
        auto repoResults = analyzeRepositoryReuse(repo);
        std::cout << repoResults.size() << "\n";
        allResults.insert(allResults.end(),
                          std::make_move_iterator(repoResults.begin()),
                          std::make_move_iterator(repoResults.end()));
    }

    return allResults;
}