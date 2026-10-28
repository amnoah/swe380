#include "examples_test.h"
#include "evaluations/examples/example_evaluator.h"
#include "evaluations/shared/markdown_utils.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace std;

void ExamplesTest::runAllTests() {

    cout << "=================================" << endl;
    cout << " EXAMPLES ANALYZER TESTS" << endl;
    cout << "=================================" << endl;

    cout << "\nTest 1: Basic processing" << endl;

    const std::string sample = "---\nname: testing\ndescription: ignored\n---\n# Testing\nRun pytest.\n\n## Examples\n```bash\npytest tests/\n```\n";
    auto examples = SkillExampleEvaluator::evaluate(sample, 11);
    assert(examples.hasExamples);
    assert(examples.exampleCount == 1);
    assert(examples.exampleCodeBlocks == 1);
    assert(examples.exampleSections == 1);
    assert(examples.examplesPer1000Words > 0);

    cout << "PASS" << endl;
    cout << "\nTest 2: Non-example Code Blocks" << endl;

    auto plain = SkillExampleEvaluator::evaluate("# Simple\nRun pytest to test code.\n```bash\npytest tests/\n```\n", 10);
    assert(!plain.hasExamples); // A code block alone is not necessarily an example.
    assert(plain.exampleCodeBlocks == 0);

    cout << "PASS" << endl;
    cout << "\nTest 3: Example code blocks" << endl;

    auto adjacent = SkillExampleEvaluator::evaluate("Use the following, for example:\n~~~sh\nls\n~~~\n", 10);
    assert(adjacent.exampleCount == 1);

    cout << "PASS" << endl;
    cout << "\nTest 4: In-line Example" << endl;

    auto inlineSample = SkillExampleEvaluator::evaluate("Example: `pytest tests/`\n", 3);
    assert(inlineSample.exampleCount == 1);

    cout << "PASS" << endl;
    cout << "\nTest 5: No code block" << endl;

    auto negativeHeader = SkillExampleEvaluator::evaluate("# Sample Size Estimation\nThere are no demonstrations.\n", 7);
    assert(negativeHeader.exampleCount == 0);

    cout << "PASS" << endl;
    cout << "\nTest 6: Non-example data" << endl;

    auto negativeCode = SkillExampleEvaluator::evaluate("sample_rate: 500\n", 2);
    assert(negativeCode.exampleCount == 0);

    cout << "PASS" << endl;
    cout << "\nTest 7: Linux commands example" << endl;

    auto bold = SkillExampleEvaluator::evaluate("**Example:** `ls`\n", 2);
    assert(bold.exampleCount == 1);

    cout << "PASS" << endl;

    cout << "\n=================================" << endl;
    cout << " ALL TESTS COMPLETED SUCCESSFULLY" << endl;
    cout << "=================================" << endl;
}