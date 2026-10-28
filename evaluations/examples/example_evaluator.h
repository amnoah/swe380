#ifndef SWE380_EXAMPLE_EVALUATOR_H
#define SWE380_EXAMPLE_EVALUATOR_H
#include <cstddef>
#include <string>

struct ExampleMetrics {
    bool hasExamples = false;
    size_t exampleCount = 0;       // Explicit sections/labels + contextual example fences, deduplicated
    size_t exampleCodeBlocks = 0;  // Fences in example sections or with an example cue nearby
    size_t exampleSections = 0;    // Explicit heading/label demonstrations
    double examplesPer1000Words = 0.0;
};

class SkillExampleEvaluator {
public:
    static ExampleMetrics evaluate(const std::string& rawMarkdown, size_t bodyWordCount);
};
#endif
