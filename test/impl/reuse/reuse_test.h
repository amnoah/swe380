#ifndef SWE380_REUSE_TEST_H
#define SWE380_REUSE_TEST_H

class ReuseTest {
    public:
        static void testDatabaseConnection();
        static void testArtifactCount();
        static void testGetSkillsByRepository();
        static void testMultiSkillRepositories();
        static void testSimilarityIdenticalText();
        static void testSimilarityDifferentText();
        static void testSimilarityNearMatch();
        static void testFrontmatterRemoval();
        static void testSurvivalRate();
        static void testAnalyzeRepositoryReuse();
        static void testAnalyzeAllRepositories();
        static void testAnalysisOptions();

        static void runAllTests();
};

#endif //SWE380_REUSE_TEST_H