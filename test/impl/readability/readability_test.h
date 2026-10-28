#ifndef SWE380_READABILITY_TEST_H
#define SWE380_READABILITY_TEST_H

class ReadabilityTest
{
public:
    static void testBasicAnalysis();
    static void testEmptyInput();
    static void testReadabilityMetrics();
    static void testSyllableCounting();
    static void testMarkdownPreprocessing();
    static void testIgnoreCodeBlocks();
    static void testIgnoreFrontmatter();
    static void testFileAnalysis();
    static void testStaticEvaluation();

    static void runAllTests();
};

#endif