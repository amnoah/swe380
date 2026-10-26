#include "readability_test.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

#include "evaluations/readability/readability_evaluator.h"

using namespace std;

    SkillReadabilityAnalyzer analyzer;


    // Test 1: Basic text analysis
    void ReadabilityTest::testBasicAnalysis()
    {
        cout << "\nTest 1: Basic Text Analysis" << endl;

        string text = "The cat sat.";

        ReadabilityMetrics result = analyzer.analyze(text);

        assert(result.wordCount == 3);
        assert(result.sentenceCount == 1);
        assert(result.syllableCount == 3);

        cout << "Words: " << result.wordCount << endl;
        cout << "Sentences: " << result.sentenceCount << endl;
        cout << "Syllables: " << result.syllableCount << endl;
        cout << "PASS" << endl;
    }

    // Test 2: Empty input
    void ReadabilityTest::testEmptyInput()
    {
        cout << "\nTest 2: Empty Input" << endl;

        string text = "";

        ReadabilityMetrics result = analyzer.analyze(text);

        assert(result.wordCount == 0);
        assert(result.sentenceCount == 0);
        assert(result.syllableCount == 0);

        cout << "PASS" << endl;
    }

    // Test 3: Different readability metrics
    void ReadabilityTest::testReadabilityMetrics()
    {
        cout << "\nTest 3: Readability Metrics" << endl;

        string text =
            "The cat sat on the mat. "
            "The dog ran across the yard. "
            "The bird flew into the tree.";

        double flesch = analyzer.calculateScore(
            text, ReadabilityMetric::FleschReadingEase);

        double grade = analyzer.calculateScore(
            text, ReadabilityMetric::FleschKincaidGrade);

        double coleman = analyzer.calculateScore(
            text, ReadabilityMetric::ColemanLiauIndex);

        double ari = analyzer.calculateScore(
            text, ReadabilityMetric::AutomatedReadability);

        double fog = analyzer.calculateScore(
            text, ReadabilityMetric::GunningFogIndex);

        assert(std::isfinite(flesch));
        assert(std::isfinite(grade));
        assert(std::isfinite(coleman));
        assert(std::isfinite(ari));
        assert(std::isfinite(fog));

        cout << "Flesch Reading Ease: " << flesch << endl;
        cout << "Flesch-Kincaid Grade: " << grade << endl;
        cout << "Coleman-Liau Index: " << coleman << endl;
        cout << "Automated Readability: " << ari << endl;
        cout << "Gunning Fog Index: " << fog << endl;

        cout << "PASS" << endl;
    }

    // Test 4: Syllable counting
    void ReadabilityTest::testSyllableCounting()
    {
        cout << "\nTest 4: Syllable Counting" << endl;

        assert(SkillReadabilityAnalyzer::countSyllables("cat") == 1);
        assert(SkillReadabilityAnalyzer::countSyllables("beautiful") == 3);
        assert(SkillReadabilityAnalyzer::countSyllables("") == 0);

        cout << "PASS" << endl;
    }

    // Test 5: Markdown preprocessing
    void ReadabilityTest::testMarkdownPreprocessing()
    {
        cout << "\nTest 5: Markdown Preprocessing" << endl;

        string text =
            "# Introduction\n"
            "The cat sat on the mat.\n"
            "* The dog ran across the yard.";

        ReadabilityMetrics result = analyzer.analyze(text);

        assert(result.wordCount == 13);

        cout << "Words after Markdown processing: "
             << result.wordCount << endl;

        cout << "PASS" << endl;
    }

    // Test 6: Ignore code blocks
    void ReadabilityTest::testIgnoreCodeBlocks()
    {
        cout << "\nTest 6: Ignore Code Blocks" << endl;

        string text =
            "The cat sat on the mat.\n"
            "```cpp\n"
            "This code should be ignored completely.\n"
            "```\n"
            "The dog ran.";

        ReadabilityMetrics result = analyzer.analyze(text);

        assert(result.wordCount == 9);

        cout << "Words after removing code: "
             << result.wordCount << endl;

        cout << "PASS" << endl;
    }

    // Test 7: Ignore frontmatter
    void ReadabilityTest::testIgnoreFrontmatter()
    {
        cout << "\nTest 7: Ignore Frontmatter" << endl;

        string text =
            "---\n"
            "name: Test Skill\n"
            "description: This is metadata.\n"
            "---\n"
            "The cat sat on the mat.";

        ReadabilityMetrics result = analyzer.analyze(text);

        assert(result.wordCount == 6);

        cout << "Words after removing frontmatter: "
             << result.wordCount << endl;

        cout << "PASS" << endl;
    }

    // Test 8: File-based analysis
    void ReadabilityTest::testFileAnalysis()
    {
        cout << "\nTest 8: File-Based Analysis" << endl;

        ofstream file("test_skill.md");

        file << "The cat sat on the mat.";
        file.close();

        ReadabilityMetrics result =
            analyzer.analyzeFile("test_skill.md");

        assert(result.wordCount == 6);

        cout << "Words read from file: "
             << result.wordCount << endl;

        cout << "PASS" << endl;
    }

    // Test 9: Static evaluation
    void ReadabilityTest::testStaticEvaluation()
    {
        cout << "\nTest 9: Static Evaluation" << endl;

        string text = "The cat sat on the mat.";

        double score = SkillReadabilityAnalyzer::evaluate(
            text,
            ReadabilityMetric::FleschReadingEase
        );

        assert(std::isfinite(score));

        cout << "Static readability score: "
             << score << endl;

        cout << "PASS" << endl;
    }

    // Run all tests
    void ReadabilityTest::runAllTests()
    {
        cout << "=================================" << endl;
        cout << " SKILL READABILITY ANALYZER TESTS" << endl;
        cout << "=================================" << endl;

        testBasicAnalysis();
        testEmptyInput();
        testReadabilityMetrics();
        testSyllableCounting();
        testMarkdownPreprocessing();
        testIgnoreCodeBlocks();
        testIgnoreFrontmatter();
        testFileAnalysis();
        testStaticEvaluation();

        cout << "\n=================================" << endl;
        cout << " ALL TESTS COMPLETED SUCCESSFULLY" << endl;
        cout << "=================================" << endl;
    }