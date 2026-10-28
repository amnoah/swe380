#include "reuse_evaluator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {

std::string columnText(sqlite3_stmt* stmt, int column) {
    const unsigned char* value = sqlite3_column_text(stmt, column);
    return value ? reinterpret_cast<const char*>(value) : std::string();
}

// Howard Hinnant's civil-date conversion: days since 1970-01-01.
// This avoids local-time and platform-specific timegm behavior.
int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= (m <= 2);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

bool parseDigits(const std::string& s, std::size_t pos, std::size_t n, int& out) {
    if (pos + n > s.size()) return false;
    out = 0;
    for (std::size_t i = pos; i < pos + n; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        out = out * 10 + (s[i] - '0');
    }
    return true;
}

std::string minNonEmpty(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    return a < b ? a : b;
}

std::string maxNonEmpty(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    return a > b ? a : b;
}

} // namespace

// =============================================================================
// GitSkillsDatabaseParser
// =============================================================================

GitSkillsDatabaseParser::GitSkillsDatabaseParser(const std::string& dbPath)
    : dbPath_(dbPath) {
    if (!dbPath_.empty()) {
        const int rc = sqlite3_open_v2(dbPath_.c_str(), &db_, SQLITE_OPEN_READONLY, nullptr);
        if (rc != SQLITE_OK) {
            if (db_) sqlite3_close(db_);
            db_ = nullptr;
        }
    }
}

GitSkillsDatabaseParser::~GitSkillsDatabaseParser() {
    if (db_) sqlite3_close(db_);
}

GitSkillsDatabaseParser::GitSkillsDatabaseParser(GitSkillsDatabaseParser&& other) noexcept
    : db_(other.db_), dbPath_(std::move(other.dbPath_)) {
    other.db_ = nullptr;
}

GitSkillsDatabaseParser& GitSkillsDatabaseParser::operator=(GitSkillsDatabaseParser&& other) noexcept {
    if (this != &other) {
        if (db_) sqlite3_close(db_);
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
    if (!db_) throw std::runtime_error("SQLite database connection is not open.");
}

int64_t GitSkillsDatabaseParser::getArtifactCount() const {
    checkDbConnection();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM artifacts;", -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

std::vector<SkillOccurrence>
GitSkillsDatabaseParser::getSkillsByRepository(const std::string& repoFullName) const {
    checkDbConnection();
    std::vector<SkillOccurrence> skills;

    // Do not filter on dedup_primary: that would remove the identical copies
    // this analysis is explicitly intended to detect.
    const char* sql = R"SQL(
        SELECT repo_full_name, path, filename, file_sha, content,
               first_commit_at, last_commit_at, commit_count, dedup_primary
        FROM artifacts
        WHERE repo_full_name = ?
        ORDER BY path;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    sqlite3_bind_text(stmt, 1, repoFullName.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SkillOccurrence s;
        s.repoFullName = columnText(stmt, 0);
        s.path = columnText(stmt, 1);
        s.filename = columnText(stmt, 2);
        s.fileSha = columnText(stmt, 3);
        s.content = columnText(stmt, 4);
        s.firstCommitAt = columnText(stmt, 5);
        s.lastCommitAt = columnText(stmt, 6);
        s.commitCount = sqlite3_column_int64(stmt, 7);
        s.dedupPrimary = sqlite3_column_int(stmt, 8) == 1;
        skills.push_back(std::move(s));
    }
    const int rc = sqlite3_errcode(db_);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_OK && rc != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db_));
    return skills;
}

std::vector<std::string>
GitSkillsDatabaseParser::getMultiSkillRepositories(std::size_t limit) const {
    checkDbConnection();
    std::vector<std::string> repos;
    std::string sql =
        "SELECT repo_full_name FROM artifacts "
        "GROUP BY repo_full_name HAVING COUNT(*) >= 2 "
        "ORDER BY COUNT(*) DESC, repo_full_name ASC";
    if (limit > 0) sql += " LIMIT " + std::to_string(limit);
    sql += ";";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db_));

    while (sqlite3_step(stmt) == SQLITE_ROW) repos.push_back(columnText(stmt, 0));
    sqlite3_finalize(stmt);
    return repos;
}

double GitSkillsDatabaseParser::parseIsoTimestampToEpoch(const std::string& s) {
    // Accept YYYY-MM-DD, YYYY-MM-DDTHH:MM:SS[.fraction][Z|+HH:MM|-HH:MM],
    // and the common SQL form YYYY-MM-DD HH:MM:SS.
    int y, mon, day;
    if (s.size() < 10 || !parseDigits(s, 0, 4, y) ||
        !parseDigits(s, 5, 2, mon) || !parseDigits(s, 8, 2, day) ||
        s[4] != '-' || s[7] != '-') return 0.0;
    if (mon < 1 || mon > 12 || day < 1 || day > 31) return 0.0;

    int hh = 0, mm = 0, ss = 0;
    if (s.size() >= 19) {
        if (!parseDigits(s, 11, 2, hh) || !parseDigits(s, 14, 2, mm) ||
            !parseDigits(s, 17, 2, ss)) return 0.0;
    }
    if (hh > 23 || mm > 59 || ss > 60) return 0.0;

    int offsetSeconds = 0;
    std::size_t tz = 19;
    while (tz < s.size() && s[tz] != 'Z' && s[tz] != '+' && s[tz] != '-') ++tz;
    if (tz < s.size() && (s[tz] == '+' || s[tz] == '-')) {
        const int sign = s[tz] == '+' ? 1 : -1;
        int oh = 0, om = 0;
        if (tz + 6 <= s.size() && s[tz + 3] == ':' &&
            parseDigits(s, tz + 1, 2, oh) && parseDigits(s, tz + 4, 2, om) &&
            oh <= 23 && om <= 59) {
            offsetSeconds = sign * (oh * 3600 + om * 60);
        } else {
            return 0.0;
        }
    }

    const int64_t days = daysFromCivil(y, static_cast<unsigned>(mon), static_cast<unsigned>(day));
    return static_cast<double>(days * 86400 + hh * 3600 + mm * 60 + ss - offsetSeconds);
}

// =============================================================================
// SkillReuseAnalyzer
// =============================================================================

SkillReuseAnalyzer::SkillReuseAnalyzer(const std::string& dbPath, AnalysisOptions options)
    : GitSkillsDatabaseParser(dbPath), options_(std::move(options)) {
    if (options_.minSimilarityThreshold < 0.0 || options_.minSimilarityThreshold > 1.0)
        throw std::invalid_argument("minSimilarityThreshold must be between 0 and 1.");
    if (options_.maxSimilarityThreshold < 0.0 || options_.maxSimilarityThreshold > 1.0)
        throw std::invalid_argument("maxSimilarityThreshold must be between 0 and 1.");
    if (options_.minSimilarityThreshold > options_.maxSimilarityThreshold)
        throw std::invalid_argument("Minimum similarity threshold exceeds maximum.");
    if (!std::isfinite(options_.recencyHalfLifeDays) || options_.recencyHalfLifeDays <= 0.0)
        throw std::invalid_argument("recencyHalfLifeDays must be greater than zero.");
}

std::string SkillReuseAnalyzer::stripFrontmatter(const std::string& text) {
    if (text.rfind("---", 0) != 0) return text;
    std::size_t lineEnd = text.find('\n');
    if (lineEnd == std::string::npos) return text;
    std::size_t close = text.find("\n---", lineEnd + 1);
    if (close == std::string::npos) return text;
    std::size_t after = text.find('\n', close + 4);
    return after == std::string::npos ? std::string() : text.substr(after + 1);
}

std::string SkillReuseAnalyzer::normalizeForComparison(const std::string& input, bool stripFm) {
    std::string text = stripFm ? stripFrontmatter(input) : input;
    std::string out;
    out.reserve(text.size());
    bool previousSpace = false;
    for (unsigned char c : text) {
        if (std::isspace(c)) {
            if (!previousSpace) out.push_back(' ');
            previousSpace = true;
        } else {
            out.push_back(static_cast<char>(std::tolower(c)));
            previousSpace = false;
        }
    }
    return out;
}

std::vector<std::string> SkillReuseAnalyzer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string token;
    for (unsigned char c : text) {
        if (std::isalnum(c)) {
            token.push_back(static_cast<char>(std::tolower(c)));
        } else if (!token.empty()) {
            tokens.push_back(std::move(token));
            token.clear();
        }
    }
    if (!token.empty()) tokens.push_back(std::move(token));
    return tokens;
}

double SkillReuseAnalyzer::jaccardSimilarity(const std::vector<std::string>& a,
                                             const std::vector<std::string>& b) {
    if (a.empty() || b.empty()) return 0.0;
    std::unordered_set<std::string> sa(a.begin(), a.end()), sb(b.begin(), b.end());
    std::size_t intersection = 0;
    for (const auto& token : sa) if (sb.count(token)) ++intersection;
    const std::size_t unionSize = sa.size() + sb.size() - intersection;
    return unionSize ? static_cast<double>(intersection) / unionSize : 0.0;
}

double SkillReuseAnalyzer::calculateSimilarity(const std::string& a,
                                               const std::string& b,
                                               bool stripFm) {
    const std::string na = normalizeForComparison(a, stripFm);
    const std::string nb = normalizeForComparison(b, stripFm);
    if (na.empty() || nb.empty()) return 0.0;
    if (na == nb) return 1.0;

    const auto ta = tokenize(na);
    const auto tb = tokenize(nb);
    const double tokenScore = jaccardSimilarity(ta, tb);

    // Character 5-gram Jaccard captures local phrase/order similarity without
    // the quadratic cost and arbitrary 2,000-character truncation of Levenshtein.
    auto grams = [](const std::string& s) {
        std::unordered_set<std::string> result;
        constexpr std::size_t N = 5;
        if (s.size() < N) {
            if (!s.empty()) result.insert(s);
            return result;
        }
        for (std::size_t i = 0; i + N <= s.size(); ++i)
            result.insert(s.substr(i, N));
        return result;
    };
    const auto ga = grams(na);
    const auto gb = grams(nb);
    std::size_t intersection = 0;
    for (const auto& g : ga) if (gb.count(g)) ++intersection;
    const std::size_t unionSize = ga.size() + gb.size() - intersection;
    const double charScore = unionSize ? static_cast<double>(intersection) / unionSize : 0.0;

    return std::round((0.5 * tokenScore + 0.5 * charScore) * 10000.0) / 10000.0;
}

double SkillReuseAnalyzer::calculateSurvivalRate(const std::string& firstCommit,
                                                 const std::string& lastCommit,
                                                 const std::string& cutoff) {
    // Compatibility overload: existing callers get the same scoring model
    // with the default 90-day recency half-life and one assumed commit.
    return calculatePersistenceScore(firstCommit, lastCommit, cutoff, 1, 90.0);
}

double SkillReuseAnalyzer::calculatePersistenceScore(const std::string& firstCommit,
                                                     const std::string& lastCommit,
                                                     const std::string& cutoff,
                                                     int64_t commitCount,
                                                     double recencyHalfLifeDays) {
    const double first = parseIsoTimestampToEpoch(firstCommit);
    const double lastRaw = parseIsoTimestampToEpoch(lastCommit);
    const double end = parseIsoTimestampToEpoch(cutoff);

    // Invalid or contradictory timestamps should not produce an optimistic score.
    if (first <= 0.0 || lastRaw <= 0.0 || end <= 0.0 || lastRaw < first || end < first)
        return 0.0;

    const double last = std::min(lastRaw, end);
    const double observationDays = (end - first) / 86400.0;
    const double activeSpanDays = std::max(0.0, (last - first) / 86400.0);

    // Temporal coverage: how much of the observed lifespan has evidence of
    // continued commits. This is a proxy, not a true Kaplan-Meier estimate.
    const double coverage = observationDays <= 0.0
        ? 1.0
        : std::clamp(activeSpanDays / observationDays, 0.0, 1.0);

    // Recency: exponential decay with a configurable half-life. Unlike a hard
    // 30-day cutoff, this changes smoothly as the last commit gets older.
    const double daysSinceLast = std::max(0.0, (end - last) / 86400.0);
    const double halfLife = recencyHalfLifeDays;
    const double recency = std::exp(-std::log(2.0) * daysSinceLast / halfLife);

    // Commit activity is deliberately a saturating signal, not a linear reward
    // that would let very large repositories dominate the score.
    const double safeCommitCount = static_cast<double>(std::max<int64_t>(0, commitCount));
    const double activity = 1.0 - std::exp(-safeCommitCount / 3.0);

    return std::clamp(0.50 * coverage + 0.30 * recency + 0.20 * activity, 0.0, 1.0);
}

std::vector<NearMatchReuseResult>
SkillReuseAnalyzer::analyzeRepositoryReuse(const std::string& repoFullName) const {
    const auto skills = getSkillsByRepository(repoFullName);
    const std::size_t n = skills.size();
    if (n < 2) return {};

    // Build an undirected graph. Exact copies are edges too, with score 1.0.
    // Connected components avoid the previous order-dependent greedy grouping.
    std::vector<std::vector<std::pair<std::size_t, double>>> graph(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (skills[i].content.size() < options_.minContentLength) continue;
        for (std::size_t j = i + 1; j < n; ++j) {
            if (skills[j].content.size() < options_.minContentLength) continue;

            const bool sameBytes = skills[i].content == skills[j].content;
            const bool sameSha = !skills[i].fileSha.empty() &&
                                 skills[i].fileSha == skills[j].fileSha;
            if (sameBytes || sameSha) {
                graph[i].push_back({j, 1.0});
                graph[j].push_back({i, 1.0});
                continue;
            }

            const double sim = calculateSimilarity(
                skills[i].content, skills[j].content, options_.ignoreFrontmatter);

            // Similarity threshold is inclusive. Exact copies have already
            // been retained above, irrespective of maxSimilarityThreshold.
            if (sim >= options_.minSimilarityThreshold &&
                sim <= options_.maxSimilarityThreshold) {
                graph[i].push_back({j, sim});
                graph[j].push_back({i, sim});
            }
        }
    }

    std::vector<bool> visited(n, false);
    std::vector<NearMatchReuseResult> results;

    for (std::size_t root = 0; root < n; ++root) {
        if (visited[root] || graph[root].empty()) continue;

        std::vector<std::size_t> component;
        std::queue<std::size_t> q;
        q.push(root);
        visited[root] = true;
        while (!q.empty()) {
            const auto u = q.front();
            q.pop();
            component.push_back(u);
            for (const auto& edge : graph[u]) {
                if (!visited[edge.first]) {
                    visited[edge.first] = true;
                    q.push(edge.first);
                }
            }
        }
        if (component.size() < 2) continue;

        // Stable representative: lexicographically first path.
        std::sort(component.begin(), component.end(), [&](std::size_t a, std::size_t b) {
            return skills[a].path < skills[b].path;
        });

        NearMatchReuseResult r;
        r.repoFullName = repoFullName;
        r.primaryPath = skills[component.front()].path;
        r.skillContent = skills[component.front()].content;

        double scoreSum = 0.0;
        std::size_t edgeCount = 0;
        for (const auto idx : component) {
            r.firstCommitAt = minNonEmpty(r.firstCommitAt, skills[idx].firstCommitAt);
            r.lastCommitAt = maxNonEmpty(r.lastCommitAt, skills[idx].lastCommitAt);
            r.totalCommits += skills[idx].commitCount;
        }

        // List all other component members, and classify each against the
        // representative. Pairwise graph edges remain the actual grouping basis.
        for (std::size_t k = 1; k < component.size(); ++k) {
            const auto idx = component[k];
            const auto primary = component.front();
            const bool exact = skills[primary].content == skills[idx].content ||
                (!skills[primary].fileSha.empty() &&
                 skills[primary].fileSha == skills[idx].fileSha);

            if (exact) {
                r.identicalPaths.push_back(skills[idx].path);
                r.identicalShas.push_back(skills[idx].fileSha);
                ++r.identicalCopyCount;
            } else {
                r.nearMatchPaths.push_back(skills[idx].path);
                r.nearMatchShas.push_back(skills[idx].fileSha);
                ++r.nearMatchCount;
            }
        }

        // Average score over every actual edge in the component, not just
        // representative-to-member comparisons.
        for (const auto u : component) {
            for (const auto& edge : graph[u]) {
                if (u < edge.first) {
                    scoreSum += edge.second;
                    ++edgeCount;
                }
            }
        }
        r.averageSimilarity = edgeCount ? scoreSum / edgeCount : 0.0;
        r.reuseCount = r.identicalCopyCount + r.nearMatchCount;

        const double t1 = parseIsoTimestampToEpoch(r.firstCommitAt);
        const double t2 = parseIsoTimestampToEpoch(r.lastCommitAt);
        if (t1 > 0.0 && t2 >= t1) r.lifespanDays = (t2 - t1) / 86400.0;
        if (!options_.observationTimestamp.empty()) {
            r.survivalRate = calculatePersistenceScore(
                r.firstCommitAt, r.lastCommitAt, options_.observationTimestamp,
                r.totalCommits, options_.recencyHalfLifeDays);
            r.persistenceScore = r.survivalRate;
            const double firstEpoch = parseIsoTimestampToEpoch(r.firstCommitAt);
            const double lastEpoch = parseIsoTimestampToEpoch(r.lastCommitAt);
            const double cutoffEpoch = parseIsoTimestampToEpoch(options_.observationTimestamp);
            if (firstEpoch > 0.0 && lastEpoch >= firstEpoch && cutoffEpoch >= firstEpoch) {
                const double observed = cutoffEpoch - firstEpoch;
                r.temporalCoverage = observed <= 0.0 ? 1.0
                    : std::clamp((std::min(lastEpoch, cutoffEpoch) - firstEpoch) / observed,
                                 0.0, 1.0);
                const double daysSinceLast = std::max(0.0, (cutoffEpoch -
                    std::min(lastEpoch, cutoffEpoch)) / 86400.0);
                r.recencyScore = std::exp(-std::log(2.0) * daysSinceLast /
                                          options_.recencyHalfLifeDays);
                r.commitActivityScore = 1.0 - std::exp(-
                    static_cast<double>(std::max<int64_t>(0, r.totalCommits)) / 3.0);
            }
        }
        results.push_back(std::move(r));
    }

    return results;
}

std::vector<NearMatchReuseResult>
SkillReuseAnalyzer::analyzeAllRepositories(std::size_t maxRepos) const {
    std::vector<NearMatchReuseResult> all;
    const auto repos = getMultiSkillRepositories(maxRepos);
    std::size_t completed = 0;
    for (const auto& repo : repos) {
        ++completed;
        std::cout << "Analyzing repository " << completed << "/" << repos.size()
                  << ": " << repo << '\n';
        auto repoResults = analyzeRepositoryReuse(repo);
        all.insert(all.end(),
                   std::make_move_iterator(repoResults.begin()),
                   std::make_move_iterator(repoResults.end()));
    }
    return all;
}

