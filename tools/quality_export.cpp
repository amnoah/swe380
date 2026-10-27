#include "dependencies/sqlite3.h"
#include "evaluations/examples/example_evaluator.h"
#include "evaluations/length/length_evaluator.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::string readText(sqlite3_stmt* statement, int index) {
    const auto* txt = sqlite3_column_text(statement, index);
    return txt ? std::string(reinterpret_cast<const char*>(txt)) : "";
}
void csvCell(std::ostream& out, const std::string& cell) {
    out << '"';
    for (char c : cell) {
        if (c == '"') out << '"';
        out << c;
    }
    out << '"';
}
void csvCell(std::ostream& out, std::int64_t value) { out << value; }
void csvCell(std::ostream& out, size_t value) { out << value; }
void csvCell(std::ostream& out, double value) { out << value; }
void csvNullableInt(std::ostream& out, sqlite3_stmt* statement, int index) {
    if (sqlite3_column_type(statement, index) != SQLITE_NULL)
        out << sqlite3_column_int64(statement, index);
}
void csvNullableText(std::ostream& out, sqlite3_stmt* statement, int index) {
    if (sqlite3_column_type(statement, index) != SQLITE_NULL)
        csvCell(out, readText(statement, index));
}
}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: quality_export <path-to-gitskills.db> <output.csv>\n";
        return 2;
    }
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;
    try {
        if (sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
            throw std::runtime_error("Could not open input database read-only");
        const char* query = R"SQL(
            WITH reuse AS (
                SELECT file_sha,
                       COUNT(*) AS file_occurrences,
                       COUNT(DISTINCT repo_full_name) AS repository_count,
                       COUNT(DISTINCT lower(substr(repo_full_name,1,instr(repo_full_name,'/')-1))) AS owner_count
                FROM artifacts
                WHERE file_sha IS NOT NULL
                GROUP BY file_sha
            )
            SELECT a.file_sha, a.repo_full_name, a.path, a.content,
                   reuse.file_occurrences, reuse.repository_count, reuse.owner_count,
                   a.history_fetched, a.commit_count, a.first_commit_at,
                   a.last_commit_at, a.frontmatter_valid, a.has_scripts,
                   a.has_references, r.stars, r.created_at, a.body_chars
            FROM artifacts AS a
            JOIN reuse ON reuse.file_sha = a.file_sha
            LEFT JOIN repos AS r ON r.full_name = a.repo_full_name
            WHERE a.dedup_primary = 1 AND a.content IS NOT NULL
            ORDER BY a.file_sha
        )SQL";
        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(std::string("Invalid query: ") + sqlite3_errmsg(db));
        std::ofstream out(argv[2], std::ios::binary);
        if (!out) throw std::runtime_error("Could not create output CSV");
        out << "file_sha,representative_repo,representative_path,"
            << "word_count,prose_word_count,code_word_count,character_count,byte_count,line_count,prose_line_count,code_line_count,total_file_line_count,over_500_lines,"
            << "has_examples,example_count,example_code_blocks,example_sections,example_density,"
            << "file_occurrences,repository_count,additional_repositories,distinct_owner_count,"
            << "history_fetched,commit_count,subsequent_commits,first_commit_at,last_commit_at,"
            << "frontmatter_valid,has_scripts,has_references,repo_stars,repo_created_at,dataset_body_chars,body_chars_difference,body_chars_match\n";
        size_t records = 0;
        int rc = SQLITE_OK;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const std::string content = readText(stmt, 3);
            const auto length = SkillLengthEvaluator::evaluate(content);
            const auto examples = SkillExampleEvaluator::evaluate(content, length.wordCount);
            csvCell(out, readText(stmt, 0)); out << ',';
            csvCell(out, readText(stmt, 1)); out << ',';
            csvCell(out, readText(stmt, 2)); out << ',';
            csvCell(out, length.wordCount); out << ',';
            csvCell(out, length.proseWordCount); out << ',';
            csvCell(out, length.codeWordCount); out << ',';
            csvCell(out, length.characterCount); out << ',';
            csvCell(out, length.byteCount); out << ',';
            csvCell(out, length.lineCount); out << ',';
            csvCell(out, length.proseLineCount); out << ',';
            csvCell(out, length.codeLineCount); out << ',';
            csvCell(out, length.totalFileLineCount); out << ',';
            out << (length.totalFileLineCount > 500 ? 1 : 0) << ',';
            out << static_cast<int>(examples.hasExamples) << ',';
            csvCell(out, examples.exampleCount); out << ',';
            csvCell(out, examples.exampleCodeBlocks); out << ',';
            csvCell(out, examples.exampleSections); out << ',';
            csvCell(out, examples.examplesPer1000Words); out << ',';
            out << sqlite3_column_int64(stmt, 4) << ',';
            const auto repositoryCount = sqlite3_column_int64(stmt, 5);
            out << repositoryCount << ',' << std::max<std::int64_t>(0, repositoryCount - 1) << ',';
            out << sqlite3_column_int64(stmt, 6) << ',';
            out << sqlite3_column_int(stmt, 7) << ',';
            csvNullableInt(out, stmt, 8); out << ',';
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL)
                out << std::max<std::int64_t>(0, sqlite3_column_int64(stmt, 8) - 1);
            out << ',';
            csvNullableText(out, stmt, 9); out << ',';
            csvNullableText(out, stmt, 10); out << ',';
            csvNullableInt(out, stmt, 11); out << ',';
            csvNullableInt(out, stmt, 12); out << ',';
            csvNullableInt(out, stmt, 13); out << ',';
            csvNullableInt(out, stmt, 14); out << ',';
            csvNullableText(out, stmt, 15); out << ',';
            csvNullableInt(out, stmt, 16); out << ',';
            if (sqlite3_column_type(stmt, 16) != SQLITE_NULL) {
                const auto provided = sqlite3_column_int64(stmt, 16);
                out << static_cast<std::int64_t>(length.characterCount) - provided << ',';
                out << (provided == static_cast<std::int64_t>(length.characterCount) ? 1 : 0);
            } else out << ',';
            out << '\n';
            ++records;
        }
        if (rc != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db));
        out.close();
        if (!out) throw std::runtime_error("CSV writing failed");
        sqlite3_finalize(stmt); stmt = nullptr;
        sqlite3_close(db); db = nullptr;
        std::cout << "Exported " << records << " unique skills to " << argv[2] << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        if (stmt) sqlite3_finalize(stmt);
        if (db) sqlite3_close(db);
        return 1;
    }
}
