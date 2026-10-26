#include "reuse_test.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include "evaluations/reuse/reuse_evaluator.h"

using namespace std;

const string dbPath = "agent_skills_sample.db";

    // Test 1: Database connection
    void ReuseTest::testDatabaseConnection()
    {
        cout << "\nTest 1: Database Connection" << endl;

        GitSkillsDatabaseParser parser(dbPath);

        assert(parser.isOpen());

        cout << "Database connected successfully." << endl;
        cout << "PASS" << endl;
    }

    // Test 2: Artifact count
    void ReuseTest::testArtifactCount()
    {
        cout << "\nTest 2: Artifact Count" << endl;

        GitSkillsDatabaseParser parser(dbPath);

        assert(parser.isOpen());

        int64_t count = parser.getArtifactCount();

        assert(count >= 0);

        cout << "Total artifacts: " << count << endl;
        cout << "PASS" << endl;
    }

    // Test 3: Retrieve skills by repository
    void ReuseTest::testGetSkillsByRepository()
    {
        cout << "\nTest 3: Skills By Repository" << endl;

        GitSkillsDatabaseParser parser(dbPath);

        assert(parser.isOpen());

        vector<string> repos = parser.getMultiSkillRepositories(1);

        if (!repos.empty())
        {
            vector<SkillOccurrence> skills =
                parser.getSkillsByRepository(repos[0]);

            assert(!skills.empty());

            cout << "Repository: " << repos[0] << endl;
            cout << "Skills found: " << skills.size() << endl;
        }

        cout << "PASS" << endl;
    }

    // Test 4: Multi-skill repositories
    void ReuseTest::testMultiSkillRepositories()
    {
        cout << "\nTest 4: Multi-Skill Repositories" << endl;

        GitSkillsDatabaseParser parser(dbPath);

        assert(parser.isOpen());

        vector<string> repos =
            parser.getMultiSkillRepositories(10);

        for (const auto& repo : repos)
        {
            cout << "Repository: " << repo << endl;
        }

        assert(repos.size() <= 10);

        cout << "Repositories found: " << repos.size() << endl;
        cout << "PASS" << endl;
    }

    // Test 5: Identical text similarity
    void ReuseTest::testSimilarityIdenticalText()
    {
        cout << "\nTest 5: Identical Text Similarity" << endl;

        string text =
            "This skill provides instructions for analyzing text "
            "and processing information efficiently.";

        double similarity =
            SkillReuseAnalyzer::calculateSimilarity(text, text);

        assert(similarity == 1.0);

        cout << "Similarity: " << similarity << endl;
        cout << "PASS" << endl;
    }

    // Test 6: Different text similarity
    void ReuseTest::testSimilarityDifferentText()
    {
        cout << "\nTest 6: Different Text Similarity" << endl;

        string text1 =
            "This skill explains how to analyze documents and process text.";

        string text2 =
            "Completely unrelated information about cooking dinner.";

        double similarity =
            SkillReuseAnalyzer::calculateSimilarity(text1, text2);

        assert(similarity >= 0.0);
        assert(similarity <= 1.0);

        cout << "Similarity: " << similarity << endl;
        cout << "PASS" << endl;
    }

    // Test 7: Near-match similarity
    void ReuseTest::testSimilarityNearMatch()
    {
        cout << "\nTest 7: Near-Match Similarity" << endl;

        string text1 =
            "This skill provides instructions for analyzing text "
            "and processing information efficiently.";

        string text2 =
            "This skill provides instructions for analyzing text "
            "and processing information effectively.";

        double similarity =
            SkillReuseAnalyzer::calculateSimilarity(text1, text2);

        assert(similarity > 0.0);
        assert(similarity < 1.0);

        cout << "Near-match similarity: " << similarity << endl;
        cout << "PASS" << endl;
    }

    // Test 8: Frontmatter removal
    void ReuseTest::testFrontmatterRemoval()
    {
        cout << "\nTest 8: Frontmatter Removal" << endl;

        string text1 =
            "---\n"
            "name: Skill One\n"
            "description: First description\n"
            "---\n"
            "This skill explains how to analyze text.";

        string text2 =
            "---\n"
            "name: Skill Two\n"
            "description: Different description\n"
            "---\n"
            "This skill explains how to analyze text.";

        double withFrontmatter =
            SkillReuseAnalyzer::calculateSimilarity(
                text1, text2, false);

        double withoutFrontmatter =
            SkillReuseAnalyzer::calculateSimilarity(
                text1, text2, true);

        assert(withoutFrontmatter == 1.0);
        assert(withFrontmatter < withoutFrontmatter);

        cout << "Similarity with frontmatter: "
             << withFrontmatter << endl;

        cout << "Similarity without frontmatter: "
             << withoutFrontmatter << endl;

        cout << "PASS" << endl;
    }

    // Test 9: Survival rate calculation
    void ReuseTest::testSurvivalRate()
    {
        cout << "\nTest 9: Survival Rate" << endl;

        double activeRate =
            SkillReuseAnalyzer::calculateSurvivalRate(
                "2026-07-01T00:00:00Z",
                "2026-07-20T00:00:00Z",
                "2026-07-31T23:59:59Z");

        double droppedRate =
            SkillReuseAnalyzer::calculateSurvivalRate(
                "2025-01-01T00:00:00Z",
                "2025-02-01T00:00:00Z",
                "2026-07-31T23:59:59Z");

        assert(activeRate >= 0.0 && activeRate <= 1.0);
        assert(droppedRate >= 0.0 && droppedRate <= 1.0);

        cout << "Active survival rate: "
             << activeRate << endl;

        cout << "Dropped survival rate: "
             << droppedRate << endl;

        cout << "PASS" << endl;
    }

    // Test 10: Analyze repository reuse
    void ReuseTest::testAnalyzeRepositoryReuse()
    {
        cout << "\nTest 10: Repository Reuse Analysis" << endl;

        SkillReuseAnalyzer analyzer(dbPath);

        assert(analyzer.isOpen());

        GitSkillsDatabaseParser parser(dbPath);

        vector<string> repos =
            parser.getMultiSkillRepositories(1);

        if (!repos.empty())
        {
            vector<NearMatchReuseResult> results =
                analyzer.analyzeRepositoryReuse(repos[0]);

            for (const auto& result : results)
            {
                assert(result.reuseCount > 0);
                assert(result.averageSimilarity >= 0.80);
                assert(result.averageSimilarity <= 0.999);
                assert(result.survivalRate >= 0.0);
                assert(result.survivalRate <= 1.0);

                cout << "Repository: "
                     << result.repoFullName << endl;

                cout << "Primary skill: "
                     << result.primaryPath << endl;

                cout << "Reuse count: "
                     << result.reuseCount << endl;

                cout << "Average similarity: "
                     << result.averageSimilarity << endl;

                cout << "Survival rate: "
                     << result.survivalRate << endl;

                cout << "Lifespan (days): "
                     << result.lifespanDays << endl;

                cout << "Total commits: "
                     << result.totalCommits << endl;
            }
        }

        cout << "PASS" << endl;
    }

    // Test 11: Analyze all repositories
    void ReuseTest::testAnalyzeAllRepositories()
    {
        cout << "\nTest 11: Analyze All Repositories" << endl;

        SkillReuseAnalyzer analyzer(dbPath);

        assert(analyzer.isOpen());

        vector<NearMatchReuseResult> results =
            analyzer.analyzeAllRepositories(5);

        for (const auto& result : results)
        {
            cout << "Repository: "
                 << result.repoFullName << endl;

            cout << "Reuse count: "
                 << result.reuseCount << endl;
        }

        cout << "Total reuse groups: "
             << results.size() << endl;

        cout << "PASS" << endl;
    }

    // Test 12: Analysis options
    void ReuseTest::testAnalysisOptions()
    {
        cout << "\nTest 12: Analysis Options" << endl;

        AnalysisOptions options;

        options.minSimilarityThreshold = 0.85;
        options.maxSimilarityThreshold = 0.99;
        options.minContentLength = 100;
        options.ignoreFrontmatter = false;

        SkillReuseAnalyzer analyzer(dbPath, options);

        assert(analyzer.isOpen());

        const AnalysisOptions& actual =
            analyzer.getOptions();

        assert(actual.minSimilarityThreshold == 0.85);
        assert(actual.maxSimilarityThreshold == 0.99);
        assert(actual.minContentLength == 100);
        assert(actual.ignoreFrontmatter == false);

        cout << "Minimum similarity: "
             << actual.minSimilarityThreshold << endl;

        cout << "Maximum similarity: "
             << actual.maxSimilarityThreshold << endl;

        cout << "Minimum content length: "
             << actual.minContentLength << endl;

        cout << "Ignore frontmatter: "
             << actual.ignoreFrontmatter << endl;

        cout << "PASS" << endl;
    }

    // Run all tests
    void ReuseTest::runAllTests()
    {
        cout << "=================================" << endl;
        cout << " SKILL REUSE ANALYZER TESTS" << endl;
        cout << "=================================" << endl;

        testDatabaseConnection();
        testArtifactCount();
        testGetSkillsByRepository();
        testMultiSkillRepositories();
        testSimilarityIdenticalText();
        testSimilarityDifferentText();
        testSimilarityNearMatch();
        testFrontmatterRemoval();
        testSurvivalRate();
        testAnalyzeRepositoryReuse();
        testAnalyzeAllRepositories();
        testAnalysisOptions();

        cout << "\n=================================" << endl;
        cout << " ALL TESTS COMPLETED SUCCESSFULLY" << endl;
        cout << "=================================" << endl;
    }
