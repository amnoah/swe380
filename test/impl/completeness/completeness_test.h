#ifndef SWE380_COMPLETENESS_TEST_H
#define SWE380_COMPLETENESS_TEST_H


class CompletenessTest
{
public:
    static void testEmptySkill();
    static void testCompleteSkill();
    static void testScoreRange();
    static void testFrontmatterDetection();
    static void testMissingFrontmatter();
    static void testMissingNameAndDescription();
    static void testBracketBalance();
    static void testUnclosedCodeBlock();
    static void testPunctuationHealth();
    static void testIncompleteSentences();
    static void testPlaceholderDetection();
    static void testShortContent();
    static void testHeaderAndCodeExclusion();
    static void runAllTests();
};


#endif //SWE380_COMPLETENESS_TEST_H